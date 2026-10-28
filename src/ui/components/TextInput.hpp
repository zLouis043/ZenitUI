#pragma once
#include "Common.hpp"
#include "Layout.hpp"
#include <deque>

namespace ZenitUI::UI
{
    class TextInput : public TLayout<TextInput>
    {
    public:
        TextInput(std::string initial = "") : TLayout<TextInput>(LayoutType::Stack), text(std::move(initial))
        {
            setInteractive(true);
            setFocusable(true);
            setStyleTag("TextInput");
            cursorPos = (int)text.size();
            saveHistory();

            style_.inlineBase.background = Color{30, 30, 36, 255};
            style_.inlineBase.color = Colors::White;
            style_.inlineBase.radius = Px(6.0f);
            style_.inlineBase.padding = Spacing(VH(1.2f), VW(1.5f));
            style_.inlineBase.borderColor = Color{60, 60, 70, 255};
            style_.inlineBase.borderWidth = Px(1.0f);
            internal_.pendingTransition = 1;
        }

        void setText(std::string t)
        {
            text = std::move(t);
            cursorPos = std::min(cursorPos, (int)text.size());
            selectionStart = -1;
            text_dirty = true;
            internal_.pendingTransition = 1;
            saveHistory();
        }
        const std::string &getText() const { return text; }
        void clear() { setText(""); }

        int getCursorPos() const { return cursorPos; }
        void setCursorPos(int pos)
        {
            cursorPos = std::clamp(pos, 0, (int)text.size());
            selectionStart = -1;
            internal_.pendingTransition = 1;
        }

        void setFont(FontHandle f)
        {
            defaultFont = f;
            internal_.pendingTransition = 1;
        }

        std::function<void(const std::string &)> onTextChanged;
        std::function<void(const std::string &)> onSubmit;

    protected:
        Vec2 computeIntrinsicSize(float, float) override
        {
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float charW = 0.0f;
            if (auto r = UIContext::get().renderer)
                charW = r->measureText(f, "M", fs, 0.0f).x;
            if (charW <= 0.0f)
                charW = fs * 0.6f;
            return {std::max(120.0f, charW * 12.0f), fs * 1.6f};
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
            hasFocusCache = UIContext::get().hasFocus(this);
            cursorBlink += dt;
            if (cursorBlink > 1.0f)
                cursorBlink -= 1.0f;

            auto &ctx = UIContext::get();
            auto &ev = ctx.inputEvents;
            bool changed = false;

            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
            float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height);
            float availW = std::max(0.0f, rect.width - pl - pr - 4.0f);

