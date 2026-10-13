#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    class CanvasLayout : public TLayout<CanvasLayout>
    {
    public:
        CanvasLayout() : TLayout<CanvasLayout>(LayoutType::Stack)
        {
            setInteractive(true);
            setStyleTag("Canvas");
        }
        ~CanvasLayout()
        {
            if (target.valid())
                UIContext::get().renderer->destroyTarget(target);
        }
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

            // Il subtree è disegnato in coordinate schermo (i rect sono
            // assoluti). Dentro il target, il sistema di coordinate riparte
            // da (0,0), quindi serve un offset che porti il nodo in cima
            // al target. Stesso pattern usato da Layout::drawLayer.
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

            // Disegna il target sul framebuffer con le transform del nodo.
            Transform2D tr;
            tr.pivot = rect.center();
            tr.translate = {
                style_.currentStyle.translateX.resolveSelfH(rect.width, rect.height),
                style_.currentStyle.translateY.resolveSelfV(rect.width, rect.height)};
            tr.rotationDeg = style_.currentStyle.rotation;
            tr.scale = style_.currentStyle.scale;

            // Resolve the shader to apply. Priority: `effect:` property
            // (if set and resolvable), then `setShader()` handle, then
            // nothing. Same priority as regular Layout nodes.
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