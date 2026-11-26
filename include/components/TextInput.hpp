#pragma once

/**
 * \file TextInput.hpp
 * \brief A single-line editable text field.
 *
 * The input is a `Stack` with a single text cursor (no child `Text` node).
 * All visual is drawn directly by `renderContent`: the text, the cursor
 * caret (blinking at 1 Hz), and the selection highlight.
 *
 * Editing model:
 *  - The text is stored as a `std::string`; the cursor is a byte offset
 *    (not a grapheme index). This is simple and correct for ASCII, but
 *    multi-byte UTF-8 codepoints are treated as multiple "characters" —
 *    arrows move by byte, backspace deletes one byte. Not Unicode-aware.
 *  - Only printable ASCII (32..126) can be inserted; pasted text is
 *    filtered to that range.
 *
 * Interaction:
 *  - Click to place the cursor; double-click selects a word; triple-click
 *    selects all.
 *  - Drag to extend the selection.
 *  - Shift+arrows extend the selection; Ctrl+A selects all.
 *  - Ctrl+C/X/V: copy / cut / paste via `IPlatform` clipboard.
 *  - Ctrl+Z/Y: undo / redo (50-step history).
 *  - Enter: submits and releases focus; Escape: releases focus.
 *  - Home / End: jump to start / end; Shift extends.
 *
 * \note This component dereferences `UIContext::platform` in `onUpdate`
 *       (for `time()` and clipboard access) without a null check. If you
 *       use `TextInput` in a headless test or without a platform backend,
 *       you must set `context().platform` to a valid mock, or the
 *       first `onUpdate` after the field is focused will crash.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Utf8.hpp"
#include <deque>

namespace ZenitUI::UI
{
    /** \addtogroup grp_widgets
     *  @{
     */

    /**
     * \class TextInput
     * \brief A single-line text editor.
     *
     * Style tag: `"TextInput"`. Focusable, interactive. Parts: `::selection`
     * (selection background), `::cursor` (caret colour and width — the width
     * comes from `borderWidth.left` if set).
     *
     * The intrinsic width is sized to roughly 12 "M" glyphs at the current
     * font size, and the intrinsic height to `1.6 * fontSize`.
     */
    class TextInput : public TLayout<TextInput>
    {
    public:
        /**
         * \brief Construct with initial text.
         * \param initial Initial content. The cursor is placed at the end.
         */
        TextInput(std::string initial = "") : TLayout<TextInput>(LayoutType::Stack), text(std::move(initial))
        {
            setInteractive(true);
            setFocusable(true);
            setStyleTag("TextInput");
            cursorPos = (int)text.size();
            saveHistory();

            style_.inlineDefaults.background = Color{30, 30, 36, 255};
            style_.inlineDefaults.color = Colors::White;
            style_.inlineDefaults.radius = Px(6.0f);
            style_.inlineDefaults.padding = Spacing(VH(1.2f), VW(1.5f));
            style_.inlineDefaults.borderColor = Color{60, 60, 70, 255};
            style_.inlineDefaults.borderWidth = Px(1.0f);
            internal_.pendingTransition = 1;
        }

        /**
         * \brief Replace the content and move the cursor to the end.
         *
         * Clamps the previous cursor position to the new size if it was past
         * the end. Clears the selection. Marks the style as pending and
         * pushes the change to the undo history.
         */
        void setText(std::string t)
        {
            text = std::move(t);
            cursorPos = static_cast<int>(
                Utf8::snapFloor(text, std::min<size_t>(static_cast<size_t>(std::max(0, cursorPos)), text.size())));
            selectionStart = -1;
            text_dirty = true;
            internal_.pendingTransition = 1;
            saveHistory();
        }
        /** \brief Current content. */
        const std::string &getText() const { return text; }
        /** \brief Reset to empty content. */
        void clear() { setText(""); }

        /** \brief Placeholder shown when body is empty. */
        void setPlaceholder(std::string p)
        {
            placeholder = std::move(p);
            internal_.pendingTransition = 1;
        }
        /** \brief Current placeholder text. */
        const std::string &getPlaceholder() const { return placeholder; }
        /** \brief Enable or disable multi-line wrapping for the placeholder. */
        void setPlaceholderWrap(bool w)
        {
            placeholderWrap = w;
            internal_.pendingTransition = 1;
        }
        /** \brief `true` if placeholder wrapping is enabled. */
        bool getPlaceholderWrap() const { return placeholderWrap; }

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

        /** \brief Override the font used for measurement and rendering. */
        void setFont(FontHandle f)
        {
            defaultFont = f;
            internal_.pendingTransition = 1;
        }

        /** \brief Fired on every content change (typing, paste, delete, cut). */
        std::function<void(const std::string &)> onTextChanged;
        /** \brief Fired on Enter; the field loses focus immediately after. */
        std::function<void(const std::string &)> onSubmit;

    protected:
        /**
         * \brief Intrinsic size: roughly 12 glyphs wide, `1.6 * fontSize` tall.
         *
         * Measures the `"M"` glyph to estimate the average character width.
         * Falls back to `0.6 * fontSize` if the renderer is not available
         * (headless tests).
         */
        Vec2 computeIntrinsicSize(float availW, float) override
        {
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            const Vec2 vp = viewportSize();
            float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
            float charW = 0.0f;
            if (auto r = context().renderer)
                charW = r->measureText(f, "M", fs, 0.0f).x;
            if (charW <= 0.0f)
                charW = fs * 0.6f;

            float w = std::max(120.0f, charW * 12.0f);
            float h = fs * 1.6f;

            if (placeholderWrap && text.empty() && !placeholder.empty() && availW > 1.0f)
            {
                float sp = style_.currentStyle.letterSpacing.resolveH(vp.x, vp.y, vp);
                auto lines = wrapPlaceholderLines(placeholder, availW, f, fs, sp);
                float wrappedH = static_cast<float>(lines.size()) * (fs * 1.35f);
                h = std::max(h, wrappedH);
            }

            return {w, h};
        }

        /**
         * \brief Reset editing state when the field is disabled.
         *
         * Clears focus cache, repeat state, cursor blink, and selection. Does
         * not clear the text.
         */
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

        /**
         * \brief The main per-frame edit loop.
         *
         * Sequence:
         *  -# Refresh `hasFocusCache` and advance the cursor blink timer.
         *  -# Handle pointer down: capture pointer, request focus, detect
         *     double/triple click, set cursor from x position, start
         *     selection.
         *  -# Handle pointer drag while captured (`clickCount == 1`): move
         *     cursor to follow pointer.
         *  -# If not focused, clear repeat state and return.
         *  -# Handle Ctrl/Cmd chords (A, C, X, V, Z, Y).
         *  -# Handle typed UTF-8 codepoints (`Utf8::isPrintable`).
         *  -# Handle key repeat for Backspace, Delete, Left, Right (plus Ctrl word jumps).
         *  -# Handle Enter (submit + blur) and Escape (blur), Home, End.
         *  -# If content changed, fire `onTextChanged`.
         *  -# Update horizontal scroll so the cursor stays visible.
         */
        void onUpdate(float dt) override
        {
            hasFocusCache = context().hasFocus(this);
            cursorBlink += dt;
            if (cursorBlink > 1.0f)
                cursorBlink -= 1.0f;

            auto &ctx = context();
            auto &ev = ctx.inputEvents;
            const Vec2 vp = viewportSize();
            bool changed = false;

            float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height, vp);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            float availW = std::max(0.0f, rect.width - pl - pr - 4.0f);

            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
                context().requestFocus(shared_from_this());

                double now = ctx.platform->time();
                if (now - lastClickTime < 0.4)
                {
                    clickCount++;
                }
                else
                {
                    clickCount = 1;
                }
                lastClickTime = now;

                cursorPos = cursorIndexFromX(ctx.pointer.pos.x - rect.x);
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
                int newPos = cursorIndexFromX(ctx.pointer.pos.x - rect.x);
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
                    changed = true;
                    return true;
                }
                selectionStart = -1;
                return false;
            };

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
                            }
                        }
                    }
                    else if (k == Key::V)
                    {
                        deleteSelection();
                        std::string clean = Utf8::sanitizeSingleLine(ctx.platform->getClipboardText());
                        if (!clean.empty())
                        {
                            text.insert(static_cast<size_t>(cursorPos), clean);
                            cursorPos += static_cast<int>(clean.size());
                            changed = true;
                        }
                        if (changed)
                            saveHistory();
                    }
                    else if (k == Key::Z)
                    {
                        if (ctx.modifiers.shift)
                            redo();
                        else
                            undo();
                    }
                    else if (k == Key::Y)
                    {
                        redo();
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
                    changed = true;
                    inserted = true;
                }
                if (inserted)
                    saveHistory();
            }
            auto doAction = [&](int key)
            {
                bool actionChanged = false;
                const bool wordJump = (ctx.modifiers.ctrl || ctx.modifiers.super);
                switch (key)
                {
                case Key::Backspace:
                    if (deleteSelection())
                    {
                        actionChanged = true;
                    }
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
                    {
                        actionChanged = true;
                    }
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
                }
                if (actionChanged)
                {
                    changed = true;
                    if (key == Key::Backspace || key == Key::Delete)
                        saveHistory();
                }
                cursorBlink = 0.0f;
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

            for (int k : ev.keys)
            {
                switch (k)
                {
                case Key::Enter:
                    if (onSubmit)
                        onSubmit(text);
                    context().releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Escape:
                    context().releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Home:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    cursorPos = 0;
                    internal_.pendingTransition = 1;
                    break;
                case Key::End:
                    if (ctx.modifiers.shift)
                    {
                        if (selectionStart == -1)
                            selectionStart = cursorPos;
                    }
                    else
                        selectionStart = -1;
                    cursorPos = (int)text.size();
                    internal_.pendingTransition = 1;
                    break;
                default:
                    break;
                }
            }

            if (changed && onTextChanged)
            {
                onTextChanged(text);
                internal_.pendingTransition = 1;
            }

            {
                auto r = context().renderer;
                const Vec2 vp = viewportSize();
                if (!r)
                    return;
                FontHandle f = resolveFont(style_.currentStyle, defaultFont);
                float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
                float sp = style_.currentStyle.letterSpacing.resolveH(vp.x, vp.y, vp);

                std::string prefix = text.substr(0, cursorPos);
                Vec2 prefixSize = r->measureText(f, prefix, fs, sp);
                Vec2 fullSize = r->measureText(f, text, fs, sp);

                if (prefixSize.x - scrollX > availW)
                    scrollX = prefixSize.x - availW;
                if (prefixSize.x - scrollX < 0.0f)
                    scrollX = prefixSize.x;
                float maxScroll = std::max(0.0f, fullSize.x - availW);
                scrollX = std::clamp(scrollX, 0.0f, maxScroll);
            }
        }

        /**
         * \brief Draw selection, text, and cursor.
         *
         * The content is clipped to the inner rect (so overflowing text
         * disappears rather than spilling out). Selection is drawn first,
         * then the text, then the caret on top. The caret is hidden when
         * there is an active selection (`selectionStart != cursorPos`) — this
         * matches most native widgets.
         */
        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = context().renderer;
            const Vec2 vp = viewportSize();
            FontHandle f = resolveFont(style, defaultFont);
            float fs = style.fontSize.resolveV(vp.x, vp.y, vp);
            float sp = style.letterSpacing.resolveH(vp.x, vp.y, vp);
            float bwl = style.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float bwr = style.borderWidth.right.resolveH(rect.width, rect.height, vp);
            float bwt = style.borderWidth.top.resolveV(rect.width, rect.height, vp);
            float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            float pl = style.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float pr = style.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            float pt = style.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            float pb = style.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            float innerW = std::max(0.0f, rect.width - pl - pr);
            float innerH = std::max(0.0f, rect.height - pt - pb);

            Rect innerRect = {rect.x + pl, rect.y + pt, innerW, innerH};
            r->pushClip(innerRect);

            Vec2 pos = {rect.x + pl - scrollX, rect.y + pt + (innerH - fs) * 0.5f - fs * 0.1f};

            if (hasFocusCache && selectionStart != -1 && selectionStart != cursorPos)
            {
                int s1 = std::min(selectionStart, cursorPos);
                int s2 = std::max(selectionStart, cursorPos);
                std::string pre = text.substr(0, s1);
                std::string sel = text.substr(s1, s2 - s1);
                float preW = r->measureText(f, pre, fs, sp).x;
                float selW = r->measureText(f, sel, fs, sp).x;

                Style selStyle = partStyle("selection");
                Color selColor = selStyle.background.is_set ? selStyle.background.value : Color{50, 130, 220, 128};

                r->fillRect({pos.x + preW, pos.y, selW, fs * 1.2f}, selColor.withAlpha(op));
            }

            Color textColor = config.enabled ? style.color : Color{120, 120, 125, 255};
            if (text.empty() && !placeholder.empty())
            {
                Style phStyle = partStyle("placeholder");
                Color phColor = phStyle.color.is_set ? phStyle.color.value : Color{115, 115, 125, 255};

                float phPl = style.padding.left.resolveH(rect.width, rect.height, vp) +
                             style.borderWidth.left.resolveH(rect.width, rect.height, vp);
                float phPr = style.padding.right.resolveH(rect.width, rect.height, vp) +
                             style.borderWidth.right.resolveH(rect.width, rect.height, vp);
                float phPt = style.padding.top.resolveV(rect.width, rect.height, vp) +
                             style.borderWidth.top.resolveV(rect.width, rect.height, vp);
                float phPb = style.padding.bottom.resolveV(rect.width, rect.height, vp) +
                             style.borderWidth.bottom.resolveV(rect.width, rect.height, vp);
                float phInnerW = std::max(0.0f, rect.width - phPl - phPr);
                float phInnerH = std::max(0.0f, rect.height - phPt - phPb);

                if (placeholderWrap && phInnerW > 1.0f)
                {
                    auto lines = wrapPlaceholderLines(placeholder, phInnerW, f, fs, sp);
                    float lineH = fs * 1.35f;
                    float totalH = (float)lines.size() * lineH;
                    float startY = rect.y + phPt + std::max(0.0f, (phInnerH - totalH) * 0.5f);
                    for (size_t i = 0; i < lines.size(); ++i)
                    {
                        float ly = startY + (float)i * lineH;
                        if (ly > rect.y + phPt + phInnerH)
                            break;
                        r->drawText(f, lines[i], {rect.x + phPl, ly}, fs, sp, phColor.withAlpha(op));
                    }
                }
                else
                {
                    std::string disp = truncatePlaceholder(placeholder, phInnerW, f, fs, sp);
                    r->drawText(f, disp, pos, fs, sp, phColor.withAlpha(op));
                }
            }
            else
            {
                r->drawText(f, text, pos, fs, sp, textColor.withAlpha(op));
            }

            if (hasFocusCache && config.enabled && cursorBlink < 0.5f && (selectionStart == -1 || selectionStart == cursorPos))
            {
                Style cursorStyle = partStyle("cursor");
                Color cursorColor = cursorStyle.color.is_set ? cursorStyle.color.value : textColor;
                float cursorW = cursorStyle.borderWidth.is_set
                                    ? cursorStyle.borderWidth.value.left.resolveH(fs, fs, vp)
                                    : 2.0f;
                float cx = pos.x + r->measureText(f, text.substr(0, cursorPos), fs, sp).x;
                r->fillRect({cx, pos.y, cursorW, fs}, cursorColor.withAlpha(op));
            }

            r->popClip();
        }

    private:
        /**
         * \brief Map a local x coordinate (relative to the node's left edge)
         *        to the closest cursor byte index.
         *
         * O(n²) in the text length: for each candidate cursor position, it
         * measures the prefix. Fine for short fields, slow for very long
         * strings. A linear-time implementation would precompute the
         * cumulative glyph widths.
         */
        int cursorIndexFromX(float localX)
        {
            auto r = context().renderer;
            if (!r)
                return cursorPos;
            const Vec2 vp = viewportSize();
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            float fs = style_.currentStyle.fontSize.resolveV(vp.x, vp.y, vp);
            float sp = style_.currentStyle.letterSpacing.resolveH(vp.x, vp.y, vp);
            float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float targetX = localX - pl + scrollX;

            auto bounds = Utf8::clusterBoundaries(text);
            int best = 0;
            float bestDist = std::abs(targetX);
            for (size_t bi = 1; bi < bounds.size(); ++bi)
            {
                size_t bytePos = bounds[bi];
                float w = r->measureText(f, std::string_view(text).substr(0, bytePos), fs, sp).x;
                float dist = std::abs(targetX - w);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = static_cast<int>(bytePos);
                }
            }
            return best;
        }

        /** \brief One snapshot of (text, cursor) for undo/redo. */
        struct HistoryState
        {
            std::string text;
            int cursorPos;
        };
        std::deque<HistoryState> undoStack;
        std::deque<HistoryState> redoStack;

        /**
         * \brief Push the current state onto the undo stack.
         *
         * No-op if the top of the undo stack already has the same text (so
         * typing many characters one at a time does not push dozens of
         * entries — wait, it does: each char is a change, each change is a
         * save. The dedup only helps for "setText called with same content").
         *
         * Caps the stack at 50 entries; older entries are discarded.
         * Clears the redo stack.
         */
        void saveHistory()
        {
            if (!undoStack.empty() && undoStack.back().text == text)
                return;
            undoStack.push_back({text, cursorPos});
            if (undoStack.size() > 50)
                undoStack.pop_front();
            redoStack.clear();
        }

        /** \brief Restore the previous state. No-op if already at the oldest. */
        void undo()
        {
            if (undoStack.size() > 1)
            {
                redoStack.push_back(undoStack.back());
                undoStack.pop_back();
                text = undoStack.back().text;
                cursorPos = undoStack.back().cursorPos;
                selectionStart = -1;
                internal_.pendingTransition = 1;
            }
        }

        /** \brief Re-apply the last undone state. No-op if the redo stack is empty. */
        void redo()
        {
            if (!redoStack.empty())
            {
                undoStack.push_back(redoStack.back());
                text = redoStack.back().text;
                cursorPos = redoStack.back().cursorPos;
                redoStack.pop_back();
                selectionStart = -1;
                internal_.pendingTransition = 1;
            }
        }

        /**
         * \brief Extend the selection to the word surrounding the cursor.
         *
         * "Word" is a run of characters for which `std::isalnum` is true and
         * which is not a space. Note that `isalnum` already returns false for
         * space, so `&& text[...] != ' '` is redundant. Underscores are
         * treated as word breaks (they are not alphanumeric).
         */
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

        std::vector<std::string_view> wrapPlaceholderLines(const std::string &src, float availW,
                                                                  FontHandle f, float fs, float sp)
        {
            std::vector<std::string_view> out;
            auto r = context().renderer;
            if (!r || availW <= 1.0f || src.empty())
            {
                if (!src.empty())
                    out.emplace_back(src);
                return out;
            }

            auto measure = [&](size_t a, size_t b) -> float
            {
                if (b <= a)
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

                if (pStart == pEnd)
                {
                    out.emplace_back();
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
                            out.emplace_back(std::string_view(src).substr(lineStart, pEnd - lineStart));
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

                        size_t visEnd = pStart + bounds[breakIdx];
                        while (visEnd > lineStart && src[visEnd - 1] == ' ')
                            --visEnd;
                        out.emplace_back(std::string_view(src).substr(lineStart, visEnd - lineStart));
                        lineStartIdx = breakIdx;
                    }
                }

                if (pEnd >= len)
                    break;
                pStart = pEnd + 1;
            }
            return out;
        }

        std::string truncatePlaceholder(const std::string &src, float maxW,
                                               FontHandle f, float fs, float sp)
        {
            auto r = context().renderer;
            if (!r || maxW <= 1.0f || src.empty())
                return src;
            if (r->measureText(f, src, fs, sp).x <= maxW)
                return src;

            const std::string ell = "...";
            float ellW = r->measureText(f, ell, fs, sp).x;
            if (ellW >= maxW)
                return "";

            auto bounds = Utf8::clusterBoundaries(src);
            size_t lo = 0, hi = bounds.size() - 1;
            while (lo < hi)
            {
                size_t mid = lo + (hi - lo + 1) / 2;
                if (r->measureText(f, std::string_view(src).substr(0, bounds[mid]), fs, sp).x + ellW <= maxW)
                    lo = mid;
                else
                    hi = mid - 1;
            }
            size_t cut = bounds[lo];
            while (cut > 0 && src[cut - 1] == ' ')
                --cut;
            return src.substr(0, cut) + ell;
        }

        std::string text;
        std::string placeholder;
        bool placeholderWrap{false};
        int cursorPos{0};
        int selectionStart{-1};
        bool text_dirty{true};
        bool hasFocusCache{false};
        float cursorBlink{0.0f};
        float scrollX{0.0f};
        FontHandle defaultFont{0};

        int clickCount{0};
        double lastClickTime{0.0};

        /** \brief Repeat state for held keys (Backspace, Delete, Left, Right). */
        struct RepeatState
        {
            float holdTime{0.0f};
            float nextFireTime{0.0f};
        };
        std::unordered_map<int, RepeatState> repeatStates;
    };
    /** @} */
}