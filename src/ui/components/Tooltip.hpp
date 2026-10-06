#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"
#include "Text.hpp"

namespace ZenitUI::UI {

class Tooltip : public TLayout<Tooltip, Panel>
{
public:
    Tooltip(std::string text)
        : TLayout<Tooltip, Panel>(), text_(std::move(text)) {}

    static std::shared_ptr<Tooltip> attach(std::shared_ptr<Layout> owner,
                                           std::string text,
                                           float showDelay = 0.4f)
    {
        auto tip = Tooltip::create(std::move(text));
        tip->anchor_ = owner.get();
        tip->showDelay_ = showDelay;

        auto prevEnter = owner->onHoverEnter;
        auto prevExit = owner->onHoverExit;

        owner->onHoverEnter = [tip, prevEnter]()
        {
            if (prevEnter)
                prevEnter();
            tip->show();
        };
        owner->onHoverExit = [tip, prevExit]()
        {
            if (prevExit)
                prevExit();
            tip->hide();
        };

        owner->addChild(tip);
        return tip;
    }

    void setAnchor(Layout *a) { anchor_ = a; }
    void setShowDelay(float s) { showDelay_ = s; }

    void show()
    {
        hovering_ = true;
        timer_ = 0.0f;
    }
    void hide()
    {
        hovering_ = false;
        visible_ = false;
        timer_ = 0.0f;
        setTargetOpacity(0.0f);
    }

    void arrange(Rect) override {}

protected:
    void onBuild() override
    {
        setStyleTag("Tooltip");
        setPortal(true);
        setInteractive(false);
        setBlocksRaycast(false);
        style_.inlineBase.position = Position::Absolute;
        style_.inlineBase.opacity = 0.0f;

        auto label = Label(text_);
        label->getInlineBase().fontSize = VH(2.4f);
        addChild(label);
    }

    void onUpdate(float dt) override
    {
        if (hovering_ && !visible_)
        {
            timer_ += dt;
            if (timer_ >= showDelay_)
            {
                visible_ = true;
                setTargetOpacity(1.0f);
            }
        }

        if (anchor_)
        {
            Rect a = anchor_->getRect();
            Vec2 ms = getMeasuredSize();
            if (ms.x > 0.0f && ms.y > 0.0f)
            {
                rect = {
                    a.x + a.width * 0.5f - ms.x * 0.5f,
                    a.y - ms.y - 6.0f,
                    ms.x,
                    ms.y};
                arrangeInto(rect);
            }
        }
    }

private:
    std::string text_;
    Layout *anchor_{nullptr};
    float showDelay_{0.4f};
    float timer_{0.0f};
    bool hovering_{false};
    bool visible_{false};

    void setTargetOpacity(float op)
    {
        float old = style_.inlineBase.opacity.get_or(0.0f);
        if (style_.inlineBase.opacity.is_set && old == op)
            return;
        style_.inlineBase.opacity = op;
        beginTransition();
    }
};

} // namespace ZenitUI::UI