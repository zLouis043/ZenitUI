#pragma once

/**
 * \file Text.hpp
 * \brief A read-only text node backed by `UIContext::textEngine`.
 *
 * Wrapping, ellipsis truncation, shaping, and batched rendering are delegated
 * to `TextEngine`. Ellipsis (`text-overflow: ellipsis`) is applied at draw time
 * only, leaving the layout box unaffected.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI
{

    /** \addtogroup grp_widgets
     *  @{
     */

    /**
     * \class Text
     * \brief A read-only text node.
     *
     * Style tag: `"Text"`. Not interactive. Emits no `:hover` / `:pressed`
     * states — wrap the text in a `Panel` or a `Button` if you need those.
     */
    class Text : public TLayout<Text>
    {
    public:
        /** \brief Construct with initial content. */
        Text(std::string text) : TLayout<Text>(LayoutType::Stack), text(std::move(text))
        {
            setInteractive(false);
            setStyleTag("Text");
        }

        /** \brief Replace the content. Triggers a re-measure and re-wrap next frame. */
        void setText(std::string new_text)
        {
            text = std::move(new_text);
            text_dirty = true;
            internal_.pendingTransition = 1;
        }

        /** \brief Current content. */
        const std::string &getText() const { return text; }

        /** \brief Override the font used for measurement and rendering. */
        void setFont(FontHandle f)
        {
            defaultFont = f;
            text_dirty = true;
            internal_.pendingTransition = 1;
        }

        /** \brief Enable or disable word wrapping. */
        void setWrap(bool w)
        {
            wrap_ = w;
            text_dirty = true;
            internal_.pendingTransition = 1;
        }
        /** \brief Whether word wrapping is enabled. */
        bool getWrap() const { return wrap_; }

    protected:
        Vec2 computeIntrinsicSize(float availW, float /*availH*/) override
        {
            const bool effectiveWrap =
                wrap_ && style_.currentStyle.whiteSpace != WhiteSpace::Nowrap;
            const float wrapW = (effectiveWrap && availW > 0.0f) ? availW : -1.0f;
            ensureLayout(wrapW, style_.currentStyle);
            return layout_ ? layout_->size : Vec2{0.0f, 0.0f};
        }

        void renderContent(float op, const ComputedStyle &style) override
        {
            auto *ctx = context();
            auto *r = ctx ? ctx->renderer : nullptr;
            if (!r)
                return;

            const Vec2 vp = viewportSize();

            const float bwl = style.borderWidth.left.resolveH(rect.width, rect.height, vp);
            const float bwr = style.borderWidth.right.resolveH(rect.width, rect.height, vp);
            const float bwt = style.borderWidth.top.resolveV(rect.width, rect.height, vp);
            const float bwb = style.borderWidth.bottom.resolveV(rect.width, rect.height, vp);

            const float pl = style.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            const float pr = style.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            const float pt = style.padding.top.resolveV(rect.width, rect.height, vp) + bwt;
            const float pb = style.padding.bottom.resolveV(rect.width, rect.height, vp) + bwb;

            const float availW = std::max(0.0f, rect.width - pl - pr);
            const float availH = std::max(0.0f, rect.height - pt - pb);

            const bool effectiveWrap = wrap_ && style.whiteSpace != WhiteSpace::Nowrap;
            const float wrapW = (effectiveWrap && availW > 0.0f) ? availW : -1.0f;
            ensureLayout(wrapW, style);

            if (!layout_ || layout_->lines.empty())
                return;

            const TextMeasureSpec spec = makeSpec(style);
            std::shared_ptr<const TextLayout> activeLayout = layout_;

            const bool isSingleLine = activeLayout->lines.size() == 1;
            const bool applyEllipsis =
                isSingleLine &&
                style.textOverflow == TextOverflow::Ellipsis &&
                activeLayout->lines[0].width > availW;

            if (applyEllipsis)
            {
                TextLayoutOptions ellOpts;
                ellOpts.maxWidth = availW;
                ellOpts.wrap = false;
                ellOpts.ellipsis = true;
                ellOpts.ellipsisText = "...";
                activeLayout = ctx->textEngine.layoutCached(text, spec, ellOpts);
            }

            Vec2 pos = {rect.x + pl, rect.y + pt};
            const Align ha = (style.textAlign == Align::Auto) ? style.alignItems : style.textAlign;

            if (style.verticalAlign == Align::Center)
                pos.y += (availH - activeLayout->size.y) * 0.5f;
            else if (style.verticalAlign == Align::End)
                pos.y += availH - activeLayout->size.y;

            r->pushClip(rect);
            ctx->textEngine.draw(*r, *activeLayout,
                                 {pos.x, pos.y, availW, availH},
                                 style.color.withAlpha(op),
                                 spec, ha, text);
            r->popClip();
        }

        void onLayout() override
        {
            if (!wrap_ || style_.currentStyle.whiteSpace == WhiteSpace::Nowrap)
                return;

            const Vec2 vp = viewportSize();
            const float bwl = style_.currentStyle.borderWidth.left.resolveH(rect.width, rect.height, vp);
            const float bwr = style_.currentStyle.borderWidth.right.resolveH(rect.width, rect.height, vp);
            const float pl = style_.currentStyle.padding.left.resolveH(rect.width, rect.height, vp) + bwl;
            const float pr = style_.currentStyle.padding.right.resolveH(rect.width, rect.height, vp) + bwr;
            const float newAvailW = std::max(0.0f, rect.width - pl - pr);

            if (newAvailW > 0.0f && newAvailW != cachedWrapW)
                ensureLayout(newAvailW, style_.currentStyle);
        }

    private:
        TextMeasureSpec makeSpec(const ComputedStyle &style) const
        {
            const Vec2 vp = viewportSize();
            TextMeasureSpec spec;
            spec.fontStack = style.font;
            spec.font = resolveFont(style, defaultFont);
            spec.fontSize = style.fontSize.resolveV(vp.x, vp.y, vp);
            spec.letterSpacing = style.letterSpacing.resolveH(vp.x, vp.y, vp);
            return spec;
        }

        void ensureLayout(float wrapW, const ComputedStyle &style)
        {
            auto *ctx = context();
            if (!ctx)
                return;

            const TextMeasureSpec spec = makeSpec(style);
            if (layout_ && !text_dirty &&
                spec.font.id == cachedFontId &&
                spec.fontSize == cachedFs &&
                spec.letterSpacing == cachedSp &&
                wrapW == cachedWrapW)
                return;

            TextLayoutOptions opts;
            opts.wrap = (wrapW > 0.0f);
            opts.maxWidth = opts.wrap ? wrapW : 0.0f;
            opts.preserveSpaces = false;

            layout_ = ctx->textEngine.layoutCached(text, spec, opts);
            cachedFs = spec.fontSize;
            cachedSp = spec.letterSpacing;
            cachedWrapW = wrapW;
            cachedFontId = spec.font.id;
            text_dirty = false;
        }

        std::string text;
        FontHandle defaultFont{0};
        bool text_dirty{true};
        bool wrap_{false};
        float cachedFs{-1.0f};
        float cachedSp{-1.0f};
        float cachedWrapW{-2.0f};
        uint32_t cachedFontId{0};
        std::shared_ptr<const TextLayout> layout_;
    };

    /** \brief Convenience factory: a `Text` node. */
    inline std::shared_ptr<Text> Label(std::string t) { return Text::create(std::move(t)); }
    /** @} */
} // namespace ZenitUI::UI