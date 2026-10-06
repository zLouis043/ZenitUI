#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"
#include "Button.hpp"

namespace ZenitUI::UI {

class Popup : public TLayout<Popup, Panel>
{
public:
    Popup() : TLayout<Popup, Panel>() {}

    void setContent(std::shared_ptr<Layout> content)
    {
        content_ = std::move(content);
        if (content_)
            addChild(content_);
    }

    void openBelow(std::shared_ptr<Layout> anchor)
    {
        anchor_ = anchor.get();
        pos_ = {0.0f, 0.0f};
        openInternal();
    }

    void openAbove(std::shared_ptr<Layout> anchor)
    {
        anchor_ = anchor.get();
        above_ = true;
        openInternal();
    }

    void openAt(Vec2 screenPos)
    {
        anchor_ = nullptr;
        pos_ = screenPos;
        above_ = false;
        openInternal();
    }

    void close()
    {
        if (!isOpen_)
            return;
        isOpen_ = false;
        anchor_ = nullptr;
        style_.inlineBase.opacity = 0.0f;
        beginTransition();
        setEnabled(false);
        if (onClose)
            onClose();
    }

    bool isOpen() const { return isOpen_; }

    bool closeOnClickOutside{true};
    bool closeOnEscape{true};
    float offsetBelow{4.0f};
    std::function<void()> onClose;

    void arrange(Rect) override { /* no-op */ }

protected:
    void onBuild() override
    {
        setStyleTag("Popup");
        setPortal(true);
        setInteractive(true);
        setFocusable(true);
        setBlocksRaycast(true);
        style_.inlineBase.position = Position::Absolute;
        style_.inlineBase.opacity = 0.0f;
        isOpen_ = false;
        setEnabled(false);
    }

    void onUpdate(float) override
    {
        if (!isOpen_)
            return;
        auto &ctx = UIContext::get();

        Vec2 size = computePopupSize();
        float x = pos_.x, y = pos_.y;
        if (anchor_)
        {
            Rect a = anchor_->getRect();
            x = a.x;

            if (above_)
            {
                y = a.y - size.y - offsetBelow;
                if (y < 0.0f)
                    y = a.y + a.height + offsetBelow;
            }
            else
            {
                y = a.y + a.height + offsetBelow;
                if (y + size.y > Metrics::viewport.y)
                    y = a.y - size.y - offsetBelow;
            }
        }

        if (x < 0.0f)
            x = 0.0f;
        if (y < 0.0f)
            y = 0.0f;
        if (x + size.x > Metrics::viewport.x)
            x = Metrics::viewport.x - size.x;
        if (y + size.y > Metrics::viewport.y)
            y = Metrics::viewport.y - size.y;

        rect = {x, y, size.x, size.y};
        arrangeInto(rect);

        if (closeOnClickOutside && ctx.pointer.pressed)
        {
            if (!rect.contains(ctx.pointer.pos))
                close();
        }

        if (closeOnClickOutside && ctx.pointer.rightPressed)
        {
            if (!rect.contains(ctx.pointer.pos))
                close();
        }

        if (closeOnEscape)
        {
            for (int k : ctx.inputEvents.keys)
            {
                if (k == Key::Escape)
                {
                    close();
                    return;
                }
            }
        }
    }

private:
    std::shared_ptr<Layout> content_;
    Layout *anchor_{nullptr};
    Vec2 pos_{0.0f, 0.0f};
    bool isOpen_{false};
    bool above_{false};

    void openInternal()
    {
        isOpen_ = true;
        above_ = (above_ && anchor_);
        setEnabled(true);
        style_.inlineBase.opacity = 1.0f;
        beginTransition();
        if (content_)
            content_->beginTransition();
        UIContext::get().requestFocus(shared_from_this());
    }

    Vec2 computePopupSize()
    {
        if (!content_)
            return {100.0f, 40.0f};

        Vec2 s = content_->getMeasuredSize();
        if (s.x <= 0.0f && s.y <= 0.0f)
        {
            content_->measure(1000.0f, 1000.0f);
            s = content_->getMeasuredSize();
        }

        float pl = style_.currentStyle.padding.left.resolveH(0.0f, 0.0f);
        float pr = style_.currentStyle.padding.right.resolveH(0.0f, 0.0f);
        float pt = style_.currentStyle.padding.top.resolveV(0.0f, 0.0f);
        float pb = style_.currentStyle.padding.bottom.resolveV(0.0f, 0.0f);
        return {s.x + pl + pr, s.y + pt + pb};
    }
};

// Helper per costruire rapidamente un popup con voci di menu.
inline std::shared_ptr<Popup> ContextMenu(std::vector<std::pair<std::string, std::function<void()>>> items)
{
    auto popup = Popup::create();
    auto stack = VStack();
    stack->getInlineBase().gap = Px(2.0f);

    std::weak_ptr<Popup> weakPopup = popup;
    for (auto &[label, cb] : items)
    {
        auto item = Btn(label, [weakPopup, cb]()
                        {
            if (cb) cb();
            if (auto p = weakPopup.lock()) p->close(); });
        item->cls("popup-item");
        item->getInlineBase().width = Percent(100);
        item->getInlineBase().justify = Justify::Start;
        stack->addChild(item);
    }
    popup->setContent(stack);
    return popup;
}

} // namespace ZenitUI::UI