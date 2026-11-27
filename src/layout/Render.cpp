#include "Layout.hpp"
#include "Debug.hpp"
#include "StyleAttr.hpp"
#include "FilterRegistry.hpp"

#include <cstdint>
#include <cstring>
#include <cstdlib>

namespace ZenitUI
{
	namespace
	{
		constexpr uint64_t kFnvOffset = 14695981039346656037ull;
		constexpr uint64_t kFnvPrime = 1099511628211ull;
		constexpr uint64_t kLayerEvictGraceFrames = 120;

		inline void hashBytes(uint64_t &h, const void *data, size_t len) noexcept
		{
			const auto *p = static_cast<const uint8_t *>(data);
			for (size_t i = 0; i < len; ++i)
			{
				h ^= p[i];
				h *= kFnvPrime;
			}
		}

		template <typename T>
		inline void hashPod(uint64_t &h, const T &v) noexcept
		{
			hashBytes(h, &v, sizeof(T));
		}

		inline void hashFloat(uint64_t &h, float v) noexcept
		{
			// Normalize -0.0f so signed zeros hash identically.
			if (v == 0.0f)
				v = 0.0f;
			uint32_t bits = 0;
			std::memcpy(&bits, &v, sizeof(bits));
			hashPod(h, bits);
		}

		inline void hashColor(uint64_t &h, const Color &c) noexcept
		{
			const uint32_t packed = (static_cast<uint32_t>(c.r) << 24) |
									(static_cast<uint32_t>(c.g) << 16) |
									(static_cast<uint32_t>(c.b) << 8) |
									static_cast<uint32_t>(c.a);
			hashPod(h, packed);
		}

		inline void hashString(uint64_t &h, const std::string &s) noexcept
		{
			const uint64_t sz = s.size();
			hashPod(h, sz);
			if (!s.empty())
				hashBytes(h, s.data(), s.size());
		}

		inline void hashFilters(uint64_t &h, const std::vector<FilterRef> &filters) noexcept
		{
			const uint64_t count = filters.size();
			hashPod(h, count);
			for (const auto &f : filters)
			{
				hashString(h, f.name);
				const uint64_t argc = f.args.size();
				hashPod(h, argc);
				for (const auto &arg : f.args)
					hashString(h, arg);
			}
		}

		inline float parseFilterLength(const std::string &s) noexcept
		{
			const char *p = s.data();
			const char *end = p + s.size();
			while (p < end && (*p == ' ' || *p == '\t'))
				++p;
			if (p == end)
				return 0.0f;

			float sign = 1.0f;
			if (*p == '-')
			{
				sign = -1.0f;
				++p;
			}
			else if (*p == '+')
			{
				++p;
			}

			float val = 0.0f;
			bool hasDigits = false;
			while (p < end && *p >= '0' && *p <= '9')
			{
				val = val * 10.0f + static_cast<float>(*p - '0');
				hasDigits = true;
				++p;
			}
			if (p < end && *p == '.')
			{
				++p;
				float scale = 0.1f;
				while (p < end && *p >= '0' && *p <= '9')
				{
					val += static_cast<float>(*p - '0') * scale;
					scale *= 0.1f;
					hasDigits = true;
					++p;
				}
			}
			if (!hasDigits)
				return 0.0f;
			if (p < end && (*p == 'e' || *p == 'E'))
				return std::strtof(s.c_str(), nullptr);
			return sign * val;
		}

		inline Color premultipliedWhite(float opacity)
		{
			const uint8_t v = static_cast<uint8_t>(
				std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
			return Color{v, v, v, v};
		}

		inline void blitCachedLayer(IRenderer *renderer,
									TargetHandle target,
									const Rect &srcRect,
									const Rect &region,
									float globalOp,
									bool premultiplied)
		{
			const Rect dst{region.x, region.y, srcRect.width, srcRect.height};
			if (premultiplied)
			{
				renderer->setBlendMode(BlendMode::Premultiplied);
				renderer->drawTarget(target, srcRect, dst, premultipliedWhite(globalOp));
				renderer->setBlendMode(BlendMode::Alpha);
			}
			else
			{
				renderer->drawTarget(target, srcRect, dst, Colors::White.withAlpha(globalOp));
			}
		}
	} // namespace

