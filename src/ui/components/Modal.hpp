#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"

namespace ZenitUI::UI
{

    class Modal : public TLayout<Modal>
    {
    public:
        Modal() : TLayout<Modal>(LayoutType::Stack)
        {
        }
        void show()
        {
            setEnabled(true);
            setInteractive(true);
            Style vis;
            vis.opacity = 1.0f;
            setInlineBase(vis);
            content->playAnimation("Intro", false);
            if (onShow)
                onShow();
        }
        void hide()
        {
            setEnabled(false);
            setInteractive(false);
            Style hid;
            hid.opacity = 0.0f;
            setInlineBase(hid);
            content->playAnimation("Intro", true);
            if (onHide)
                onHide();
        }

        // Content slot for ZMarkup. The modal builds an internal
        // `content` wrapper in onBuild() (with the Intro animation).
        // Markup children should end up inside that wrapper, not as
        // siblings of it, so that the animation applies to them.
        // Layout::contentTarget() tells ZMarkup::create where to
        // attach. Regular C++ code can keep using addChild(), which
        // routes to the same place.
        Layout *contentTarget() override
        {
            return content ? content.get() : this;
        }

        std::function<void()> onShow;
        std::function<void()> onHide;

    protected:
        void onBuild() override
        {
            setInteractive(false);
            setStyleTag("Modal");
            setFocusScope(true);
            Style base;
            base.background = Colors::Black.withAlpha(0.6f);
            base.opacity = 0.0f;
            setInlineBase(base);
            setEnabled(false); // <-- disabilita il modal finché non è mostrato

            content = VStack()->cls("modal-content");
            addChild(content);

            auto intro = std::make_shared<UIAnimation>(0.4f);
            intro->addTrack<Value>(VH(100.0f), Value(0.0f), [](Layout *l, Value v)
                                   { l->getInlineBase().translateY = v; }, TransitionFunction::EaseOutBack);
            content->addAnimation("Intro", intro);
        }

        std::shared_ptr<Layout> content;
    };

} // namespace ZenitUI::UI