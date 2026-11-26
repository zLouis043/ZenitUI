#pragma once

/**
 * \file TextArea.hpp
 * \brief A multi-line UTF-8 editable text area backed by `UIContext::textEngine`.
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
     * Styleable parts: `::placeholder`, `::selection`, `::cursor`.
     */
    class TextArea : public TLayout<TextArea>
    {
    public:
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
        const std::string &getText() const { return text; }
        void clear() { setText(""); }

        int getCursorPos() const { return cursorPos; }

        void setCursorPos(int pos)
        {
            size_t clamped = static_cast<size_t>(std::clamp(pos, 0, static_cast<int>(text.size())));
            cursorPos = static_cast<int>(Utf8::snapFloor(text, clamped));
            selectionStart = -1;
            internal_.pendingTransition = 1;
        }

        void setPlaceholder(std::string p)
        {
            placeholder = std::move(p);
            placeholder_dirty = true;
            internal_.pendingTransition = 1;
        }
        const std::string &getPlaceholder() const { return placeholder; }

        void setPlaceholderWrap(bool w)
        {
            placeholderWrap_ = w;
            placeholder_dirty = true;
            internal_.pendingTransition = 1;
        }
        bool getPlaceholderWrap() const { return placeholderWrap_; }

        void setRows(int r)
        {
            rows_ = std::max(1, r);
            internal_.pendingTransition = 1;
        }

        int getRows() const { return rows_; }

        void setFont(FontHandle f)
        {
            defaultFont = f;
            text_dirty = true;
            placeholder_dirty = true;
            internal_.pendingTransition = 1;
        }

        std::function<void(const std::string &)> onTextChanged;

    protected:
        Vec2 computeIntrinsicSize(float, float) override
        {
            auto *ctx = context();
            if (!ctx)
                return {0.0f, 0.0f};

            TextMeasureSpec mSpec = makeSpec(style_.currentStyle);
            mSpec.letterSpacing = 0.0f;
            float charW = ctx->textEngine.measureText("M", mSpec).x;
            if (charW <= 0.0f)
                charW = mSpec.fontSize * 0.6f;

            const float lineH = mSpec.lineHeight;
            return {std::max(180.0f, charW * 16.0f), std::max(40.0f, lineH * (float)rows_)};
        }

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

        void onUpdate(float dt) override
        {
            auto *ctx = context();
            if (!ctx)
                return;
            hasFocusCache = ctx->hasFocus(this);
            cursorBlink += dt;
            if (cursorBlink > 1.0f)
                cursorBlink -= 1.0f;

            auto &ev = ctx->inputEvents;
            const Vec2 vp = viewportSize();
            bool changed = false;

            const float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            const float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height, vp);
            const float bwt = style_.currentStyle.borderWidth.top.resolveV(rect.width, rect.height, vp);
            const float bwb = style_.currentStyle.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            const float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            const float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            const float pt = style_.currentStyle.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            const float pb = style_.currentStyle.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            const float innerW = std::max(0.0f, rect.width - pl - pr - 8.0f);
            const float innerH = std::max(0.0f, rect.height - pt - pb);

            const float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
            const float lineH = fs * 1.4f;

            ensureLines(innerW, style_.currentStyle);

            const float totalH = textLayout_ ? textLayout_->size.y : lineH;
            const float maxScroll = std::max(0.0f, totalH - innerH);
            scrollY = std::clamp(scrollY, 0.0f, maxScroll);

            // Vertical mouse-wheel scrolling (yields to parent scroll containers at bounds).
            if (!ctx->wheelConsumedThisFrame && ctx->topmostConsumer == this &&
                rect.contains(ctx->pointer.pos) && ctx->pointer.wheelY != 0.0f && maxScroll > 0.0f)
            {
                float nextScroll = std::clamp(scrollY - ctx->pointer.wheelY * lineH * 2.0f, 0.0f, maxScroll);
                if (std::abs(nextScroll - scrollY) > 0.01f)
                {
                    scrollY = nextScroll;
                    ctx->wheelConsumedThisFrame = true;
                }
            }

            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
                ctx->requestFocus(shared_from_this());

                double now = ctx->platform ? ctx->platform->time() : ctx->time;
                clickCount = (now - lastClickTime < 0.4) ? (clickCount + 1) : 1;
                lastClickTime = now;

                cursorPos = cursorIndexFromPos(ctx->pointer.pos.x - rect.x, ctx->pointer.pos.y - rect.y, pl, pt);
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
                int newPos = cursorIndexFromPos(ctx->pointer.pos.x - rect.x, ctx->pointer.pos.y - rect.y, pl, pt);
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

            if (ctx->modifiers.ctrl || ctx->modifiers.super)
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
                            if (ctx->platform)
                                ctx->platform->setClipboardText(text.substr(s1, s2 - s1));
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
                        std::string clean = ctx->platform
                                                ? Utf8::sanitizeMultiLine(ctx->platform->getClipboardText())
                                                : std::string{};
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
                        if (ctx->modifiers.shift)
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
                ensureLines(innerW, style_.currentStyle);
                if (!textLayout_ || textLayout_->lines.empty())
                    return;

                const size_t curLine = TextEngine::lineIndexForByte(*textLayout_, static_cast<size_t>(cursorPos));
                const int targetLine = std::clamp(
                    static_cast<int>(curLine) + dir, 0, static_cast<int>(textLayout_->lines.size()) - 1);
                if (targetLine == static_cast<int>(curLine))
                    return;

                const float curX = TextEngine::caretXOnLine(textLayout_->lines[curLine], static_cast<size_t>(cursorPos));
                cursorPos = static_cast<int>(
                    TextEngine::hitTestLine(text, textLayout_->lines[static_cast<size_t>(targetLine)], curX, true));
                internal_.pendingTransition = 1;
                keepCursorInView = true;
            };

            auto doAction = [&](int key)
            {
                bool actionChanged = false;
                const bool wordJump = (ctx->modifiers.ctrl || ctx->modifiers.super);
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
                    if (ctx->modifiers.shift)
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
                    if (ctx->modifiers.shift)
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
                    if (ctx->modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    moveVertical(-1);
                    break;
                case Key::Down:
                    if (ctx->modifiers.shift)
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
                    ctx->releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Home:
                {
                    ensureLines(innerW, style_.currentStyle);
                    if (ctx->modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    if (textLayout_ && !textLayout_->lines.empty())
                    {
                        size_t ln = TextEngine::lineIndexForByte(*textLayout_, static_cast<size_t>(cursorPos));
                        cursorPos = static_cast<int>(textLayout_->lines[ln].byteStart);
                    }
                    internal_.pendingTransition = 1;
                    keepCursorInView = true;
                    break;
                }
                case Key::End:
                {
                    ensureLines(innerW, style_.currentStyle);
                    if (ctx->modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    if (textLayout_ && !textLayout_->lines.empty())
                    {
                        size_t ln = TextEngine::lineIndexForByte(*textLayout_, static_cast<size_t>(cursorPos));
                        const auto &line = textLayout_->lines[ln];
                        if (line.hardBreak)
                        {
                            cursorPos = static_cast<int>(line.byteEnd);
                        }
                        else
                        {
                            size_t prev = Utf8::prevCluster(text, line.byteEnd);
                            cursorPos = static_cast<int>(std::max(line.byteStart, prev));
                        }
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
                ensureLines(innerW, style_.currentStyle);
                if (textLayout_ && !textLayout_->lines.empty())
                {
                    const size_t curLine = TextEngine::lineIndexForByte(*textLayout_, static_cast<size_t>(cursorPos));
                    const float curTop = textLayout_->lines[curLine].yOffset;
                    const float curBot = curTop + textLayout_->lines[curLine].height;
                    if (curTop < scrollY)
                        scrollY = curTop;
                    if (curBot > scrollY + innerH)
                        scrollY = curBot - innerH;
                    const float newMax = std::max(0.0f, textLayout_->size.y - innerH);
                    scrollY = std::clamp(scrollY, 0.0f, newMax);
                }
            }
        }

        void renderContent(float op, const ComputedStyle &style) override
        {
            auto *ctx = context();
            auto *r = ctx ? ctx->renderer : nullptr;
            if (!r)
                return;

            const Vec2 vp = viewportSize();
            const TextMeasureSpec spec = makeSpec(style);
            const float fs = spec.fontSize;

            const float bwl = style.borderWidth.left.resolveH(rect.width, rect.height, vp);
            const float bwr = style.borderWidth.right.resolveH(rect.width, rect.height, vp);
            const float bwt = style.borderWidth.top.resolveV(rect.width, rect.height, vp);
            const float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            const float pl = style.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            const float pr = style.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            const float pt = style.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            const float pb = style.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            const float innerW = std::max(0.0f, rect.width - pl - pr - 8.0f);
            const float innerH = std::max(0.0f, rect.height - pt - pb);

            ensureLines(innerW, style);

            const Rect clipRect = {rect.x + pl, rect.y + pt, std::max(0.0f, rect.width - pl - pr), innerH};
            r->pushClip(clipRect);

            const Color textColor = config.enabled ? style.color : Color{120, 120, 125, 255};

            if (text.empty() && !placeholder.empty())
            {
                Style phStyle = partStyle("placeholder");
                const Color phColor = phStyle.color.is_set ? phStyle.color.value : Color{115, 115, 125, 255};
                ensurePlaceholderLines(innerW, style);
                if (placeholderLayout_)
                {
                    ctx->textEngine.draw(*r, *placeholderLayout_,
                                         {rect.x + pl, rect.y + pt, innerW, innerH},
                                         phColor.withAlpha(op),
                                         spec, Align::Start, placeholder, &clipRect);
                }
            }
            else if (textLayout_)
            {
                const bool hasSel = hasFocusCache && selectionStart != -1 && selectionStart != cursorPos;
                if (hasSel)
                {
                    Style selStyle = partStyle("selection");
                    const Color selColor = selStyle.background.is_set ? selStyle.background.value : Color{50, 130, 220, 128};
                    auto selRects = TextEngine::selectionRects(
                        *textLayout_,
                        static_cast<size_t>(std::min(selectionStart, cursorPos)),
                        static_cast<size_t>(std::max(selectionStart, cursorPos)));
                    for (const auto &sr : selRects)
                    {
                        const float ly = rect.y + pt + sr.y - scrollY;
                        if (ly + sr.height < rect.y + pt || ly > rect.y + pt + innerH)
                            continue;
                        r->fillRect({rect.x + pl + sr.x, ly, sr.width, sr.height}, selColor.withAlpha(op));
                    }
                }

                if (!text.empty())
                {
                    ctx->textEngine.draw(*r, *textLayout_,
                                         {rect.x + pl, rect.y + pt - scrollY, innerW, textLayout_->size.y},
                                         textColor.withAlpha(op),
                                         spec, Align::Start, text, &clipRect);
                }
            }

            if (hasFocusCache && config.enabled && cursorBlink < 0.5f &&
                (selectionStart == -1 || selectionStart == cursorPos) && textLayout_)
            {
                Style cursorStyle = partStyle("cursor");
                const Color cursorColor = cursorStyle.color.is_set ? cursorStyle.color.value : textColor;
                const float cursorW = cursorStyle.borderWidth.is_set
                                          ? cursorStyle.borderWidth.value.left.resolveH(fs, fs, vp)
                                          : 2.0f;

                const Rect cr = TextEngine::caretRect(*textLayout_, static_cast<size_t>(cursorPos), cursorW);
                r->fillRect({rect.x + pl + cr.x, rect.y + pt + cr.y - scrollY, cr.width, fs},
                            cursorColor.withAlpha(op));
            }

            // Internal scrollbar when content exceeds visible height.
            const float totalH = textLayout_ ? textLayout_->size.y : 0.0f;
            if (totalH > innerH + 1.0f && innerH > 10.0f)
            {
                const float sbW = 4.0f;
                const float sbX = rect.x + rect.width - bwr - sbW - 3.0f;
                const float ratio = innerH / totalH;
                const float thumbH = std::max(20.0f, innerH * ratio);
                const float maxScroll = totalH - innerH;
                const float scrollFrac = (maxScroll > 0.0f) ? (scrollY / maxScroll) : 0.0f;
                const float thumbY = rect.y + pt + scrollFrac * (innerH - thumbH);
                r->fillRoundedRect({sbX, thumbY, sbW, thumbH}, 2.0f, Color{130, 130, 140, 180}.withAlpha(op));
            }

            r->popClip();
        }

    private:
        TextMeasureSpec makeSpec(const ComputedStyle &style) const
        {
            const Vec2 vp = viewportSize();
            TextMeasureSpec spec;
            spec.fontStack = style.font;
            spec.font = resolveFont(style, defaultFont);
            spec.fontSize = style.fontSize.resolveV(vp.x, vp.y, vp);
            spec.letterSpacing = style.letterSpacing.resolveH(vp.x, vp.y, vp);
            spec.lineHeight = spec.fontSize * 1.4f;
            return spec;
        }

        void ensureLines(float availW, const ComputedStyle &style)
        {
            auto *ctx = context();
            if (!ctx)
                return;

            const TextMeasureSpec spec = makeSpec(style);
            if (textLayout_ && !text_dirty &&
                std::abs(availW - lastWrapW) <= 0.5f &&
                std::abs(spec.fontSize - lastFontSize) <= 0.1f &&
                std::abs(spec.letterSpacing - lastLetterSpacing) <= 0.01f &&
                spec.font.id == lastFontId)
                return;

            text_dirty = false;
            lastWrapW = availW;
            lastFontSize = spec.fontSize;
            lastLetterSpacing = spec.letterSpacing;
            lastFontId = spec.font.id;

            TextLayoutOptions opts;
            opts.maxWidth = availW;
            opts.wrap = (availW > 1.0f);
            opts.preserveSpaces = true;
            textLayout_ = ctx->textEngine.layoutCached(text, spec, opts);
        }

        void ensurePlaceholderLines(float availW, const ComputedStyle &style)
        {
            auto *ctx = context();
            if (!ctx)
                return;

            const TextMeasureSpec spec = makeSpec(style);
            if (placeholderLayout_ && !placeholder_dirty &&
                std::abs(availW - lastPhWrapW) <= 0.5f &&
                std::abs(spec.fontSize - lastPhFontSize) <= 0.1f &&
                std::abs(spec.letterSpacing - lastPhLetterSpacing) <= 0.01f &&
                spec.font.id == lastPhFontId)
                return;

            placeholder_dirty = false;
            lastPhWrapW = availW;
            lastPhFontSize = spec.fontSize;
            lastPhLetterSpacing = spec.letterSpacing;
            lastPhFontId = spec.font.id;

            TextLayoutOptions opts;
            opts.wrap = placeholderWrap_ && (availW > 1.0f);
            opts.maxWidth = opts.wrap ? availW : 0.0f;
            opts.preserveSpaces = true;
            placeholderLayout_ = ctx->textEngine.layoutCached(placeholder, spec, opts);
        }

        int cursorIndexFromPos(float localX, float localY, float pl, float pt) const
        {
            if (!textLayout_ || textLayout_->lines.empty())
                return 0;
            return static_cast<int>(TextEngine::hitTestPoint(
                text, *textLayout_, {localX - pl, localY - pt + scrollY}, Align::Start, 0.0f, true));
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
        float lastPhLetterSpacing{-1.0f};
        uint32_t lastPhFontId{0};
        std::shared_ptr<const TextLayout> placeholderLayout_;

        int rows_{3};
        int cursorPos{0};
        int selectionStart{-1};
        bool text_dirty{true};
        bool hasFocusCache{false};
        float cursorBlink{0.0f};
        float scrollY{0.0f};
        float lastWrapW{-1.0f};
        float lastFontSize{-1.0f};
        float lastLetterSpacing{-1.0f};
        uint32_t lastFontId{0};
        FontHandle defaultFont{0};

        std::shared_ptr<const TextLayout> textLayout_;
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