	void Layout::maybeEvictStaleLayerCache()
	{
		if (!layerCache_ || !layerCache_->target.valid())
			return;

		auto *ctx = context();
		if (!ctx)
			return;
		const auto &pool = ctx->targetPool;
		if (layerCache_->poolGeneration != pool.generation())
		{
			layerCache_->target = {};
			layerCache_->stableFrames = 0;
			layerCache_->dirty = true;
			return;
		}

		const uint64_t now = pool.frameIndex();
		if (now >= layerCache_->lastUsedFrame &&
			(now - layerCache_->lastUsedFrame) >= kLayerEvictGraceFrames)
		{
			releaseLayerCache();
		}
	}

	void Layout::appendSubtreeFingerprint(const Layout &layerRoot,
										  const ComputedStyle &st,
										  bool isRoot,
										  uint64_t &h) const
	{
		// Include node identity, interaction state, and relative geometry.
		const uintptr_t selfPtr = reinterpret_cast<uintptr_t>(this);
		hashPod(h, selfPtr);

		const Vec2 vp = layerRoot.viewportSize();

		const uint8_t stateBits = static_cast<uint8_t>(
			(state.hovered ? 1u : 0u) |
			(state.pressed ? 2u : 0u) |
			(state.focused ? 4u : 0u) |
			(state.checked ? 8u : 0u) |
			(config.enabled ? 16u : 0u));
		hashPod(h, stateBits);

		const float w = rect.width;
		const float hPx = rect.height;
		if (!isRoot)
		{
			hashFloat(h, rect.x - layerRoot.rect.x);
			hashFloat(h, rect.y - layerRoot.rect.y);
		}
		hashFloat(h, w);
		hashFloat(h, hPx);

		// Scroll offset and active transition progress.
		hashFloat(h, scroll_.state.offset.x);
		hashFloat(h, scroll_.state.offset.y);
		hashFloat(h, style_.transitionTimer);

		// Visual style properties.
		hashColor(h, st.background);
		hashColor(h, st.color);
		hashColor(h, st.borderColor);
		hashColor(h, st.tint);
		hashFloat(h, st.opacity);

		const float maxRadius = std::min(w, hPx) * 0.5f;
		const float refDim = maxRadius * 2.0f;
		hashFloat(h, st.radius.resolveH(refDim, refDim, vp));
		hashFloat(h, st.borderWidth.left.resolveH(refDim, refDim, vp));
		hashFloat(h, st.borderWidth.right.resolveH(refDim, refDim, vp));
		hashFloat(h, st.borderWidth.top.resolveH(refDim, refDim, vp));
		hashFloat(h, st.borderWidth.bottom.resolveH(refDim, refDim, vp));
		hashFloat(h, st.fontSize.resolveH(w, hPx, vp));
		hashFloat(h, st.letterSpacing.resolveH(w, hPx, vp));
		hashPod(h, st.textAlign);
		hashPod(h, st.overflowX);
		hashPod(h, st.overflowY);

		hashPod(h, st.boxShadow.enabled);
		if (st.boxShadow.enabled)
		{
			hashColor(h, st.boxShadow.color);
			hashFloat(h, st.boxShadow.x.resolveSelfH(w, hPx, vp));
			hashFloat(h, st.boxShadow.y.resolveSelfV(w, hPx, vp));
			hashFloat(h, st.boxShadow.blur.resolveSelfH(w, hPx, vp));
		}

		// Background texture and attached shaders.
		hashPod(h, bgTexture.id);
		hashPod(h, bgPatchInfo);
		hashString(h, st.backgroundTexture.name);
		hashPod(h, st.backgroundTexture.left);
		hashPod(h, st.backgroundTexture.top);
		hashPod(h, st.backgroundTexture.right);
		hashPod(h, st.backgroundTexture.bottom);
		hashPod(h, hasShader);
		hashPod(h, customEffect.id);
		hashPod(h, currentEffect_.id);
		hashString(h, st.effect);

		if (!isRoot)
		{
			hashFloat(h, st.translateX.resolveSelfH(w, hPx, vp));
			hashFloat(h, st.translateY.resolveSelfV(w, hPx, vp));
			hashFloat(h, st.rotation);
			hashFloat(h, st.scale);
			hashFilters(h, st.filters);
		}

		// Recurse into visible non-portal children in tree order.
		uint32_t visibleChildren = 0;
		for (const auto &child : children)
		{
			if (!child || !child->isVisible() || child->isPortal())
				continue;
			++visibleChildren;
			child->appendSubtreeFingerprint(layerRoot, child->style_.currentStyle, false, h);
		}
		hashPod(h, visibleChildren);
	}

