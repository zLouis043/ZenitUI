#pragma once

/**
 * \file Popup.hpp
 * \brief An anchored floating panel with close-on-outside and close-on-Escape.
 *
 * A `Popup` is a `Panel` subclass that:
 *  - sets `portal = true` (draws and hit-tests above everything else);
 *  - is hidden by default (`enabled = false`, `opacity = 0`);
 *  - on open, sizes itself to its content's measured size and positions
 *    itself adjacent to an anchor widget (or at a screen position);
 *  - auto-flips above / below the anchor when there is not enough room;
 *  - closes itself on outside-click or Escape when configured to do so.
 *
 * The popup never participates in the parent layout: its `arrange` override
 * is a no-op, and it takes full ownership of its own rect via `onUpdate`.
 *
 * \note `openAbove` sets a flag but does not measure anything: the actual
 *       positioning decision happens in `onUpdate` on the next frame, once
 *       the content has been measured. This two-step design lets a popup
 *       adapt to content that is only known after the first `measure` pass.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Layouts.hpp"
#include "Button.hpp"

namespace ZenitUI::UI
{

    /**
     * \class Popup
     * \brief A floating, anchored panel.
     *
     * Style tag: `"Popup"`. Portal by default. Content is whatever you pass
     * to `setContent`.
     */
    class Popup : public TLayout<Popup, Panel>
    {
    public:
        Popup() : TLayout<Popup, Panel>() {}

        /**
         * \brief Set the popup's content. Replaces any previous content.
         *
         * The content is added as a child of the popup; its measured size
         * determines the popup's size on the next frame.
         */
        void setContent(std::shared_ptr<Layout> content)
        {
            content_ = std::move(content);
            if (content_)
                addChild(content_);
        }

        /** \brief Open the popup just below \p anchor. */
        void openBelow(std::shared_ptr<Layout> anchor)
        {
            anchor_ = anchor.get();
            pos_ = {0.0f, 0.0f};
            openInternal();
        }

        /**
         * \brief Open the popup just above \p anchor.
         *
         * Sets the preferred direction; `onUpdate` will flip below if there
         * is not enough room above.
         */
        void openAbove(std::shared_ptr<Layout> anchor)
        {
            anchor_ = anchor.get();
            above_ = true;
            openInternal();
        }

        /** \brief Open the popup at an absolute screen position. */
        void openAt(Vec2 screenPos)
        {
            anchor_ = nullptr;
            pos_ = screenPos;
            above_ = false;
            openInternal();
        }

        /**
         * \brief Close the popup, if it is open.
         *
         * Fades out via inline style transition, disables the popup, and
         * fires `onClose`. No-op if already closed.
         */
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

        /** \brief `true` while the popup is visible. */
        bool isOpen() const { return isOpen_; }

        bool closeOnClickOutside{true};  ///< Close on any button press outside the popup.
        bool closeOnEscape{true};        ///< Close on Escape.
        float offsetBelow{4.0f};         ///< Gap between the anchor and the popup.
        std::function<void()> onOpen;    ///< Fired after `open*`.
        std::function<void()> onClose;   ///< Fired after `close`.

        /** \brief No-op: the popup owns its rect via `onUpdate`. */
        void arrange(Rect) override { /* no-op */ }

    protected:
        /**
         * \brief Initialize the popup: portal, interactive, focusable,
         *        block-raycast, hidden.
         */
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

        /**
         * \brief Reposition, resize, and close-on-outside / Escape.
         *
         * Runs every frame while open. Positions the popup based on the
         * anchor's current rect (or `pos_` if opened with `openAt`), flips
         * if necessary, and clamps to the viewport. Fires the configured
         * close behaviours.
         */
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

            if (closeOnClickOutside && ctx.pointer[MouseButton::Left].pressed)
            {
                if (!rect.contains(ctx.pointer.pos))
                    close();
            }

            if (closeOnClickOutside && ctx.pointer[MouseButton::Right].pressed)
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

        /**
         * \brief Common open logic: enable, fade in, request focus, fire
         *        `onOpen`.
         */
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
            if (onOpen)
                onOpen();
        }

        /**
         * \brief Compute the popup size from the content's measured size.
         *
         * If the content has not been measured yet (first frame after
         * `setContent`), triggers a measurement with a large available box
         * and uses the result. Padding is added on top of the content size.
         */
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

    /**
     * \brief Convenience factory: a vertical context menu popup.
     *
     * Each item is a full-width, left-aligned button. Clicking an item
     * closes the popup and fires its callback.
     *
     * \param items List of (label, callback) pairs. Empty labels produce
     *              invisible buttons; use them for separators at your own
     *              discretion (no special handling).
     */
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