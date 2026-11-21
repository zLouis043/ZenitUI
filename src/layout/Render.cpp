#include "Layout.hpp"
#include "Debug.hpp"
#include "StyleAttr.hpp"
#include "FilterRegistry.hpp"

namespace ZenitUI
{
	void Layout::drawWithStyle(const ComputedStyle &renderStyle, float parentOpacity)
	{
		auto renderer = UIContext::get().renderer;
		if (!renderer)
			return;

		resolveEffectIfNeeded(renderStyle);

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f)
			return;

		const bool isLayer =
			!renderStyle.filters.empty() &&
			renderer->supports(Feature::Effects) &&
			rect.width > 0.0f && rect.height > 0.0f &&
			(!renderer->inTarget() || renderer->supports(Feature::NestedTargets));

		if (isLayer)
			drawLayer(renderStyle, globalOp);
		else
			drawInline(renderStyle, globalOp);

		scroll_.drawScrollbar(*this, parentOpacity);
	}

	void Layout::draw(float parentOpacity)
	{
		if (config.portal)
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

		if (anim_.css.empty())
		{
			drawWithStyle(style_.currentStyle, parentOpacity);
		}
		else
		{
			ComputedStyle renderStyle = style_.currentStyle;
			Anim::overlayCssComputed(renderStyle, anim_.css);
			drawWithStyle(renderStyle, parentOpacity);
		}

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

		renderChrome(globalOp, renderStyle);

		bool needsClip = renderStyle.overflowX != Overflow::Visible || renderStyle.overflowY != Overflow::Visible;
		if (needsClip)
			renderer->pushClip(rect);

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

	void Layout::drawLayer(const ComputedStyle &renderStyle, float globalOp)
	{
		auto &ctxUI = UIContext::get();
		auto *renderer = ctxUI.renderer;

		Transform2D currTr = currentTransform(renderStyle);

		Vec2 p0 = applyTransform(currTr, {rect.x, rect.y});
		Vec2 p1 = applyTransform(currTr, {rect.x + rect.width, rect.y});
		Vec2 p2 = applyTransform(currTr, {rect.x, rect.y + rect.height});
		Vec2 p3 = applyTransform(currTr, {rect.x + rect.width, rect.y + rect.height});

		float minX = std::min({p0.x, p1.x, p2.x, p3.x});
		float maxX = std::max({p0.x, p1.x, p2.x, p3.x});
		float minY = std::min({p0.y, p1.y, p2.y, p3.y});
		float maxY = std::max({p0.y, p1.y, p2.y, p3.y});

		Rect transformedRect = {minX, minY, maxX - minX, maxY - minY};

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

		constexpr int MAX_TARGET_DIM = TargetPool::MAX_DIM;
		if (tw > MAX_TARGET_DIM || th > MAX_TARGET_DIM)
		{
			drawInline(renderStyle, globalOp);
			return;
		}

		registerBuiltinFilters();
		auto &registry = FilterRegistry::get();
		auto &stats = ctxUI.renderStats;
		auto &pool = ctxUI.targetPool;

		TargetHandle mainTarget = pool.acquire(*renderer, tw, th, &stats);
		if (!mainTarget.valid())
			return;

		renderer->pushTarget(mainTarget);
		stats.pushTargetCalls++;

		Transform2D offsetTr;
		offsetTr.pivot = {0.0f, 0.0f};
		offsetTr.translate = {-region.x, -region.y};

		Transform2D tr = currentTransform(renderStyle);
		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;
		renderer->pushTransform(offsetTr);
		if (!identity)
			renderer->pushTransform(tr);

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

		const Rect dstRect = {0, 0, (float)tw, (float)th};

		if (renderStyle.filters.empty())
		{
			renderer->drawTarget(mainTarget, dstRect, region,
								 Colors::White.withAlpha(globalOp));
			pool.release(mainTarget);
			return;
		}

		const size_t numFilters = renderStyle.filters.size();
		const bool needsScratch = (numFilters >= 2);
		bool needsSeparable = false;
		for (const auto &fref : renderStyle.filters)
		{
			if (const auto *def = registry.find(fref.name))
			{
				if ((def->pattern == FilterPattern::Separable && def->downscale <= 1) ||
					def->pattern == FilterPattern::Custom)
				{
					needsSeparable = true;
					break;
				}
			}
		}

		TargetHandle scratchTarget{};
		TargetHandle separableTarget{};

		if (needsScratch)
		{
			scratchTarget = pool.acquire(*renderer, tw, th, &stats);
			if (!scratchTarget.valid())
			{
				pool.release(mainTarget);
				return;
			}
		}
		if (needsSeparable)
		{
			separableTarget = pool.acquire(*renderer, tw, th, &stats);
			if (!separableTarget.valid())
			{
				if (scratchTarget.valid())
					pool.release(scratchTarget);
				pool.release(mainTarget);
				return;
			}
		}

		FilterContext ctx;
		ctx.renderer = renderer;
		ctx.node = this;
		ctx.scratch = separableTarget;
		ctx.region = region;
		ctx.dst = dstRect;
		ctx.texSize = {(float)mainTarget.width, (float)mainTarget.height};
		ctx.opacity = globalOp;

		TargetHandle currentRead = mainTarget;
		TargetHandle currentWrite = scratchTarget;

		for (size_t i = 0; i < numFilters; ++i)
		{
			const bool isLast = (i == numFilters - 1);

			ctx.ref = &renderStyle.filters[i];
			ctx.src = currentRead;
			ctx.texSize = {(float)currentRead.width, (float)currentRead.height};
			ctx.write_target = isLast ? TargetHandle{} : currentWrite;

			if (const auto *def = registry.find(ctx.ref->name))
			{
				applyFilter(*def, ctx);
			}
			else
			{
				if (isLast)
				{
					renderer->drawTarget(currentRead, ctx.dst, region,
										 Colors::White.withAlpha(globalOp));
				}
				else
				{
					renderer->pushTarget(currentWrite);
					stats.pushTargetCalls++;
					renderer->drawTarget(currentRead, ctx.dst, ctx.dst, Colors::White);
					renderer->popTarget();
				}
			}

			if (!isLast)
			{
				std::swap(currentRead, currentWrite);
			}
		}

		if (separableTarget.valid())
			pool.release(separableTarget);
		if (scratchTarget.valid())
			pool.release(scratchTarget);
		pool.release(mainTarget);
	}

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