            // --- Multi-click e Drag ---
            if (state.pressed && !hasPointerCapture())
            {
                capturePointer();
                UIContext::get().requestFocus(shared_from_this());

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

            // --- Tasti e Modifiers ---
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
                        std::string clip = ctx.platform->getClipboardText();
                        for (char c : clip)
                        {
                            if (c >= 32 && c <= 126)
                            {
                                text.insert(text.begin() + cursorPos, c);
                                cursorPos++;
                                changed = true;
                            }
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
                for (int c : ev.chars)
                {
                    if (c < 32 || c > 126)
                        continue;
                    deleteSelection();
                    text.insert(text.begin() + cursorPos, (char)c);
                    cursorPos++;
                    changed = true;
                }
                if (ev.chars.size() > 0)
                    saveHistory();
            }

            auto doAction = [&](int key)
            {
                bool actionChanged = false;
                switch (key)
                {
                case Key::Backspace:
                    if (deleteSelection()) {
                        actionChanged = true;
                    } else if (cursorPos > 0) {
                        text.erase(text.begin() + cursorPos - 1);
                        cursorPos--;
                        actionChanged = true;
                    }
                    break;
                case Key::Delete:
                    if (deleteSelection()) {
                        actionChanged = true;
                    } else if (cursorPos < (int)text.size()) {
                        text.erase(text.begin() + cursorPos);
                        actionChanged = true;
                    }
                    break;
                case Key::Left:
                    if (ctx.modifiers.shift) { if (selectionStart == -1) selectionStart = cursorPos; }
                    else selectionStart = -1;
                    if (cursorPos > 0) { cursorPos--; internal_.pendingTransition = 1; }
                    break;
                case Key::Right:
                    if (ctx.modifiers.shift) { if (selectionStart == -1) selectionStart = cursorPos; }
                    else selectionStart = -1;
                    if (cursorPos < (int)text.size()) { cursorPos++; internal_.pendingTransition = 1; }
                    break;
                }
                if (actionChanged) {
                    changed = true;
                    if (key == Key::Backspace || key == Key::Delete) saveHistory();
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
                    UIContext::get().releaseFocus();
                    hasFocusCache = false;
                    repeatStates.clear();
                    return;
                case Key::Escape:
                    UIContext::get().releaseFocus();
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

            // --- Scroll orizzontale automatico ---
            {
                auto r = UIContext::get().renderer;
                if (!r)
                    return;
                FontHandle f = resolveFont(style_.currentStyle, defaultFont);
                float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
                float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);

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

        void renderContent(float op, const ComputedStyle &style) override
        {
            auto r = UIContext::get().renderer;
            FontHandle f = resolveFont(style, defaultFont);
            float fs = style.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);
            float pl = style.padding.left.resolveH(rect.width, rect.height);
            float pr = style.padding.right.resolveH(rect.width, rect.height);
            float pt = style.padding.top.resolveV(rect.width, rect.height);
            float pb = style.padding.bottom.resolveV(rect.width, rect.height);

            Rect innerRect = {
                rect.x + pl,
                rect.y + pt,
                std::max(0.0f, rect.width - pl - pr),
                std::max(0.0f, rect.height - pt - pb)};

            r->pushClip(innerRect);

            Vec2 pos = {rect.x + pl - scrollX, rect.y + (rect.height - fs) * 0.5f - fs * 0.1f};

            if (hasFocusCache && selectionStart != -1 && selectionStart != cursorPos)
            {
                int s1 = std::min(selectionStart, cursorPos);
                int s2 = std::max(selectionStart, cursorPos);
                std::string pre = text.substr(0, s1);
                std::string sel = text.substr(s1, s2 - s1);
                float preW = r->measureText(f, pre, fs, sp).x;
                float selW = r->measureText(f, sel, fs, sp).x;

                Color selColor = Color{50, 130, 220, 128};
                r->fillRect({pos.x + preW, pos.y, selW, fs * 1.2f}, selColor.withAlpha(op));
            }

            Color textColor = config.enabled ? style.color : Color{120, 120, 125, 255};
            r->drawText(f, text, pos, fs, sp, textColor.withAlpha(op));

            if (hasFocusCache && config.enabled && cursorBlink < 0.5f && (selectionStart == -1 || selectionStart == cursorPos))
            {
                Style cursorStyle = partStyle("cursor");
                Color cursorColor = cursorStyle.color.is_set ? cursorStyle.color.value : textColor;
                float cursorW = cursorStyle.borderWidth.is_set ? cursorStyle.borderWidth.value.resolveH(fs, fs) : 2.0f;
                float cx = pos.x + r->measureText(f, text.substr(0, cursorPos), fs, sp).x;
                r->fillRect({cx, pos.y, cursorW, fs}, cursorColor.withAlpha(op));
            }

            r->popClip();
        }

    private:
        int cursorIndexFromX(float localX)
        {
            auto r = UIContext::get().renderer;
            if (!r)
                return cursorPos;
            FontHandle f = resolveFont(style_.currentStyle, defaultFont);
            float fs = style_.currentStyle.fontSize.resolveV(Metrics::viewport.x, Metrics::viewport.y);
            float sp = style_.currentStyle.letterSpacing.resolveH(Metrics::viewport.x, Metrics::viewport.y);
            float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height);
            float targetX = localX - pl + scrollX;

            int best = 0;
            float bestDist = std::abs(targetX);
            for (int i = 1; i <= (int)text.size(); ++i)
            {
                std::string prefix = text.substr(0, i);
                float w = r->measureText(f, prefix, fs, sp).x;
                float dist = std::abs(targetX - w);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = i;
                }
            }
            return best;
        }

        // ---- UNDO / REDO SYSTEM ----
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
            int s = cursorPos, e = cursorPos;
            while (s > 0 && std::isalnum(text[s - 1]) && text[s - 1] != ' ')
                s--;
            while (e < (int)text.size() && std::isalnum(text[e]) && text[e] != ' ')
                e++;
            if (s != e)
            {
                selectionStart = s;
                cursorPos = e;
            }
        }

        std::string text;
        int cursorPos{0};
        int selectionStart{-1};
        bool text_dirty{true};
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
}