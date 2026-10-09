#include "Layout.hpp"
#include "Debug.hpp"
#include "StyleAttr.hpp"
#include "FilterRegistry.hpp"

namespace ZenitUI
{

	// =========================================================================
	//  RENDER
	//  draw() ha due path:
	//   - inline: senza filter. Chrome+content+figli sul framebuffer.
	//   - layer:  con filter. Subtree → render target → shader → framebuffer.
	// =========================================================================

	void Layout::draw(float parentOpacity)
	{
		if (isPortal_)
		{
			UIContext::get().framePortals.push_back(weak_from_this());
			return;
		}

		auto renderer = UIContext::get().renderer;
		if (!renderer)
			return;

		if (rect.width > 0.0f && rect.height > 0.0f)
		{
			Rect clip = renderer->getClipRect();
			float m = 40.0f;
			if (rect.x > clip.x + clip.width + m ||
				rect.x + rect.width < clip.x - m ||
				rect.y > clip.y + clip.height + m ||
				rect.y + rect.height < clip.y - m)
				return;
		}

		ComputedStyle renderStyle = style_.currentStyle;
		Anim::overlayCssComputed(renderStyle, anim_.css);
		resolveEffectIfNeeded(renderStyle);

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f)
			return;

		const bool isLayer =
			!renderStyle.filters.empty() &&
			renderer->supports(Feature::Effects) &&
			rect.width > 0.0f && rect.height > 0.0f &&
			!renderer->inTarget(); // Raylib non ha texture mode annidate

		if (isLayer)
			drawLayer(renderStyle, globalOp);
		else
			drawInline(renderStyle, globalOp);

		scroll_.drawScrollbar(*this, parentOpacity);

