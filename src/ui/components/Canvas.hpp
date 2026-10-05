#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

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
        float op = currentStyle.opacity * parentOp;
        r->pushTarget(target);
        renderChrome(op, currentStyle);
        renderContent(op, currentStyle);
        for (auto &c : children)
            c->draw(op);
        r->popTarget();

        Transform2D tr;
        tr.pivot = rect.center();
        tr.translate = {
            currentStyle.translateX.resolveSelfH(rect.width, rect.height),
            currentStyle.translateY.resolveSelfV(rect.width, rect.height)};
        tr.rotationDeg = currentStyle.rotation;
        tr.scale = currentStyle.scale;

        r->pushTransform(tr);
        if (hasShader && r->supports(Feature::Effects))
            r->pushEffect(customEffect);
        r->drawTarget(target, rect, Colors::White.withAlpha(op));
        if (hasShader && r->supports(Feature::Effects))
            r->popEffect();
        r->popTransform();
    }

private:
    TargetHandle target;
};

} // namespace ZenitUI::UI