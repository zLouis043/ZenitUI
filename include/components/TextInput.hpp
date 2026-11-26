#pragma once

/**
 * \file TextInput.hpp
 * \brief A single-line UTF-8 editable text field backed by `UIContext::textEngine`.
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
     * Style tag: `"TextInput"`. Focusable, interactive. Parts: `::selection`,
     * `::cursor`, `::placeholder`.
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

        /** \brief Replace the content, snap cursor to a valid UTF-8 boundary, clear selection, and save undo history. */
        void setText(std::string t)
        {
            text = std::move(t);
            cursorPos = static_cast<int>(
                Utf8::snapFloor(text, std::min<size_t>(static_cast<size_t>(std::max(0, cursorPos)), text.size())));
            selectionStart = -1;
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
        Vec2 computeIntrinsicSize(float availW, float) override
        {
            auto *ctx = context();
            if (!ctx)
                return {0.0f, 0.0f};

            TextMeasureSpec mSpec = makeSpec(style_.currentStyle);
            mSpec.letterSpacing = 0.0f;
            float charW = ctx->textEngine.measureText("M", mSpec).x;
            if (charW <= 0.0f)
                charW = mSpec.fontSize * 0.6f;

            float w = std::max(120.0f, charW * 12.0f);
            float h = mSpec.fontSize * 1.6f;

            if (placeholderWrap && text.empty() && !placeholder.empty() && availW > 1.0f)
            {
                TextMeasureSpec phSpec = makeSpec(style_.currentStyle, mSpec.fontSize * 1.35f);
                TextLayoutOptions opts;
                opts.wrap = true;
                opts.maxWidth = availW;
                auto phLo = ctx->textEngine.layoutCached(placeholder, phSpec, opts);
                h = std::max(h, phLo->size.y);
            }

            return {w, h};
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

            float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height, vp);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            float availW = std::max(0.0f, rect.width - pl - pr - 4.0f);

            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
                ctx->requestFocus(shared_from_this());

                double now = ctx->platform ? ctx->platform->time() : ctx->time;
                if (now - lastClickTime < 0.4)
                    clickCount++;
                else
                    clickCount = 1;
                lastClickTime = now;

                cursorPos = cursorIndexFromX(ctx->pointer.pos.x - rect.x);
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
                int newPos = cursorIndexFromX(ctx->pointer.pos.x - rect.x);
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
                            }
                        }
                    }
                    else if (k == Key::V)
                    {
                        deleteSelection();
                        std::string clean = ctx->platform
                                                ? Utf8::sanitizeSingleLine(ctx->platform->getClipboardText())
                                                : std::string{};
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
                        if (ctx->modifiers.shift)
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
                const bool wordJump = (ctx->modifiers.ctrl || ctx->modifiers.super);
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
                    ctx->releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Escape:
                    ctx->releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Home:
                    if (ctx->modifiers.shift)
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
                    if (ctx->modifiers.shift)
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

            if (ctx->renderer)
            {
                const TextMeasureSpec spec = makeSpec(style_.currentStyle);
                auto lo = ctx->textEngine.layoutCached(text, spec);
                const float prefixW = (!lo->lines.empty())
                                          ? TextEngine::caretXOnLine(lo->lines[0], static_cast<size_t>(cursorPos))
                                          : 0.0f;
                const float fullW = lo->size.x;

                if (prefixW - scrollX > availW)
                    scrollX = prefixW - availW;
                if (prefixW - scrollX < 0.0f)
                    scrollX = prefixW;
                const float maxScroll = std::max(0.0f, fullW - availW);
                scrollX = std::clamp(scrollX, 0.0f, maxScroll);
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

            const float innerW = std::max(0.0f, rect.width - pl - pr);
            const float innerH = std::max(0.0f, rect.height - pt - pb);

            const Rect innerRect = {rect.x + pl, rect.y + pt, innerW, innerH};
            r->pushClip(innerRect);

            const Vec2 pos = {rect.x + pl - scrollX, rect.y + pt + (innerH - fs) * 0.5f - fs * 0.1f};
            auto lo = ctx->textEngine.layoutCached(text, spec);

            if (hasFocusCache && selectionStart != -1 && selectionStart != cursorPos && !lo->lines.empty())
            {
                const size_t s1 = static_cast<size_t>(std::min(selectionStart, cursorPos));
                const size_t s2 = static_cast<size_t>(std::max(selectionStart, cursorPos));
                const float preW = TextEngine::caretXOnLine(lo->lines[0], s1);
                const float selW = TextEngine::caretXOnLine(lo->lines[0], s2) - preW;

                Style selStyle = partStyle("selection");
                Color selColor = selStyle.background.is_set ? selStyle.background.value : Color{50, 130, 220, 128};

                r->fillRect({pos.x + preW, pos.y, selW, fs * 1.2f}, selColor.withAlpha(op));
            }

            const Color textColor = config.enabled ? style.color : Color{120, 120, 125, 255};
            if (text.empty() && !placeholder.empty())
            {
                Style phStyle = partStyle("placeholder");
                const Color phColor = phStyle.color.is_set ? phStyle.color.value : Color{115, 115, 125, 255};

                if (placeholderWrap && innerW > 1.0f)
                {
                    TextMeasureSpec phSpec = spec;
                    phSpec.lineHeight = fs * 1.35f;
                    TextLayoutOptions phOpts;
                    phOpts.wrap = true;
                    phOpts.maxWidth = innerW;
                    auto phLo = ctx->textEngine.layoutCached(placeholder, phSpec, phOpts);
                    const float startY = rect.y + pt + std::max(0.0f, (innerH - phLo->size.y) * 0.5f);
                    ctx->textEngine.draw(*r, *phLo,
                                         {rect.x + pl, startY, innerW, innerH},
                                         phColor.withAlpha(op),
                                         phSpec, Align::Start, placeholder, &innerRect);
                }
                else
                {
                    TextLayoutOptions phOpts;
                    phOpts.wrap = false;
                    phOpts.ellipsis = (innerW > 1.0f);
                    phOpts.maxWidth = innerW;
                    phOpts.ellipsisText = "...";
                    auto phLo = ctx->textEngine.layoutCached(placeholder, spec, phOpts);
                    ctx->textEngine.draw(*r, *phLo,
                                         {pos.x, pos.y, innerW, fs},
                                         phColor.withAlpha(op),
                                         spec, Align::Start, placeholder);
                }
            }
            else if (!text.empty())
            {
                ctx->textEngine.draw(*r, *lo,
                                     {pos.x, pos.y, std::max(innerW, lo->size.x), fs},
                                     textColor.withAlpha(op),
                                     spec, Align::Start, text);
            }

            if (hasFocusCache && config.enabled && cursorBlink < 0.5f &&
                (selectionStart == -1 || selectionStart == cursorPos))
            {
                Style cursorStyle = partStyle("cursor");
                const Color cursorColor = cursorStyle.color.is_set ? cursorStyle.color.value : textColor;
                const float cursorW = cursorStyle.borderWidth.is_set
                                          ? cursorStyle.borderWidth.value.left.resolveH(fs, fs, vp)
                                          : 2.0f;
                const float cx = pos.x + (!lo->lines.empty()
                                              ? TextEngine::caretXOnLine(lo->lines[0], static_cast<size_t>(cursorPos))
                                              : 0.0f);
                r->fillRect({cx, pos.y, cursorW, fs}, cursorColor.withAlpha(op));
            }

            r->popClip();
        }

    private:
        TextMeasureSpec makeSpec(const ComputedStyle &style, float lineHeight = 0.0f) const
        {
            const Vec2 vp = viewportSize();
            TextMeasureSpec spec;
            spec.fontStack = style.font;
            spec.font = resolveFont(style, defaultFont);
            spec.fontSize = style.fontSize.resolveV(vp.x, vp.y, vp);
            spec.letterSpacing = style.letterSpacing.resolveH(vp.x, vp.y, vp);
            spec.lineHeight = lineHeight;
            return spec;
        }

        int cursorIndexFromX(float localX)
        {
            auto *ctx = context();
            if (!ctx || !ctx->renderer)
                return cursorPos;

            const Vec2 vp = viewportSize();
            const float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            const float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            const float targetX = localX - pl + scrollX;

            const TextMeasureSpec spec = makeSpec(style_.currentStyle);
            auto lo = ctx->textEngine.layoutCached(text, spec);
            if (lo->lines.empty())
                return 0;
            return static_cast<int>(TextEngine::hitTestLine(text, lo->lines[0], targetX, false));
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
        bool placeholderWrap{false};
        int cursorPos{0};
        int selectionStart{-1};
        bool hasFocusCache{false};
        float cursorBlink{0.0f};
        float scrollX{0.0f};
        FontHandle defaultFont{0};

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
}