		if (!hasParent())
		{
			auto portals = UIContext::get().framePortals;
			for (auto &wp : portals)
			{
				auto sp = wp.lock();
				if (!sp)
					continue;
				bool wasPortal = sp->isPortal();
				sp->setPortal(false);
				sp->draw(1.0f);
				sp->setPortal(wasPortal);
			}
		}
	}

	// -------------------------------------------------------------------------
	//  INLINE: comportamento attuale (chrome + content + figli direttamente)
	// -------------------------------------------------------------------------
	void Layout::drawInline(const ComputedStyle &renderStyle, float globalOp)
	{
		auto renderer = UIContext::get().renderer;

		Transform2D tr = currentTransform(renderStyle);
		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;

		if (!identity)
			renderer->pushTransform(tr);
		if (hasShader && renderer->supports(Feature::Effects))
			renderer->pushEffect(customEffect);
		if (currentEffect_.valid() && renderer->supports(Feature::Effects))
			renderer->pushEffect(currentEffect_);

		bool needsClip = renderStyle.overflowX != Overflow::Visible || renderStyle.overflowY != Overflow::Visible;
		if (needsClip)
			renderer->pushClip(rect);

		renderChrome(globalOp, renderStyle);
		renderContent(globalOp, renderStyle);
		drawChildren(globalOp);

		if (needsClip)
			renderer->popClip();
		if (currentEffect_.valid() && renderer->supports(Feature::Effects))
			renderer->popEffect();
		if (hasShader && renderer->supports(Feature::Effects))
			renderer->popEffect();
		if (!identity)
			renderer->popTransform();
	}

	// -------------------------------------------------------------------------
	//  LAYER: subtree → render target → filtro → framebuffer
	//  Un solo filtro per ora; multi-filter è un'estensione (ping-pong).
	// -------------------------------------------------------------------------
	void Layout::drawLayer(const ComputedStyle &renderStyle, float globalOp)
	{
		auto renderer = UIContext::get().renderer;

		// rect e getClipRect() sono entrambe in coordinate schermo.
		// La regione visibile è la loro intersezione.
		// Calcola il bounding box del nodo DOPO il transform (scale + translate).
		// Rotation ignorata: rara su nodi con filter. Se servirà, si aggiunge.
		const float scale = renderStyle.scale;
		const float tx = renderStyle.translateX.resolveSelfH(rect.width, rect.height);
		const float ty = renderStyle.translateY.resolveSelfV(rect.width, rect.height);
		const Vec2 pivot = rect.center();

		const float tW = rect.width * scale;
		const float tH = rect.height * scale;

		Rect transformedRect = {
			pivot.x - tW * 0.5f + tx,
			pivot.y - tH * 0.5f + ty,
			tW, tH};

		// Regione visibile = intersezione fra il bbox trasformato e il clip.
		Rect clip = renderer->getClipRect();

		Rect region;
		region.x = std::max(transformedRect.x, clip.x);
		region.y = std::max(transformedRect.y, clip.y);
		float right = std::min(transformedRect.x + transformedRect.width, clip.x + clip.width);
		float bottom = std::min(transformedRect.y + transformedRect.height, clip.y + clip.height);
		region.width = std::max(0.0f, right - region.x);
		region.height = std::max(0.0f, bottom - region.y);

		if (region.width < 1.0f || region.height < 1.0f)
			return;

		const int tw = (int)std::ceil(region.width);
		const int th = (int)std::ceil(region.height);
		if (tw <= 0 || th <= 0)
			return;

		constexpr int MAX_TARGET_DIM = 4096;
		if (tw > MAX_TARGET_DIM || th > MAX_TARGET_DIM)
		{
			drawInline(renderStyle, globalOp);
			return;
		}

		if (!layerTarget_.valid() || layerTarget_.width != tw || layerTarget_.height != th)
		{
			if (layerTarget_.valid())
				renderer->destroyTarget(layerTarget_);
			layerTarget_ = renderer->createTarget(tw, th);
			if (!layerTarget_.valid())
				return;
		}
		if (!layerScratch_.valid() || layerScratch_.width != tw || layerScratch_.height != th)
		{
			if (layerScratch_.valid())
				renderer->destroyTarget(layerScratch_);
			layerScratch_ = renderer->createTarget(tw, th);
			if (!layerScratch_.valid())
				return;
		}

		// Disegna il subtree nel layerTarget_.
		// - region.x/y sono in screen; il translate -region li porta in (0,0) del target.
		// - il matrix del parent è già isolato da pushTarget.
		renderer->pushTarget(layerTarget_);

		Transform2D offsetTr;
		offsetTr.pivot = {0.0f, 0.0f};
		offsetTr.translate = {-region.x, -region.y};

		Transform2D tr = currentTransform(renderStyle);
		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;
		if (!identity)
			renderer->pushTransform(tr);
		renderer->pushTransform(offsetTr);

		if (hasShader && renderer->supports(Feature::Effects))
			renderer->pushEffect(customEffect);
		if (currentEffect_.valid() && renderer->supports(Feature::Effects))
			renderer->pushEffect(currentEffect_);

		renderChrome(globalOp, renderStyle);
		renderContent(globalOp, renderStyle);
		drawChildren(globalOp);

		if (currentEffect_.valid() && renderer->supports(Feature::Effects))
			renderer->popEffect();
		if (hasShader && renderer->supports(Feature::Effects))
			renderer->popEffect();
		renderer->popTransform();
		if (!identity)
			renderer->popTransform();

		renderer->popTarget();

		registerBuiltinFilters();

		FilterContext ctx;
		ctx.renderer = renderer;
		ctx.node = this;
		ctx.src = layerTarget_;
		ctx.scratch = layerScratch_;
		ctx.region = region;
		ctx.dst = {0, 0, (float)tw, (float)th};
		ctx.opacity = globalOp;

		auto &registry = FilterRegistry::get();

		if (renderStyle.filters.empty())
		{
			renderer->drawTarget(layerTarget_, region, Colors::White.withAlpha(globalOp));
			return;
		}

		for (const auto &f : renderStyle.filters)
		{
			ctx.ref = &f;

			if (const auto *def = registry.find(f.name))
			{
				applyFilter(*def, ctx);
			}
			else
			{
				renderer->drawTarget(layerTarget_, region, Colors::White.withAlpha(globalOp));
			}
		}
	}

	// -------------------------------------------------------------------------
	//  Figli ordinati per z-index.
	// -------------------------------------------------------------------------
	void Layout::drawChildren(float globalOp)
	{
		drawNegZ_.clear();
		drawNormal_.clear();
		drawPosZ_.clear();

		for (auto &child : children)
		{
			if (child->isStackingContext())
			{
				if (child->getZIndex() < 0)
					drawNegZ_.push_back(child.get());
				else
					drawPosZ_.push_back(child.get());
			}
			else
			{
				drawNormal_.push_back(child.get());
			}
		}

		auto sortByZ = [](Layout *a, Layout *b)
		{ return a->getZIndex() < b->getZIndex(); };
		std::stable_sort(drawNegZ_.begin(), drawNegZ_.end(), sortByZ);
		std::stable_sort(drawPosZ_.begin(), drawPosZ_.end(), sortByZ);

		for (auto *child : drawNegZ_)
			child->draw(globalOp);
		for (auto *child : drawNormal_)
			child->draw(globalOp);
		for (auto *child : drawPosZ_)
			child->draw(globalOp);
	}

	// -------------------------------------------------------------------------
	//  Effect resolution (nome stile → EffectHandle)
	// -------------------------------------------------------------------------
	void Layout::resolveEffectIfNeeded(const ComputedStyle &style)
	{
		if (style.effect.empty())
		{
			effectName_.clear();
			currentEffect_ = {};
			return;
		}
		if (style.effect == effectName_)
			return;

		effectName_ = style.effect;
		if (auto *assets = UIContext::get().assets)
			currentEffect_ = assets->getEffect(style.effect);
		else
			currentEffect_ = {};
	}

} // namespace ZenitUI