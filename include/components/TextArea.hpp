#pragma once

/**
 * \file TextArea.hpp
 * \brief A multi-line UTF-8 editable text area with word wrapping and vertical scrolling.
 *
 * `TextArea` is a single `Stack` node that renders its own wrapped visual
 * lines, selection highlights, blinking caret, placeholder, and internal
 * scrollbar.
 *
 * Editing model:
 *  - Text is stored as UTF-8 (`std::string`); cursor movement, deletion,
 *    word jumps (`Ctrl+Arrow`/`Backspace`/`Delete`), double-click selection,
 *    click-to-caret, and soft word-wrapping all operate on grapheme-cluster
 *    boundaries via `ZenitUI::Utf8`.
 *  - `Enter` inserts a `\n` newline while keeping focus; `Escape` releases focus.
 *  - Supports `Ctrl+A/C/X/V` clipboard operations (normalizing `\r\n` / `\r`
 *    to `\n`), `Ctrl+Z/Y` (and `Ctrl+Shift+Z`) 50-step undo/redo history, and
 *    mouse-wheel scrolling that yields to parent scroll containers at bounds.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Utf8.hpp"
#include <cctype>
#include <cmath>
#include <deque>

namespace ZenitUI::UI
{
    /** \addtogroup grp_widgets
     *  @{
     */

    /**
     * \class TextArea
     * \ingroup grp_widgets
     * \brief A multi-line UTF-8 text editor.
     *
     * Style tag: `"TextArea"`. Interactive and focusable.
     * Styleable parts:
     *  - `::placeholder` — placeholder text colour when empty.
     *  - `::selection` — selection highlight background.
     *  - `::cursor` — caret colour and width (`borderWidth.left`).
     */
    class TextArea : public TLayout<TextArea>
    {
    public:
        /**
         * \brief Construct with optional initial text and visible row count.
         * \param initial Initial UTF-8 content; cursor starts at the end.
         * \param rows    Minimum visible rows used for intrinsic height (clamped to `>= 1`).
         */
        TextArea(std::string initial = "", int rows = 3)
            : TLayout<TextArea>(LayoutType::Stack),
              text(std::move(initial)),
              rows_(std::max(1, rows))
        {
            setInteractive(true);
            setFocusable(true);
            setStyleTag("TextArea");
            cursorPos = (int)text.size();
            saveHistory();

            style_.inlineDefaults.background = Color{18, 18, 20, 255};
            style_.inlineDefaults.color = Colors::White;
            style_.inlineDefaults.radius = Px(6.0f);
            style_.inlineDefaults.padding = Spacing(Px(8.0f), Px(12.0f));
            style_.inlineDefaults.borderColor = Color{51, 51, 56, 255};
            style_.inlineDefaults.borderWidth = Px(1.0f);
            internal_.pendingTransition = 1;
        }

        /** \brief Replace the content, snap the cursor to a valid UTF-8 boundary, clear selection, and record undo history. */
        void setText(std::string t)
        {
            text = std::move(t);
            cursorPos = static_cast<int>(
                Utf8::snapFloor(text, std::min<size_t>(static_cast<size_t>(std::max(0, cursorPos)), text.size())));
            selectionStart = -1;
            scrollY = 0.0f;
            text_dirty = true;
            internal_.pendingTransition = 1;
            saveHistory();
        }
        /** \brief Current UTF-8 content. */
        const std::string &getText() const { return text; }
        void clear() { setText(""); }

        /** \brief Current cursor position in bytes from the start. */
        int getCursorPos() const { return cursorPos; }

        /** \brief Move the cursor (clamped and snapped to a valid UTF-8 boundary); clears selection. */
        void setCursorPos(int pos)
        {
            size_t clamped = static_cast<size_t>(std::clamp(pos, 0, static_cast<int>(text.size())));
            cursorPos = static_cast<int>(Utf8::snapFloor(text, clamped));
            selectionStart = -1;
            internal_.pendingTransition = 1;
        }

        /** \brief Set placeholder text displayed when the field is empty. */
        void setPlaceholder(std::string p)
        {
            placeholder = std::move(p);
            internal_.pendingTransition = 1;
        }
        /** \brief Current placeholder text. */
        const std::string &getPlaceholder() const { return placeholder; }

        /** \brief Enable or disable soft wrapping for the placeholder (default: `true`). */
        void setPlaceholderWrap(bool w)
        {
            placeholderWrap_ = w;
            placeholder_dirty = true;
            internal_.pendingTransition = 1;
        }
        /** \brief `true` if placeholder wrapping is enabled. */
        bool getPlaceholderWrap() const { return placeholderWrap_; }

        /** \brief Set the row count used to compute intrinsic height (clamped to `>= 1`). */
        void setRows(int r)
        {
            rows_ = std::max(1, r);
            internal_.pendingTransition = 1;
        }

        /** \brief Configured row count. */
        int getRows() const { return rows_; }

        /** \brief Override the font used for measurement and rendering. */
        void setFont(FontHandle f)
        {
            defaultFont = f;
            text_dirty = true;
            internal_.pendingTransition = 1;
        }

        std::function<void(const std::string &)> onTextChanged;

    protected:
        struct VisualLine
        {
            int start{0};
            int end{0};
            bool hardBreak{true};
        };

        /** \brief Compute intrinsic size from roughly 16 "M" glyphs and rows_ line heights. */
        Vec2 computeIntrinsicSize(float, float) override
        {
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            const Vec2 vp = viewportSize();
            float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
            float charW = 0.0f;
            if (auto r = UIContext::get().renderer)
                charW = r->measureText(f, "M", fs, 0.0f).x;
            if (charW <= 0.0f)
                charW = fs * 0.6f;
            float lineH = fs * 1.4f;
            return {std::max(180.0f, charW * 16.0f), std::max(40.0f, lineH * (float)rows_)};
        }

        /** \brief Reset focus, selection, and key-repeat state when disabled. */
        void onEnabledChanged(bool nowEnabled) override
        {
            if (!nowEnabled)
            {
                hasFocusCache = false;
                repeatStates.clear();
                cursorBlink = 0.0f;
                selectionStart = -1;
            }
        }

        /** \brief Handle pointer selection, wheel scrolling, keyboard editing, and auto-scroll. */
        void onUpdate(float dt) override
        {
            hasFocusCache = UIContext::get().hasFocus(this);
            cursorBlink += dt;
            if (cursorBlink > 1.0f)
                cursorBlink -= 1.0f;

            auto &ctx = UIContext::get();
            auto &ev = ctx.inputEvents;
            const Vec2 vp = viewportSize();
            bool changed = false;

            float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height, vp);
            float bwt = style_.currentStyle.borderWidth.top.resolveV(rect.width, rect.height, vp);
            float bwb = style_.currentStyle.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            float pt = style_.currentStyle.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            float pb = style_.currentStyle.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            float innerW = std::max(0.0f, rect.width - pl - pr - 8.0f);
            float innerH = std::max(0.0f, rect.height - pt - pb);

            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
            float sp = style_.currentStyle.letterSpacing.resolveH(vp.x, vp.y, vp);
            float lineH = fs * 1.4f;

            ensureLines(innerW, f, fs, sp);

            float totalH = (float)visualLines.size() * lineH;
            float maxScroll = std::max(0.0f, totalH - innerH);
            scrollY = std::clamp(scrollY, 0.0f, maxScroll);

            // Scroll verticale con rotellina (cede allo scroll padre ai limiti)
            if (!ctx.wheelConsumedThisFrame && ctx.topmostConsumer == this &&
                rect.contains(ctx.pointer.pos) && ctx.pointer.wheelY != 0.0f && maxScroll > 0.0f)
            {
                float nextScroll = std::clamp(scrollY - ctx.pointer.wheelY * lineH * 2.0f, 0.0f, maxScroll);
                if (std::abs(nextScroll - scrollY) > 0.01f)
                {
                    scrollY = nextScroll;
                    ctx.wheelConsumedThisFrame = true;
                }
            }

            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
                UIContext::get().requestFocus(shared_from_this());

                double now = ctx.platform->time();
                clickCount = (now - lastClickTime < 0.4) ? (clickCount + 1) : 1;
                lastClickTime = now;

                cursorPos = cursorIndexFromPos(ctx.pointer.pos.x - rect.x, ctx.pointer.pos.y - rect.y,
                                               pl, pt, f, fs, sp, lineH);
                selectionStart = cursorPos;

                if (clickCount == 2)
                {
                    selectWordAtCursor();
                }
                else if (clickCount >= 3)
                {
                    selectionStart = 0;
                    cursorPos = (int)text.size();
                }

                cursorBlink = 0.0f;
                internal_.pendingTransition = 1;
            }

            if (hasPointerCapture() && clickCount == 1)
            {
                int newPos = cursorIndexFromPos(ctx.pointer.pos.x - rect.x, ctx.pointer.pos.y - rect.y,
                                                pl, pt, f, fs, sp, lineH);
                if (newPos != cursorPos)
                {
                    cursorPos = newPos;
                    cursorBlink = 0.0f;
                    internal_.pendingTransition = 1;
                }
            }

            if (!hasFocusCache)
            {
                repeatStates.clear();
                selectionStart = -1;
                return;
            }

            auto deleteSelection = [&]()
            {
                if (selectionStart != -1 && selectionStart != cursorPos)
                {
                    int s1 = std::min(selectionStart, cursorPos);
                    int s2 = std::max(selectionStart, cursorPos);
                    text.erase(text.begin() + s1, text.begin() + s2);
                    cursorPos = s1;
                    selectionStart = -1;
                    text_dirty = true;
                    changed = true;
                    return true;
                }
                selectionStart = -1;
                return false;
            };

            bool keepCursorInView = false;

            if (ctx.modifiers.ctrl || ctx.modifiers.super)
            {
                for (int k : ev.keys)
                {
                    if (k == Key::A)
                    {
                        selectionStart = 0;
                        cursorPos = (int)text.size();
                        internal_.pendingTransition = 1;
                    }
                    else if (k == Key::C || k == Key::X)
                    {
                        if (selectionStart != -1 && selectionStart != cursorPos)
                        {
                            int s1 = std::min(selectionStart, cursorPos);
                            int s2 = std::max(selectionStart, cursorPos);
                            ctx.platform->setClipboardText(text.substr(s1, s2 - s1));
                            if (k == Key::X)
                            {
                                deleteSelection();
                                saveHistory();
                                keepCursorInView = true;
                            }
                        }
                    }
                    else if (k == Key::V)
                    {
                        deleteSelection();
                        std::string clean = Utf8::sanitizeMultiLine(ctx.platform->getClipboardText());
                        if (!clean.empty())
                        {
                            text.insert(static_cast<size_t>(cursorPos), clean);
                            cursorPos += static_cast<int>(clean.size());
                            changed = true;
                        }
                        if (changed)
                        {
                            text_dirty = true;
                            saveHistory();
                            keepCursorInView = true;
                        }
                    }
                    else if (k == Key::Z)
                    {
                        if (ctx.modifiers.shift)
                            redo();
                        else
                            undo();
                        keepCursorInView = true;
                    }
                    else if (k == Key::Y)
                    {
                        redo();
                        keepCursorInView = true;
                    }
                }
            }
            else
            {
                bool inserted = false;
                for (int c : ev.chars)
                {
                    uint32_t cp = static_cast<uint32_t>(c);
                    if (!Utf8::isPrintable(cp))
                        continue;
                    deleteSelection();
                    std::string encoded = Utf8::encode(cp);
                    text.insert(static_cast<size_t>(cursorPos), encoded);
                    cursorPos += static_cast<int>(encoded.size());
                    text_dirty = true;
                    changed = true;
                    inserted = true;
                    keepCursorInView = true;
                }
                if (inserted)
                    saveHistory();
            }

            auto moveVertical = [&](int dir)
            {
                ensureLines(innerW, f, fs, sp);
                int curLine = lineIndexForPos(cursorPos);
                int targetLine = std::clamp(curLine + dir, 0, (int)visualLines.size() - 1);
                if (targetLine == curLine)
                    return;

                auto r = UIContext::get().renderer;
                float curX = 0.0f;
                if (r)
                {
                    int a = visualLines[curLine].start;
                    curX = r->measureText(f, std::string_view(text).substr(a, cursorPos - a), fs, sp).x;
                }
                cursorPos = closestIndexInLine(targetLine, curX, f, fs, sp);
                internal_.pendingTransition = 1;
                keepCursorInView = true;
            };

            auto doAction = [&](int key)
            {
                bool actionChanged = false;
                const bool wordJump = (ctx.modifiers.ctrl || ctx.modifiers.super);
                switch (key)
                {
                case Key::Backspace:
                    if (deleteSelection())
                        actionChanged = true;
                    else if (cursorPos > 0)
                    {
                        size_t prev = wordJump
                                          ? Utf8::prevWordBoundary(text, static_cast<size_t>(cursorPos))
                                          : Utf8::prevCluster(text, static_cast<size_t>(cursorPos));
                        text.erase(prev, static_cast<size_t>(cursorPos) - prev);
                        cursorPos = static_cast<int>(prev);
                        actionChanged = true;
                    }
                    break;
                case Key::Delete:
                    if (deleteSelection())
                        actionChanged = true;
                    else if (cursorPos < static_cast<int>(text.size()))
                    {
                        size_t next = wordJump
                                          ? Utf8::nextWordBoundary(text, static_cast<size_t>(cursorPos))
                                          : Utf8::nextCluster(text, static_cast<size_t>(cursorPos));
                        text.erase(static_cast<size_t>(cursorPos), next - static_cast<size_t>(cursorPos));
                        actionChanged = true;
                    }
                    break;
                case Key::Left:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    if (cursorPos > 0)
                    {
                        size_t prev = wordJump
                                          ? Utf8::prevWordBoundary(text, static_cast<size_t>(cursorPos))
                                          : Utf8::prevCluster(text, static_cast<size_t>(cursorPos));
                        cursorPos = static_cast<int>(prev);
                        internal_.pendingTransition = 1;
                    }
                    break;
                case Key::Right:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    if (cursorPos < static_cast<int>(text.size()))
                    {
                        size_t next = wordJump
                                          ? Utf8::nextWordBoundary(text, static_cast<size_t>(cursorPos))
                                          : Utf8::nextCluster(text, static_cast<size_t>(cursorPos));
                        cursorPos = static_cast<int>(next);
                        internal_.pendingTransition = 1;
                    }
                    break;
                case Key::Up:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    moveVertical(-1);
                    break;
                case Key::Down:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    moveVertical(1);
                    break;
                default:
                    break;
                }
                if (actionChanged)
                {
                    text_dirty = true;
                    changed = true;
                    if (key == Key::Backspace || key == Key::Delete)
                        saveHistory();
                }
                cursorBlink = 0.0f;
                keepCursorInView = true;
            };

            constexpr float REPEAT_DELAY = 0.40f;
            constexpr float REPEAT_INTERVAL = 0.04f;
            auto handleRepeat = [&](int key)
            {
                bool isHeld = std::find(ev.held.begin(), ev.held.end(), key) != ev.held.end();
                auto it = repeatStates.find(key);
                if (!isHeld)
                {
                    if (it != repeatStates.end())
                        repeatStates.erase(it);
                    return;
                }
                if (it == repeatStates.end())
                {
                    doAction(key);
                    repeatStates[key] = {0.0f, REPEAT_DELAY};
                }
                else
                {
                    it->second.holdTime += dt;
                    while (it->second.holdTime >= it->second.nextFireTime)
                    {
                        doAction(key);
                        it->second.nextFireTime += REPEAT_INTERVAL;
                    }
                }
            };

            handleRepeat(Key::Backspace);
            handleRepeat(Key::Delete);
            handleRepeat(Key::Left);
            handleRepeat(Key::Right);
            handleRepeat(Key::Up);
            handleRepeat(Key::Down);

            for (int k : ev.keys)
            {
                switch (k)
                {
                case Key::Enter:
                    deleteSelection();
                    text.insert(text.begin() + cursorPos, '\n');
                    cursorPos++;
                    text_dirty = true;
                    changed = true;
                    cursorBlink = 0.0f;
                    saveHistory();
                    keepCursorInView = true;
                    break;
                case Key::Escape:
                    UIContext::get().releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Home:
                {
                    ensureLines(innerW, f, fs, sp);
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    int ln = lineIndexForPos(cursorPos);
                    cursorPos = visualLines[ln].start;
                    internal_.pendingTransition = 1;
                    keepCursorInView = true;
                    break;
                }
                case Key::End:
                {
                    ensureLines(innerW, f, fs, sp);
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    int ln = lineIndexForPos(cursorPos);
                    if (visualLines[ln].hardBreak)
                    {
                        cursorPos = visualLines[ln].end;
                    }
                    else
                    {
                        size_t prev = Utf8::prevCluster(text, static_cast<size_t>(visualLines[ln].end));
                        cursorPos = std::max(visualLines[ln].start, static_cast<int>(prev));
                    }
                    internal_.pendingTransition = 1;
                    keepCursorInView = true;
                    break;
                }
                default:
                    break;
                }
            }

            if (changed && onTextChanged)
            {
                onTextChanged(text);
                internal_.pendingTransition = 1;
            }

            if (keepCursorInView)
            {
                ensureLines(innerW, f, fs, sp);
                int curLine = lineIndexForPos(cursorPos);
                float curTop = (float)curLine * lineH;
                float curBot = curTop + lineH;
                if (curTop < scrollY)
                    scrollY = curTop;
                if (curBot > scrollY + innerH)
                    scrollY = curBot - innerH;
                float newMax = std::max(0.0f, (float)visualLines.size() * lineH - innerH);
                scrollY = std::clamp(scrollY, 0.0f, newMax);
            }
        }

        /** \brief Render placeholder or wrapped lines, selection rects, caret, and internal scrollbar. */
        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = UIContext::get().renderer;
            if (!r)
                return;

            FontHandle f = resolveFont(style, defaultFont);
            const Vec2 vp = viewportSize();
            float fs = style.fontSize.resolveV(vp.x, vp.y, vp);
            float sp = style.letterSpacing.resolveH(vp.x, vp.y, vp);
            float lineH = fs * 1.4f;

            float bwl = style.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float bwr = style.borderWidth.right.resolveH(rect.width, rect.height, vp);
            float bwt = style.borderWidth.top.resolveV(rect.width, rect.height, vp);
            float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            float pl = style.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float pr = style.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            float pt = style.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            float pb = style.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            float innerW = std::max(0.0f, rect.width - pl - pr - 8.0f);
            float innerH = std::max(0.0f, rect.height - pt - pb);

            ensureLines(innerW, f, fs, sp);

            Rect clipRect = {rect.x + pl, rect.y + pt, std::max(0.0f, rect.width - pl - pr), innerH};
            r->pushClip(clipRect);

            Color textColor = config.enabled ? style.color : Color{120, 120, 125, 255};

            if (text.empty() && !placeholder.empty())
            {
                Style phStyle = partStyle("placeholder");
                Color phColor = phStyle.color.is_set ? phStyle.color.value : Color{115, 115, 125, 255};
                if (placeholderWrap_)
                {
                    ensurePlaceholderLines(innerW, f, fs, sp);
                    for (int i = 0; i < (int)placeholderLines.size(); ++i)
                    {
                        float ly = rect.y + pt + (float)i * lineH;
                        if (ly > rect.y + pt + innerH)
                            break;
                        const auto &ln = placeholderLines[i];
                        std::string_view lineView = std::string_view(placeholder).substr(ln.start, ln.end - ln.start);
                        if (!lineView.empty())
                            r->drawText(f, lineView, {rect.x + pl, ly}, fs, sp, phColor.withAlpha(op));
                    }
                }
                else
                {
                    r->drawText(f, placeholder, {rect.x + pl, rect.y + pt}, fs, sp, phColor.withAlpha(op));
                }
            }
            else
            {
                bool hasSel = hasFocusCache && selectionStart != -1 && selectionStart != cursorPos;
                int s1 = hasSel ? std::min(selectionStart, cursorPos) : -1;
                int s2 = hasSel ? std::max(selectionStart, cursorPos) : -1;
                Color selColor = Color{50, 130, 220, 128};
                if (hasSel)
                {
                    Style selStyle = partStyle("selection");
                    if (selStyle.background.is_set)
                        selColor = selStyle.background.value;
                }

                for (int i = 0; i < (int)visualLines.size(); ++i)
                {
                    float ly = rect.y + pt + (float)i * lineH - scrollY;
                    if (ly + lineH < rect.y + pt || ly > rect.y + pt + innerH)
                        continue;

                    const auto &ln = visualLines[i];
                    std::string_view lineView = std::string_view(text).substr(ln.start, ln.end - ln.start);

                    if (hasSel && s2 > ln.start && s1 < ln.end)
                    {
                        int a = std::max(s1, ln.start);
                        int b = std::min(s2, ln.end);
                        float preW = r->measureText(f, std::string_view(text).substr(ln.start, a - ln.start), fs, sp).x;
                        float selW = r->measureText(f, std::string_view(text).substr(a, b - a), fs, sp).x;
                        if (selW < 4.0f && s2 > ln.end)
                            selW = 6.0f;
                        r->fillRect({rect.x + pl + preW, ly, selW, lineH}, selColor.withAlpha(op));
                    }

                    if (!lineView.empty())
                        r->drawText(f, lineView, {rect.x + pl, ly}, fs, sp, textColor.withAlpha(op));
                }
            }

            if (hasFocusCache && config.enabled && cursorBlink < 0.5f &&
                (selectionStart == -1 || selectionStart == cursorPos))
            {
                Style cursorStyle = partStyle("cursor");
                Color cursorColor = cursorStyle.color.is_set ? cursorStyle.color.value : textColor;
                float cursorW = cursorStyle.borderWidth.is_set
                                    ? cursorStyle.borderWidth.value.left.resolveH(fs, fs, vp)
                                    : 2.0f;

                int curLine = lineIndexForPos(cursorPos);
                const auto &ln = visualLines[curLine];
                float cx = rect.x + pl +
                           r->measureText(f, std::string_view(text).substr(ln.start, cursorPos - ln.start), fs, sp).x;
                float cy = rect.y + pt + (float)curLine * lineH - scrollY;
                r->fillRect({cx, cy, cursorW, fs}, cursorColor.withAlpha(op));
            }

            // Scrollbar interna quando il testo supera l'altezza
            float totalH = (float)visualLines.size() * lineH;
            if (totalH > innerH + 1.0f && innerH > 10.0f)
            {
                float sbW = 4.0f;
                float sbX = rect.x + rect.width - bwr - sbW - 3.0f;
                float ratio = innerH / totalH;
                float thumbH = std::max(20.0f, innerH * ratio);
                float maxScroll = totalH - innerH;
                float scrollFrac = (maxScroll > 0.0f) ? (scrollY / maxScroll) : 0.0f;
                float thumbY = rect.y + pt + scrollFrac * (innerH - thumbH);
                r->fillRoundedRect({sbX, thumbY, sbW, thumbH}, 2.0f, Color{130, 130, 140, 180}.withAlpha(op));
            }

            r->popClip();
        }

    private:
        static std::vector<VisualLine> wrapString(const std::string &src, float availW,
                                                  FontHandle f, float fs, float sp)
        {
            std::vector<VisualLine> out;
            auto r = UIContext::get().renderer;
            auto measure = [&](size_t a, size_t b) -> float
            {
                if (!r || b <= a)
                    return 0.0f;
                return r->measureText(f, std::string_view(src).substr(a, b - a), fs, sp).x;
            };

            size_t len = src.size();
            size_t pStart = 0;
            while (true)
            {
                size_t pEnd = pStart;
                while (pEnd < len && src[pEnd] != '\n')
                    ++pEnd;

                if (pStart == pEnd || availW <= 1.0f || !r)
                {
                    out.push_back({static_cast<int>(pStart), static_cast<int>(pEnd), true});
                }
                else
                {
                    auto bounds = Utf8::clusterBoundaries(std::string_view(src).substr(pStart, pEnd - pStart));
                    size_t lineStartIdx = 0;
                    const size_t lastIdx = bounds.size() - 1;
                    while (lineStartIdx < lastIdx)
                    {
                        size_t lineStart = pStart + bounds[lineStartIdx];
                        if (measure(lineStart, pEnd) <= availW)
                        {
                            out.push_back({static_cast<int>(lineStart), static_cast<int>(pEnd), true});
                            break;
                        }

                        size_t lo = lineStartIdx + 1;
                        size_t hi = lastIdx;
                        while (lo < hi)
                        {
                            size_t mid = lo + (hi - lo + 1) / 2;
                            if (measure(lineStart, pStart + bounds[mid]) <= availW)
                                lo = mid;
                            else
                                hi = mid - 1;
                        }
                        size_t fitIdx = std::max(lineStartIdx + 1, lo);
                        size_t breakIdx = fitIdx;

                        if (fitIdx < lastIdx && src[pStart + bounds[fitIdx]] == ' ')
                        {
                            breakIdx = fitIdx + 1;
                        }
                        else
                        {
                            for (size_t k = fitIdx - 1; k > lineStartIdx; --k)
                            {
                                if (src[pStart + bounds[k]] == ' ')
                                {
                                    breakIdx = k + 1;
                                    break;
                                }
                            }
                        }

                        size_t breakPos = pStart + bounds[breakIdx];
                        out.push_back({static_cast<int>(lineStart), static_cast<int>(breakPos), false});
                        lineStartIdx = breakIdx;
                    }
                }

                if (pEnd >= len)
                    break;
                pStart = pEnd + 1;
            }

            if (out.empty())
                out.push_back({0, 0, true});
            return out;
        }

        void ensureLines(float availW, FontHandle f, float fs, float sp)
        {
            if (!text_dirty && std::abs(availW - lastWrapW) <= 0.5f && std::abs(fs - lastFontSize) <= 0.1f)
                return;
            text_dirty = false;
            lastWrapW = availW;
            lastFontSize = fs;
            visualLines = wrapString(text, availW, f, fs, sp);
        }

        void ensurePlaceholderLines(float availW, FontHandle f, float fs, float sp)
        {
            if (!placeholder_dirty && std::abs(availW - lastPhWrapW) <= 0.5f && std::abs(fs - lastPhFontSize) <= 0.1f)
                return;
            placeholder_dirty = false;
            lastPhWrapW = availW;
            lastPhFontSize = fs;
            placeholderLines = wrapString(placeholder, availW, f, fs, sp);
        }

        int lineIndexForPos(int pos) const
        {
            for (int i = 0; i < (int)visualLines.size(); ++i)
            {
                const auto &ln = visualLines[i];
                if (pos >= ln.start &&
                    (pos < ln.end || (pos == ln.end && (ln.hardBreak || i + 1 == (int)visualLines.size()))))
                    return i;
            }
            return std::max(0, (int)visualLines.size() - 1);
        }

        int closestIndexInLine(int lineIdx, float targetX, FontHandle f, float fs, float sp) const
        {
            auto r = UIContext::get().renderer;
            const auto &ln = visualLines[lineIdx];
            if (!r)
                return ln.start;

            int maxIdx = ln.hardBreak
                             ? ln.end
                             : std::max(ln.start,
                                        static_cast<int>(Utf8::prevCluster(text, static_cast<size_t>(ln.end))));
            if (maxIdx <= ln.start)
                return ln.start;

            auto bounds = Utf8::clusterBoundaries(
                std::string_view(text).substr(static_cast<size_t>(ln.start),
                                              static_cast<size_t>(maxIdx - ln.start)));

            int best = ln.start;
            float bestDist = std::abs(targetX);
            for (size_t bi = 1; bi < bounds.size(); ++bi)
            {
                size_t bytePos = static_cast<size_t>(ln.start) + bounds[bi];
                float w = r->measureText(
                               f,
                               std::string_view(text).substr(static_cast<size_t>(ln.start), bounds[bi]),
                               fs, sp)
                              .x;
                float dist = std::abs(targetX - w);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = static_cast<int>(bytePos);
                }
            }
            return best;
        }

        int cursorIndexFromPos(float localX, float localY, float pl, float pt,
                               FontHandle f, float fs, float sp, float lineH) const
        {
            if (visualLines.empty())
                return 0;
            int lineIdx = std::clamp((int)std::floor((localY - pt + scrollY) / lineH),
                                     0, (int)visualLines.size() - 1);
            return closestIndexInLine(lineIdx, localX - pl, f, fs, sp);
        }

        struct HistoryState
        {
            std::string text;
            int cursorPos;
        };
        std::deque<HistoryState> undoStack;
        std::deque<HistoryState> redoStack;

        void saveHistory()
        {
            if (!undoStack.empty() && undoStack.back().text == text)
                return;
            undoStack.push_back({text, cursorPos});
            if (undoStack.size() > 50)
                undoStack.pop_front();
            redoStack.clear();
        }

        void undo()
        {
            if (undoStack.size() > 1)
            {
                redoStack.push_back(undoStack.back());
                undoStack.pop_back();
                text = undoStack.back().text;
                cursorPos = undoStack.back().cursorPos;
                selectionStart = -1;
                text_dirty = true;
                internal_.pendingTransition = 1;
            }
        }

        void redo()
        {
            if (!redoStack.empty())
            {
                undoStack.push_back(redoStack.back());
                text = redoStack.back().text;
                cursorPos = redoStack.back().cursorPos;
                redoStack.pop_back();
                selectionStart = -1;
                text_dirty = true;
                internal_.pendingTransition = 1;
            }
        }

        void selectWordAtCursor()
        {
            if (text.empty())
                return;
            size_t s = Utf8::snapFloor(text, static_cast<size_t>(std::max(0, cursorPos)));
            size_t e = s;
            while (s > 0)
            {
                size_t prev = Utf8::prevCluster(text, s);
                uint32_t cp = 0;
                Utf8::decode(text, prev, cp);
                if (!Utf8::isWordChar(cp))
                    break;
                s = prev;
            }
            while (e < text.size())
            {
                uint32_t cp = 0;
                Utf8::decode(text, e, cp);
                if (!Utf8::isWordChar(cp))
                    break;
                e = Utf8::nextCluster(text, e);
            }
            if (s != e)
            {
                selectionStart = static_cast<int>(s);
                cursorPos = static_cast<int>(e);
            }
        }

        std::string text;
        std::string placeholder;
        bool placeholderWrap_{true};
        bool placeholder_dirty{true};
        float lastPhWrapW{-1.0f};
        float lastPhFontSize{-1.0f};
        std::vector<VisualLine> placeholderLines;
        int rows_{3};
        int cursorPos{0};
        int selectionStart{-1};
        bool text_dirty{true};
        bool hasFocusCache{false};
        float cursorBlink{0.0f};
        float scrollY{0.0f};
        float lastWrapW{-1.0f};
        float lastFontSize{-1.0f};
        FontHandle defaultFont{0};

        std::vector<VisualLine> visualLines;
        int clickCount{0};
        double lastClickTime{0.0};

        struct RepeatState
        {
            float holdTime{0.0f};
            float nextFireTime{0.0f};
        };
        std::unordered_map<int, RepeatState> repeatStates;
    };
    /** @} */
} // namespace ZenitUI::UI