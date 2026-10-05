#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class ImageContainer : public TLayout<ImageContainer>
{
public:
    ImageContainer(TextureHandle tex) : TLayout<ImageContainer>(LayoutType::Stack), tex(tex)
    {
        setInteractive(false);
        setStyleTag("ImageContainer");
    }
    void setTexture(TextureHandle t) { tex = t; }

protected:
    void renderContent(float op, const ComputedStyle &style) override
    {
        if (tex.valid())
            UIContext::get().renderer->drawTexture(
                tex,
                {0, 0, (float)tex.width, (float)tex.height},
                rect,
                style.tint.withAlpha(op));
    }

private:
    TextureHandle tex;
};

} // namespace ZenitUI::UI