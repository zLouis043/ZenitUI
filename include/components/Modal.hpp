#pragma once

/**
 * \file Modal.hpp
 * \brief A full-screen overlay with an animated content panel.
 *
 * `Modal` is a `Stack` that covers the screen (once sized to `100%` by the
 * caller), paints a semi-transparent black backdrop, and hosts a single
 * content panel in its `content` slot. Show / hide animate both the backdrop
 * opacity (via inline style transition) and a slide-in of the content (via
 * an imperative `UIAnimation` named `"Intro"`).
 *
 * \note `Modal::onBuild` sets `interactive = false` — clicks pass through the
 *       backdrop unless a child under the pointer is interactive. This is
 *       usually the opposite of what a modal wants; see the `Interaction_modal`
 *       tests for how the framework currently behaves in practice. Also note
 *       that `setEnabled(false)` is used to disable a hidden modal so its
 *       subtree does not receive input.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"

namespace ZenitUI::UI
{

    /**
     * \class Modal
     * \brief A backdrop + content overlay.
     *
     * Style tag: `"Modal"`. Content slot: `contentTarget()` returns the
     * internal `content` VStack, so children added via `addChild` are
     * reparented into it automatically by `ZMarkup::create_impl`.
     */
    class Modal : public TLayout<Modal>
    {
    public:
        Modal() : TLayout<Modal>(LayoutType::Stack)
        {
        }

        /**
         * \brief Show the modal: enable, fade in, play the intro animation.
         *
         * Fires `onShow`. Note: this does not request focus; if the modal is
         * meant to be modal-with-focus, add a focusable child and call
         * `UIContext::get().requestFocus(child)` in `onShow`.
         */
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

        /**
         * \brief Hide the modal: fade out, play the intro animation in reverse.
         *
         * Fires `onHide`. The modal is disabled (which releases focus and
         * pointer capture if held) after the fade-out begins. There is no
         * "closed" notification; use the `onHide` callback if you need one.
         */
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

        /**
         * \brief Return the internal content panel.
         *
         * This is where children added through `contentTarget()` end up. It
         * is a `VStack` with class `"modal-content"` so themes can target
         * it.
         */
        Layout *contentTarget() override
        {
            return content ? content.get() : this;
        }

        /** \brief Fired after `show()`. */
        std::function<void()> onShow;
        /** \brief Fired after `hide()`. */
        std::function<void()> onHide;

    protected:
        /**
         * \brief Initialize the backdrop and content panel.
         *
         * Creates a full-screen `Stack` with:
         *  - `background: black @ 60%`;
         *  - `opacity: 0` (faded out at rest);
         *  - `focusScope: true` (Tab cycling stays inside the modal);
         *  - a `VStack` content child with class `"modal-content"`.
         *
         * Registers the `"Intro"` imperative animation on the content: a
         * 0.4s slide-up from `VH(100)` to `0` with `EaseOutBack` easing.
         */
        void onBuild() override
        {
            setInteractive(false);
            setStyleTag("Modal");
            setFocusScope(true);
            Style base;
            base.background = Colors::Black.withAlpha(0.6f);
            base.opacity = 0.0f;
            setInlineBase(base);
            setEnabled(false);

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