#pragma once

/**
 * \file Tooltip.hpp
 * \brief A hover-triggered floating label.
 *
 * Tooltips are created with `Tooltip::attach(owner, text, delay)`: the
 * returned tooltip is added as a child of \p owner but behaves as a portal,
 * so it draws above everything and does not affect the owner's layout. On
 * hover-enter the tooltip counts down `showDelay`; if the pointer is still
 * over the owner when the timer expires, the tooltip fades in. On
 * hover-exit, it hides immediately.
 *
 * The tooltip positions itself above the anchor and flips below if there is
 * not enough room; it clamps to the viewport horizontally.
 *
 * \warning The tooltip stores the anchor as a raw pointer and is owned by the
 *          anchor. `Tooltip::attach` uses `owner->addChild`, so the anchor
 *          outlives the tooltip automatically. If you ever reparent the
 *          tooltip manually, use `setAnchor` with a live anchor or the
 *          pointer will dangle.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"
#include "Text.hpp"

namespace ZenitUI::UI
{

    /**
     * \class Tooltip
     * \brief A portal-based hover label.
     *
     * Style tag: `"Tooltip"`. Portal by default. Not interactive, does not
     * block raycast. `arrange()` is a no-op; the tooltip positions itself via
     * `onUpdate` once the label has been measured.
     */
    class Tooltip : public TLayout<Tooltip, Panel>
    {
    public:
        /**
         * \brief Construct a tooltip with the given text.
         * \note Use `Tooltip::attach` unless you are managing the anchor and
         *       parenting manually.
         */
        Tooltip(std::string text)
            : TLayout<Tooltip, Panel>(), text_(std::move(text)) {}

        /**
         * \brief Attach a tooltip to \p owner.
         *
         * Installs `onHoverEnter` / `onHoverExit` handlers on \p owner
         * (chaining any previous ones) so that hovering the owner shows the
         * tooltip after \p showDelay and leaves immediately. The tooltip is
         * added as a child of \p owner; this is what makes its lifetime
         * safe.
         *
         * \param owner      Node to attach to. Must be non-null.
         * \param text       Text to display.
         * \param showDelay  Seconds the pointer must remain over the owner
         *                   before the tooltip appears. `0` shows instantly
         *                   on hover.
         * \return The created tooltip, for further customization.
         */
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

        /** \brief Change the anchor (advanced). See class warning. */
        void setAnchor(Layout *a) { anchor_ = a; }
        /** \brief Change the delay in seconds before the tooltip appears. */
        void setShowDelay(float s) { showDelay_ = s; }

        /** \brief Begin the countdown to show. Idempotent while hovering. */
        void show()
        {
            hovering_ = true;
            timer_ = 0.0f;
        }
        /** \brief Hide immediately and cancel any pending countdown. */
        void hide()
        {
            hovering_ = false;
            visible_ = false;
            timer_ = 0.0f;
            setTargetOpacity(0.0f);
        }

        /** \brief No-op: the tooltip owns its rect via `onUpdate`. */
        void arrange(Rect) override {}

    protected:
        /**
         * \brief Build the internal label and initialize the visual.
         *
         * Content is a single `Label` at `VH(2.4)` font size. The tooltip
         * starts invisible (`opacity: 0`).
         */
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

        /**
         * \brief Count down the delay and position the tooltip.
         *
         * Positioning: centered above the anchor, offset 6 px. Flips below if
         * the top would be clipped, and clamps to the viewport on both axes.
         * Runs every frame while visible (or hovering).
         */
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
                    float targetX = a.x + a.width * 0.5f - ms.x * 0.5f;
                    float targetY = a.y - ms.y - 6.0f;

                    if (targetX < 0.0f)
                        targetX = 0.0f;
                    if (targetX + ms.x > Metrics::viewport.x)
                        targetX = Metrics::viewport.x - ms.x;

                    if (targetY < 0.0f)
                    {
                        targetY = a.y + a.height + 6.0f;
                    }
                    if (targetY + ms.y > Metrics::viewport.y)
                    {
                        targetY = Metrics::viewport.y - ms.y;
                    }

                    rect = {targetX, targetY, ms.x, ms.y};
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

        /** \brief Set the target opacity and start a transition if it changed. */
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