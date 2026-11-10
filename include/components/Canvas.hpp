#pragma once

/**
 * \file Canvas.hpp
 * \brief An off-screen rendering surface with optional post-effect.
 *
 * `CanvasLayout` renders its entire subtree into an off-screen target, then
 * blits the target into its own rect with an optional shader effect applied.
 * This lets you build things that plain clip-and-draw cannot express:
 *
 *  - content that spills outside its own rect and should be clipped to it;
 *  - per-subtree post-processing (`effect` from style, or a shader set via
 *    `setShader`) applied as a single pass;
 *  - transforms that would otherwise bleed into siblings.
 *
 * The target is created lazily on first draw and re-created only when the
 * canvas size changes. It is destroyed in the destructor.
 *
 * \note `CanvasLayout::draw` overrides the base implementation entirely: it
 *       does **not** apply CSS animations to the chrome (background, border,
 *       shadow), does not apply filters from the style, and does not draw
 *       scrollbars. If you need those, use a plain `Layout` with `effect`
 *       instead of a `CanvasLayout`.
 *
 * \warning The destructor dereferences `UIContext::get().renderer` without a
 *          null check after `target.valid()` returns true. If the renderer
 *          has already been destroyed when the canvas is destroyed (a
 *          shutdown-order bug), this will crash. Ensure canvases outlive
 *          their renderer or explicitly destroy them before tearing down the
 *          backend.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    /**
     * \class CanvasLayout
     * \brief A subtree rendered into an off-screen target.
     *
     * Style tag: `"Canvas"`. Interactive (clicks are hit-tested normally
     * against the canvas rect, not against the transformed content — hit
     * testing does **not** account for the effect).
     *
     * The canvas's own rect defines both the target size and the blit
     * destination. Content is translated by `(-rect.x, -rect.y)` inside the
     * target so that absolutely-positioned children at `(0, 0)` land at the
     * target origin.
     */
    class CanvasLayout : public TLayout<CanvasLayout>
    {
    public:
        CanvasLayout() : TLayout<CanvasLayout>(LayoutType::Stack)
        {
            setInteractive(true);
            setStyleTag("Canvas");
        }

        /**
         * \brief Destroy the off-screen target if one was created.
         *
         * \warning Does not check `UIContext::get().renderer` for null. See
         *          the class-level warning about shutdown order.
         */
        ~CanvasLayout()
        {
            if (target.valid())
                UIContext::get().renderer->destroyTarget(target);
        }

        /**
         * \brief Render the subtree into the target, then blit with effect.
         *
         * Steps:
         *  1. Ensure the target exists and matches the canvas size.
         *  2. Push the target, push a `(-rect.x, -rect.y)` translate so that
         *     content positioned in absolute screen coords lands at target
         *     origin.
         *  3. Draw chrome, content, and children into the target.
         *  4. Pop the target and the translate.
         *  5. Push the canvas's own transform (rotate / scale / translate),
         *     push the effect if any, blit the target into `rect`, pop effect
         *     and transform.
         *
         * Note that the `opacity` multiplies through: children are drawn at
         * `opacity`, and the final blit also applies `opacity` — this is
         * intentional so that a fading canvas fades both its content and its
         * chrome simultaneously.
         */
        void draw(float parentOp) override
        {
            auto r = UIContext::get().renderer;
            if (rect.width <= 0 || rect.height <= 0)
                return;
            if (!target.valid() || target.width != (int)rect.width || target.height != (int)rect.height)
            {
                if (target.valid())
                    r->destroyTarget(target);
                target = r->createTarget((int)rect.width, (int)rect.height);
            }
            float op = style_.currentStyle.opacity * parentOp;

            Transform2D offsetTr;
            offsetTr.pivot = {0.0f, 0.0f};
            offsetTr.translate = {-rect.x, -rect.y};

            r->pushTarget(target);
            r->pushTransform(offsetTr);

            renderChrome(op, style_.currentStyle);
            renderContent(op, style_.currentStyle);
            for (auto &c : children)
                c->draw(op);

            r->popTransform();
            r->popTarget();

            Transform2D tr;
            tr.pivot = rect.center();
            tr.translate = {
                style_.currentStyle.translateX.resolveSelfH(rect.width, rect.height),
                style_.currentStyle.translateY.resolveSelfV(rect.width, rect.height)};
            tr.rotationDeg = style_.currentStyle.rotation;
            tr.scale = style_.currentStyle.scale;

            EffectHandle fx = resolveActiveEffect();
            const bool applyFx = fx.valid() && r->supports(Feature::Effects);

            r->pushTransform(tr);
            if (applyFx)
                r->pushEffect(fx);
            r->drawTarget(target, rect, Colors::White.withAlpha(op));
            if (applyFx)
                r->popEffect();
            r->popTransform();
        }

    private:
        TargetHandle target;
    };

} // namespace ZenitUI::UI