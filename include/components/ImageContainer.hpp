#pragma once

/**
 * \file ImageContainer.hpp
 * \brief A node that draws a single texture into its rect.
 *
 * Uses the style's `tint` colour to modulate the texture alpha; the renderer
 * receives the resulting colour. The intrinsic size is zero — the caller
 * must set an explicit size or wrap the container in a layout that does.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

/**
 * \class ImageContainer
 * \brief A single-texture node.
 *
 * \note Unlike most components, `ImageContainer` does not participate in
 *       hit-testing (`setInteractive(false)`): an image is treated as purely
 *       decorative unless a caller explicitly re-enables interaction. This
 *       matches the behaviour of CSS images.
 */
class ImageContainer : public TLayout<ImageContainer>
{
public:
    /**
     * \brief Construct with a texture.
     * \param tex Texture handle; may be invalid until `setTexture` is called.
     */
    ImageContainer(TextureHandle tex) : TLayout<ImageContainer>(LayoutType::Stack), tex(tex)
    {
        setInteractive(false);
        setStyleTag("ImageContainer");
    }

    /** \brief Replace the texture. Takes effect on the next frame. */
    void setTexture(TextureHandle t) { tex = t; }

protected:
    /**
     * \brief Draw the texture into `rect`, modulated by the style's `tint` and
     *        the current opacity `op`.
     *
     * The source rectangle is the full texture (no crop support yet). If the
     * texture handle is invalid, nothing is drawn — useful for lazy loading.
     */
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