	void Layout::drawWithStyle(const ComputedStyle &renderStyle, float parentOpacity)
	{
		auto *ctx = context();
		auto *renderer = ctx ? ctx->renderer : nullptr;
		if (!renderer)
			return;

		resolveEffectIfNeeded(renderStyle);

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f)
		{
			maybeEvictStaleLayerCache();
			return;
		}

		const bool isLayer =
			!renderStyle.filters.empty() &&
			renderer->supports(Feature::Effects) &&
			rect.width > 0.0f && rect.height > 0.0f &&
			(!renderer->inTarget() || renderer->supports(Feature::NestedTargets));

		if (isLayer)
		{
			drawLayer(renderStyle, globalOp);
		}
		else
		{
			if (renderStyle.filters.empty())
				releaseLayerCache();
			drawInline(renderStyle, globalOp);
		}

		scroll_.drawScrollbar(*this, parentOpacity);
	}

	void Layout::draw(float parentOpacity)
	{
		if (!config.visible)
			return;

		auto *ctx = context();
		if (!ctx)
			return;

		if (config.portal)
		{
			ctx->framePortals.push_back(weak_from_this());
			return;
		}

		auto renderer = ctx->renderer;
		if (!renderer)
			return;

		if (rect.width > 0.0f && rect.height > 0.0f)
		{
			Rect clip = ctx->getCullingClip();
			float m = 40.0f;
			if (rect.x > clip.x + clip.width + m ||
				rect.x + rect.width < clip.x - m ||
				rect.y > clip.y + clip.height + m ||
				rect.y + rect.height < clip.y - m)
			{
				maybeEvictStaleLayerCache();
				return;
			}
		}

		if (anim_.css.empty())
		{
			drawWithStyle(style_.currentStyle, parentOpacity);
		}
		else
		{
			ComputedStyle renderStyle = style_.currentStyle;
			Anim::overlayCssComputed(renderStyle, anim_.css, theme());
			drawWithStyle(renderStyle, parentOpacity);
		}

		if (!hasParent())
		{
			auto portals = ctx->framePortals;
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
		auto *ctx = context();
		auto *renderer = ctx ? ctx->renderer : nullptr;
		if (!renderer)
			return;

		Transform2D tr = currentTransform(renderStyle);
		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;

		if (!identity)
		{
			renderer->pushTransform(tr);
		}
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
		auto *ctxUI = context();
		if (!ctxUI || !ctxUI->renderer)
			return;
		auto *renderer = ctxUI->renderer;

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

		Rect clip = ctxUI->getCullingClip();

		Rect region;
		region.x = std::floor(std::max(transformedRect.x, clip.x));
		region.y = std::floor(std::max(transformedRect.y, clip.y));
		float right = std::ceil(std::min(transformedRect.x + transformedRect.width, clip.x + clip.width));
		float bottom = std::ceil(std::min(transformedRect.y + transformedRect.height, clip.y + clip.height));
		region.width = std::max(0.0f, right - region.x);
		region.height = std::max(0.0f, bottom - region.y);

		if (region.width < 1.0f || region.height < 1.0f)
		{
			maybeEvictStaleLayerCache();
			return;
		}

		const int tw = (int)std::ceil(region.width);
		const int th = (int)std::ceil(region.height);
		if (tw <= 0 || th <= 0)
		{
			maybeEvictStaleLayerCache();
			return;
		}

		constexpr int MAX_TARGET_DIM = TargetPool::MAX_DIM;
		if (tw > MAX_TARGET_DIM || th > MAX_TARGET_DIM)
		{
			releaseLayerCache();
			drawInline(renderStyle, globalOp);
			return;
		}

		auto &stats = ctxUI->renderStats;
		auto &pool = ctxUI->targetPool;

		// Only cache top-level layers when no animation is active anywhere in the subtree.
		// Checking upfront avoids hashing or filter-arg parsing on animated nodes.
		const bool allowCache = !renderer->inTarget() &&
								!renderStyle.filters.empty() &&
								!hasActiveAnimations();

		bool saveToCache = false;
		uint64_t fingerprint = kFnvOffset;

		if (allowCache)
		{
			if (!layerCache_)
				layerCache_ = std::make_unique<LayerCache>();

			auto &cache = *layerCache_;
			const uint64_t poolGen = pool.generation();

			// Drop stale handle if TargetPool::clear() wiped all pooled targets.
			if (cache.target.valid() && cache.poolGeneration != poolGen)
			{
				cache.target = {};
				cache.stableFrames = 0;
				cache.dirty = true;
			}

			const float regOffX = region.x - transformedRect.x;
			const float regOffY = region.y - transformedRect.y;
			const uint32_t themeVer = ctxUI->theme.version;

			// Cheap scalar pre-check: skip subtree fingerprinting completely while
			// transform, size, clip offset, opacity, or global generations are changing.
			const bool cheapMatch =
				cache.stableFrames >= 1 &&
				!cache.dirty &&
				cache.tw == tw &&
				cache.th == th &&
				cache.lastRotation == currTr.rotationDeg &&
				cache.lastScale == currTr.scale &&
				cache.lastOpacity == globalOp &&
				cache.lastRegionOffX == regOffX &&
				cache.lastRegionOffY == regOffY &&
				cache.lastDpiScale == ctxUI->dpiScale &&
				cache.lastViewportGen == ctxUI->viewportGeneration &&
				cache.lastThemeVer == themeVer;

			if (!cheapMatch)
			{
				if (cache.target.valid())
				{
					pool.release(cache.target);
					cache.target = {};
				}
				cache.tw = tw;
				cache.th = th;
				cache.lastRotation = currTr.rotationDeg;
				cache.lastScale = currTr.scale;
				cache.lastOpacity = globalOp;
				cache.lastRegionOffX = regOffX;
				cache.lastRegionOffY = regOffY;
				cache.lastDpiScale = ctxUI->dpiScale;
				cache.lastViewportGen = ctxUI->viewportGeneration;
				cache.lastThemeVer = themeVer;
				cache.stableFrames = 1;
				cache.dirty = false;
			}
			else
			{
				// Root scalars are stable: compute full subtree fingerprint.
				hashFloat(fingerprint, ctxUI->dpiScale);
				hashPod(fingerprint, ctxUI->viewportGeneration);
				hashPod(fingerprint, themeVer);
				hashFloat(fingerprint, currTr.rotationDeg);
				hashFloat(fingerprint, currTr.scale);
				hashFloat(fingerprint, globalOp);
				hashFloat(fingerprint, regOffX);
				hashFloat(fingerprint, regOffY);
				hashFloat(fingerprint, region.width);
				hashFloat(fingerprint, region.height);
				hashPod(fingerprint, tw);
				hashPod(fingerprint, th);
				hashFilters(fingerprint, renderStyle.filters);
				appendSubtreeFingerprint(*this, renderStyle, true, fingerprint);

				// Fast path: reuse the previously baked filtered target.
				if (cache.target.valid() && cache.fingerprint == fingerprint)
				{
					cache.lastUsedFrame = pool.frameIndex();
					blitCachedLayer(renderer, cache.target, cache.srcRect, region,
									globalOp, cache.premultiplied);
					return;
				}

				// Promote to a cached target once two consecutive full fingerprints agree.
				const bool stableMatch =
					cache.stableFrames >= 2 &&
					cache.lastFingerprint == fingerprint;

				if (cache.target.valid())
				{
					pool.release(cache.target);
					cache.target = {};
				}

				if (stableMatch)
				{
					saveToCache = true;
				}
				else
				{
					cache.lastFingerprint = fingerprint;
					cache.stableFrames = 2;
				}
			}
		}
		else
		{
			releaseLayerCache();
		}

		auto &registry = ctxUI->filters;

		const size_t numFilters = renderStyle.filters.size();
		bool lastIsSilhouette = false;
		int outW = tw;
		int outH = th;

		// Parse silhouette shadow extents only on the single frame that bakes into the cache.
		if (saveToCache && numFilters > 0)
		{
			const FilterDef *lastDef = registry.find(renderStyle.filters.back().name);
			if (lastDef && lastDef->pattern == FilterPattern::Silhouette)
			{
				lastIsSilhouette = true;
				float dx = 2.0f;
				float dy = 2.0f;
				const auto &lastRef = renderStyle.filters.back();
				for (const auto &p : lastDef->params)
				{
					if (p.uniform == "dx" && p.argIndex < static_cast<int>(lastRef.args.size()))
						dx = parseFilterLength(lastRef.args[p.argIndex]);
					if (p.uniform == "dy" && p.argIndex < static_cast<int>(lastRef.args.size()))
						dy = parseFilterLength(lastRef.args[p.argIndex]);
				}
				if (dx < 0.0f || dy < 0.0f)
				{
					saveToCache = false;
				}
				else
				{
					outW = tw + static_cast<int>(std::ceil(dx));
					outH = th + static_cast<int>(std::ceil(dy));
					if (outW > MAX_TARGET_DIM || outH > MAX_TARGET_DIM)
						saveToCache = false;
				}
			}
		}

		TargetHandle mainTarget = pool.acquire(*renderer, tw, th, &stats);
		if (!mainTarget.valid())
			return;

		ctxUI->pushLayerTarget(mainTarget);

		Transform2D offsetTr;
		offsetTr.pivot = {0.0f, 0.0f};
		offsetTr.translate = {-region.x, -region.y};

		Transform2D tr = currentTransform(renderStyle);
		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;
		renderer->pushTransform(offsetTr);
		if (!identity)
		{
			renderer->pushTransform(tr);
		}

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
		ctxUI->popLayerTarget();

		const Rect dstRect = {0, 0, (float)tw, (float)th};

		if (renderStyle.filters.empty())
		{
			renderer->drawTarget(mainTarget, dstRect, region,
								 Colors::White.withAlpha(globalOp));
			pool.release(mainTarget);
			return;
		}

		const bool singleSeparable =
			(numFilters == 1) &&
			([&]()
			 {
				 const auto *def = registry.find(renderStyle.filters[0].name);
				 return def && def->pattern == FilterPattern::Separable; }());
		const bool needsScratch = (numFilters >= 2) || (saveToCache && !singleSeparable);
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
			const int sw = (saveToCache && lastIsSilhouette && numFilters == 1) ? outW : tw;
			const int sh = (saveToCache && lastIsSilhouette && numFilters == 1) ? outH : th;
			scratchTarget = pool.acquire(*renderer, sw, sh, &stats);
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
		ctx.ui = ctxUI;
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
			const bool writeOffscreen = saveToCache || !isLast;
			const auto *def = registry.find(renderStyle.filters[i].name);
			// Separable filters read src only in pass 1 (into the intermediate target)
			// and read only the intermediate in pass 2, so writing pass 2 back into
			// currentRead avoids allocating a 3rd full-size FBO.
			const bool writeInPlace =
				writeOffscreen && !currentWrite.valid() &&
				def && def->pattern == FilterPattern::Separable;

			ctx.ref = &renderStyle.filters[i];
			ctx.src = currentRead;
			ctx.texSize = {(float)currentRead.width, (float)currentRead.height};

			if (isLast && saveToCache && lastIsSilhouette &&
				(currentWrite.width < outW || currentWrite.height < outH))
			{
				if (currentWrite.valid())
					pool.release(currentWrite);
				currentWrite = pool.acquire(*renderer, outW, outH, &stats);
				if (!currentWrite.valid())
				{
					if (separableTarget.valid())
						pool.release(separableTarget);
					pool.release(currentRead);
					return;
				}
			}

			ctx.write_target = writeOffscreen
								   ? (writeInPlace ? currentRead : currentWrite)
								   : TargetHandle{};
			ctx.opacity = writeOffscreen ? 1.0f : globalOp;

			if (def)
			{
				applyFilter(*def, ctx);
			}
			else
			{
				if (!writeOffscreen)
				{
					renderer->drawTarget(currentRead, ctx.dst, region,
										 Colors::White.withAlpha(globalOp));
				}
				else
				{
					renderer->pushTarget(currentWrite);
					stats.pushTargetCalls++;
					renderer->setBlendMode(BlendMode::Replace);
					renderer->drawTarget(currentRead, ctx.dst, ctx.dst, Colors::White);
					renderer->setBlendMode(BlendMode::Alpha);
					renderer->popTarget();
				}
			}

			if (writeOffscreen && !writeInPlace)
			{
				std::swap(currentRead, currentWrite);
			}
		}

		if (separableTarget.valid())
			pool.release(separableTarget);

		if (saveToCache && layerCache_)
		{
			if (currentWrite.valid())
				pool.release(currentWrite);

			auto &cache = *layerCache_;
			cache.target = currentRead;
			cache.poolGeneration = pool.generation();
			cache.srcRect = {0.0f, 0.0f, static_cast<float>(outW), static_cast<float>(outH)};
			cache.tw = tw;
			cache.th = th;
			cache.fingerprint = fingerprint;
			cache.lastFingerprint = fingerprint;
			cache.lastUsedFrame = pool.frameIndex();
			cache.stableFrames = 3;
			cache.premultiplied = lastIsSilhouette;
			cache.dirty = false;

			blitCachedLayer(renderer, cache.target, cache.srcRect, region,
							globalOp, cache.premultiplied);
			return;
		}

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
			if (!child->isVisible())
				continue;
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
		auto *ctx = context();
		if (auto *assets = ctx ? ctx->assets : nullptr)
			currentEffect_ = assets->getEffect(style.effect);
		else
			currentEffect_ = {};
	}

} // namespace ZenitUI