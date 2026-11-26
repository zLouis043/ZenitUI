#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

	static Rect intersectRect(const Rect &a, const Rect &b)
	{
		float x1 = std::max(a.x, b.x);
		float y1 = std::max(a.y, b.y);
		float x2 = std::min(a.x + a.width, b.x + b.width);
		float y2 = std::min(a.y + a.height, b.y + b.height);
		return {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
	}

	// Resolve the effective minimum cross-axis size for a flex item.
	// CSS default: min-height/min-width auto resolves to the content size
	// when overflow is visible, and to 0 otherwise. An explicit value
	// overrides the auto rule.
	inline float resolveFlexMin(const ComputedStyle &cst, bool isVerticalAxis,
								float contentSize, float parentW, float parentH, Vec2 viewport)
	{
		const Value &mv = isVerticalAxis ? cst.minHeight : cst.minWidth;
		const Overflow ov = isVerticalAxis ? cst.overflowY : cst.overflowX;
		if (!mv.isAuto())
			return isVerticalAxis ? mv.resolveV(parentW, parentH, viewport)
								  : mv.resolveH(parentW, parentH, viewport);
		return (ov == Overflow::Visible) ? contentSize : 0.0f;
	}

	struct FlexItem
	{
		float base;
		float minS;
		float maxS;
		float grow;
		float shrink;
	};

	// CSS Flexbox §9.7: iteratively distribute free space, freezing any item
	// that hits its min/max constraint and redistributing the remainder to
	// still-flexible siblings.
	static std::vector<float> resolveFlexSizes(const std::vector<FlexItem> &items,
											   float availMain)
	{
		const size_t n = items.size();
		std::vector<float> sizes(n, 0.0f);
		std::vector<bool> frozen(n, false);

		auto clamp = [](float v, float lo, float hi) -> float
		{
			return (hi < lo) ? hi : std::clamp(v, lo, hi);
		};

		float baseSum = 0.0f;
		for (const auto &it : items)
			baseSum += it.base;

		const bool growing = (availMain >= baseSum);

		for (size_t i = 0; i < n; ++i)
		{
			const float factor = growing ? items[i].grow : items[i].shrink;
			if (factor <= 0.0f)
			{
				sizes[i] = clamp(items[i].base, items[i].minS, items[i].maxS);
				frozen[i] = true;
			}
			else
			{
				sizes[i] = items[i].base;
			}
		}

		for (size_t iter = 0; iter <= n; ++iter)
		{
			float remainingFree = availMain;
			float totalFlex = 0.0f;
			size_t unfrozen = 0;

			for (size_t i = 0; i < n; ++i)
			{
				if (frozen[i])
				{
					remainingFree -= sizes[i];
				}
				else
				{
					remainingFree -= items[i].base;
					totalFlex += growing ? items[i].grow : items[i].shrink;
					++unfrozen;
				}
			}

			if (unfrozen == 0 || totalFlex <= 0.0f)
				break;

			if ((growing && remainingFree <= 0.0f) || (!growing && remainingFree >= 0.0f))
			{
				for (size_t i = 0; i < n; ++i)
					if (!frozen[i])
						sizes[i] = clamp(items[i].base, items[i].minS, items[i].maxS);
				break;
			}

			bool anyClamped = false;
			for (size_t i = 0; i < n; ++i)
			{
				if (frozen[i])
					continue;
				const float factor = growing ? items[i].grow : items[i].shrink;
				const float target = items[i].base + remainingFree * (factor / totalFlex);
				const float clamped = clamp(target, items[i].minS, items[i].maxS);
				sizes[i] = clamped;
				if (std::fabs(clamped - target) > 1e-4f)
				{
					frozen[i] = true;
					anyClamped = true;
				}
			}

			if (!anyClamped)
				break;
		}

		return sizes;
	}

	Vec2 Layout::measure(float parent_w, float parent_h)
	{
		if (!config.visible)
		{
			measuredSize = {0.0f, 0.0f};
			lastMeasureW_ = parent_w;
			lastMeasureH_ = parent_h;
			return measuredSize;
		}

		const Vec2 vp = viewportSize();

		const bool forcedW = (forcedMeasureW_ >= 0.0f);
		if (!forcedW && !internal_.subtreeDirty && parent_w == lastMeasureW_ && parent_h == lastMeasureH_)
			return measuredSize;
		lastMeasureW_ = forcedW ? -1.0f : parent_w;
		lastMeasureH_ = forcedW ? -1.0f : parent_h;

		if (!internal_.styleInitialized)
		{
			internal_.styleInitialized = 1;
			style_.currentStyle = style_.targetStyle = style_.transitionStartStyle = style_.resolveFor(*this);
			internal_.pendingTransition = 0;
		}

		ComputedStyle calcStyle = internal_.pendingTransition ? style_.resolveFor(*this) : style_.currentStyle;
		float pl = calcStyle.padding.left.resolveH(parent_w, parent_h, vp);
		float pr = calcStyle.padding.right.resolveH(parent_w, parent_h, vp);
		float pt = calcStyle.padding.top.resolveV(parent_w, parent_h, vp);
		float pb = calcStyle.padding.bottom.resolveV(parent_w, parent_h, vp);
		const auto &bws = calcStyle.borderWidth;
		float bwl = bws.left.resolveH(parent_w, parent_h, vp);
		float bwr = bws.right.resolveH(parent_w, parent_h, vp);
		float bwt = bws.top.resolveV(parent_w, parent_h, vp);
		float bwb = bws.bottom.resolveV(parent_w, parent_h, vp);

		float availW = forcedW
						   ? forcedMeasureW_
						   : (calcStyle.width.isAuto() ? parent_w : calcStyle.width.resolveH(parent_w, parent_h, vp));
		float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolveV(parent_w, parent_h, vp);
		if (!forcedW && !calcStyle.maxWidth.isAuto())
			availW = std::min(availW, calcStyle.maxWidth.resolveH(parent_w, parent_h, vp));
		if (!calcStyle.maxHeight.isAuto())
			availH = std::min(availH, calcStyle.maxHeight.resolveV(parent_w, parent_h, vp));
		float innerW = std::max(0.0f, availW - pl - pr - bwl - bwr);
		float innerH = std::max(0.0f, availH - pt - pb - bwt - bwb);

		if (getEffectiveLayoutType() == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
			innerH = 1e9f;
		if (getEffectiveLayoutType() == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
			innerW = 1e9f;

		Vec2 contentSize = computeIntrinsicSize(innerW, innerH);
		float gapX = calcStyle.gap.resolveH(innerW, innerH, vp);
		float gapY = calcStyle.gap.resolveV(innerW, innerH, vp);

		if (getEffectiveLayoutType() == LayoutType::Horizontal && calcStyle.wrap)
		{
			float curX = 0, curY = 0, maxW = 0, maxLineH = 0;
			int visibleInLine = 0;
			for (auto &c : children)
			{
				if (!c->isVisible())
					continue;
				Vec2 cs = c->measure(innerW, innerH);
				if (c->getStyle().position == Position::Absolute || c->isPortal())
					continue;
				const auto &cst = c->getStyle();
				float ml = cst.margin.left.resolveH(innerW, innerH, vp);
				float mr = cst.margin.right.resolveH(innerW, innerH, vp);
				float mt = cst.margin.top.resolveV(innerW, innerH, vp);
				float mb = cst.margin.bottom.resolveV(innerW, innerH, vp);
				float baseW = cst.flexBasis.isAuto() ? cs.x : cst.flexBasis.resolveH(innerW, innerH, vp);
				float childW = baseW + ml + mr;
				float childH = cs.y + mt + mb;
				float spaceNeeded = childW + (visibleInLine > 0 ? gapX : 0.0f);
				if (visibleInLine > 0 && curX + spaceNeeded > innerW && innerW > 0.0f)
				{
					maxW = std::max(maxW, curX);
					curY += maxLineH + gapY;
					curX = 0;
					maxLineH = 0;
					visibleInLine = 0;
					spaceNeeded = childW;
				}
				if (visibleInLine > 0)
					curX += gapX;
				curX += childW;
				maxLineH = std::max(maxLineH, childH);
				visibleInLine++;
			}
			maxW = std::max(maxW, curX);
			contentSize.x = maxW;
			contentSize.y = curY + maxLineH;
		}
		else if (getEffectiveLayoutType() == LayoutType::Horizontal)
		{
			float fixedMainSize = 0.0f;
			float maxCrossSize = 0.0f;
			float totalGrow = 0.0f;
			int visibleChildren = 0;

			for (auto &c : children)
			{
				if (!c->isVisible())
					continue;
				c->initStyleIfNeeded();
				if (c->internal_.pendingTransition)
					c->style_.currentStyle = c->style_.resolveFor(*c);

				const auto &cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal())
				{
					c->measure(innerW, innerH);
					continue;
				}
				visibleChildren++;
				if (cst.grow > 0.0f && cst.width.isAuto() && innerW < 1e8f)
				{
					totalGrow += cst.grow;
				}
				else
				{
					Vec2 cs = c->measure(innerW, innerH);
					float ml = cst.margin.left.resolveH(innerW, innerH, vp);
					float mr = cst.margin.right.resolveH(innerW, innerH, vp);
					float mt = cst.margin.top.resolveV(innerW, innerH, vp);
					float mb = cst.margin.bottom.resolveV(innerW, innerH, vp);
					fixedMainSize += cs.x + ml + mr;
					maxCrossSize = std::max(maxCrossSize, cs.y + mt + mb);
				}
			}

			float totalMainSize = fixedMainSize;
			float totalGaps = (visibleChildren > 1) ? gapX * (visibleChildren - 1) : 0.0f;
			float remW = std::max(0.0f, innerW - fixedMainSize - totalGaps);

			if (totalGrow > 0.0f)
			{
				for (auto &c : children)
				{
					if (!c->isVisible())
						continue;
					const auto &cst = c->getStyle();
					if (cst.position == Position::Absolute || c->isPortal())
						continue;
					if (cst.grow > 0.0f && cst.width.isAuto() && innerW < 1e8f)
					{
						float ml = cst.margin.left.resolveH(innerW, innerH, vp);
						float mr = cst.margin.right.resolveH(innerW, innerH, vp);
						float mt = cst.margin.top.resolveV(innerW, innerH, vp);
						float mb = cst.margin.bottom.resolveV(innerW, innerH, vp);
						float shareW = remW * (cst.grow / totalGrow);
						float childAvailW = std::max(0.0f, shareW - ml - mr);
						Vec2 cs = c->measure(childAvailW, innerH);
						totalMainSize += cs.x + ml + mr;
						maxCrossSize = std::max(maxCrossSize, cs.y + mt + mb);
					}
				}
			}

			if (innerW < 1e8f && visibleChildren > 0)
			{
				std::vector<FlexItem> flexItems;
				flexItems.reserve(visibleChildren);
				float totalMarginsX = 0.0f;
				float totalBaseMain = 0.0f;

				for (auto &c : children)
				{
					if (!c->isVisible())
						continue;
					const auto &cst = c->getStyle();
					if (cst.position == Position::Absolute || c->isPortal())
						continue;

					float ml = cst.margin.left.isAuto() ? 0.0f : cst.margin.left.resolveH(innerW, innerH, vp);
					float mr = cst.margin.right.isAuto() ? 0.0f : cst.margin.right.resolveH(innerW, innerH, vp);
					float baseW = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(innerW, innerH, vp);
					float minW = resolveFlexMin(cst, false, c->measuredSize.x, innerW, innerH, vp);
					float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(innerW, innerH, vp);

					flexItems.push_back({baseW, minW, maxW, cst.grow, cst.shrink});
					totalMarginsX += ml + mr;
					totalBaseMain += baseW + ml + mr;
				}

				if (totalBaseMain + totalGaps > innerW + 0.5f)
				{
					float availMain = innerW - totalMarginsX - totalGaps;
					std::vector<float> shrunkWidths = resolveFlexSizes(flexItems, availMain);

					bool anyResized = false;
					size_t flowIdx = 0;
					for (auto &c : children)
					{
						if (!c->isVisible())
							continue;
						const auto &cst = c->getStyle();
						if (cst.position == Position::Absolute || c->isPortal())
							continue;

						float targetW = shrunkWidths[flowIdx++];
						if (targetW < c->measuredSize.x - 0.5f)
						{
							c->measureConstrainedW(targetW, innerW, innerH);
							anyResized = true;
						}
					}

					if (anyResized)
					{
						totalMainSize = 0.0f;
						maxCrossSize = 0.0f;
						for (auto &c : children)
						{
							if (!c->isVisible())
								continue;
							const auto &cst = c->getStyle();
							if (cst.position == Position::Absolute || c->isPortal())
								continue;

							float ml = cst.margin.left.isAuto() ? 0.0f : cst.margin.left.resolveH(innerW, innerH, vp);
							float mr = cst.margin.right.isAuto() ? 0.0f : cst.margin.right.resolveH(innerW, innerH, vp);
							float mt = cst.margin.top.isAuto() ? 0.0f : cst.margin.top.resolveV(innerW, innerH, vp);
							float mb = cst.margin.bottom.isAuto() ? 0.0f : cst.margin.bottom.resolveV(innerW, innerH, vp);
							totalMainSize += c->measuredSize.x + ml + mr;
							maxCrossSize = std::max(maxCrossSize, c->measuredSize.y + mt + mb);
						}
					}
				}
			}

			totalMainSize += totalGaps;
			contentSize.x = std::max(contentSize.x, totalMainSize);
			contentSize.y = std::max(contentSize.y, maxCrossSize);
		}
		else
		{
			float totalMainSize = 0.0f;
			float maxCrossSize = 0.0f;
			int visibleChildren = 0;
			for (auto &c : children)
			{
				if (!c->isVisible())
					continue;
				Vec2 cs = c->measure(innerW, innerH);
				if (c->getStyle().position == Position::Absolute || c->isPortal())
					continue;
				const auto &cst = c->getStyle();
				float ml = cst.margin.left.resolveH(innerW, innerH, vp);
				float mr = cst.margin.right.resolveH(innerW, innerH, vp);
				float mt = cst.margin.top.resolveV(innerW, innerH, vp);
				float mb = cst.margin.bottom.resolveV(innerW, innerH, vp);
				float childTotalW = cs.x + ml + mr;
				float childTotalH = cs.y + mt + mb;
				if (getEffectiveLayoutType() == LayoutType::Vertical)
				{
					totalMainSize += childTotalH;
					maxCrossSize = std::max(maxCrossSize, childTotalW);
				}
				else if (getEffectiveLayoutType() == LayoutType::Stack)
				{
					contentSize.x = std::max(contentSize.x, childTotalW);
					contentSize.y = std::max(contentSize.y, childTotalH);
				}
				visibleChildren++;
			}
			if (getEffectiveLayoutType() == LayoutType::Vertical)
			{
				if (visibleChildren > 1)
					totalMainSize += gapY * (visibleChildren - 1);
				contentSize.x = std::max(contentSize.x, maxCrossSize);
				contentSize.y = std::max(contentSize.y, totalMainSize);
			}
		}

		scroll_.contentSize = contentSize;

		float finalW = calcStyle.width.isAuto()
						   ? (forcedW ? std::min(availW, contentSize.x + pl + pr + bwl + bwr)
									  : (contentSize.x + pl + pr + bwl + bwr))
						   : availW;
		float finalH = calcStyle.height.isAuto()
						   ? (contentSize.y + pt + pb + bwt + bwb)
						   : availH;

		float minW = calcStyle.minWidth.isAuto() ? 0.0f : calcStyle.minWidth.resolveH(parent_w, parent_h, vp);
		float maxW = calcStyle.maxWidth.isAuto() ? 1e9f : calcStyle.maxWidth.resolveH(parent_w, parent_h, vp);
		float minH = calcStyle.minHeight.isAuto() ? 0.0f : calcStyle.minHeight.resolveV(parent_w, parent_h, vp);
		float maxH = calcStyle.maxHeight.isAuto() ? 1e9f : calcStyle.maxHeight.resolveV(parent_w, parent_h, vp);

		float clampedW = forcedW
							 ? clampSafe(finalW, minW, std::min(availW, maxW))
							 : clampSafe(finalW, minW, maxW);
		measuredSize = {clampedW, clampSafe(finalH, minH, maxH)};
		return measuredSize;
	}

	void Layout::arrange(Rect space)
	{
		if (!config.visible)
		{
			rect = {0.0f, 0.0f, 0.0f, 0.0f};
			return;
		}
		const bool rectChanged = (space.x != rect.x || space.y != rect.y ||
								  space.width != rect.width || space.height != rect.height);
		rect = space;

		if (isScrollContainer() && getEffectiveLayoutType() != LayoutType::Stack)
		{
			if (scroll_.needsFullArrange(*this, rectChanged))
			{
				Rect arrangeSpace = scroll_.computeArrangeSpace(*this, space);
				arrangeInto(arrangeSpace);
				scroll_.arrangeInitialized = true;
				scroll_.appliedOffset = {0.0f, 0.0f};
				scroll_.applyOffsetDelta(*this);
				scroll_.updateMaxScroll(*this, space);
			}
			else
			{
				scroll_.applyOffsetDelta(*this);
				scroll_.updateMaxScroll(*this, space);

				auto syncNestedScrolls = [](auto &self, Layout &node) -> void
				{
					for (auto &c : node.children)
					{
						if (!c || c->isPortal())
							continue;
						if (c->isScrollContainer() && c->getEffectiveLayoutType() != LayoutType::Stack)
						{
							c->scroll_.applyOffsetDelta(*c);
							c->scroll_.updateMaxScroll(*c, c->rect);
						}
						self(self, *c);
					}
				};
				syncNestedScrolls(syncNestedScrolls, *this);
			}
		}
		else
		{
			arrangeInto(space);
			scroll_.arrangeInitialized = true;
		}

		onLayout();
	}

	void Layout::translateSubtree(float dx, float dy)
	{
		for (auto &c : children)
		{
			if (c->isPortal())
				continue;
			c->rect.x += dx;
			c->rect.y += dy;
			c->translateSubtree(dx, dy);
		}
	}

	void Layout::arrangeInto(Rect space)
	{
		const Vec2 vp = viewportSize();
		float pl = style_.currentStyle.padding.left.resolveH(space.width, space.height, vp);
		float pr = style_.currentStyle.padding.right.resolveH(space.width, space.height, vp);
		float pt = style_.currentStyle.padding.top.resolveV(space.width, space.height, vp);
		float pb = style_.currentStyle.padding.bottom.resolveV(space.width, space.height, vp);
		const auto &bws = style_.currentStyle.borderWidth;
		float bwl = bws.left.resolveH(space.width, space.height, vp);
		float bwr = bws.right.resolveH(space.width, space.height, vp);
		float bwt = bws.top.resolveV(space.width, space.height, vp);
		float bwb = bws.bottom.resolveV(space.width, space.height, vp);

		Rect inner = {
			space.x + pl + bwl,
			space.y + pt + bwt,
			std::max(0.0f, space.width - pl - pr - bwl - bwr),
			std::max(0.0f, space.height - pt - pb - bwt - bwb)};

		float gapX = style_.currentStyle.gap.resolveH(inner.width, inner.height, vp);
		float gapY = style_.currentStyle.gap.resolveV(inner.width, inner.height, vp);

		arrangeAbsoluteAndPortals(inner);

		if (getEffectiveLayoutType() == LayoutType::Vertical)
			arrangeVerticalFlow(inner, gapY);
		else if (getEffectiveLayoutType() == LayoutType::Horizontal)
		{
			if (style_.currentStyle.wrap)
				arrangeWrapFlow(inner, gapX, gapY);
			else
				arrangeHorizontalFlow(inner, gapX);
		}
		else if (getEffectiveLayoutType() == LayoutType::Stack)
			arrangeStackFlow(inner);
	}

	float Layout::clampSafe(float v, float lo, float hi)
	{
		if (hi < lo)
			return hi;
		return std::clamp(v, lo, hi);
	}

	void Layout::arrangeAbsoluteAndPortals(const Rect &inner)
	{
		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (c->isPortal())
			{
				c->arrange({inner.x, inner.y, c->measuredSize.x, c->measuredSize.y});
				continue;
			}
			if (cst.position == Position::Absolute)
				arrangeAbsolute(c.get(), cst, inner);
		}
	}

	void Layout::arrangeAbsolute(Layout *c, const ComputedStyle &cst, const Rect &in)
	{
		const Vec2 vp = viewportSize();
		float ml = cst.margin.left.resolveH(in.width, in.height, vp);
		float mr = cst.margin.right.resolveH(in.width, in.height, vp);
		float mt = cst.margin.top.resolveV(in.width, in.height, vp);
		float mb = cst.margin.bottom.resolveV(in.width, in.height, vp);

		bool hasL = !cst.left.isAuto();
		bool hasR = !cst.right.isAuto();
		bool hasT = !cst.top.isAuto();
		bool hasB = !cst.bottom.isAuto();

		float w = c->measuredSize.x;
		float h = c->measuredSize.y;
		float x, y;

		if (hasL && hasR && cst.width.isAuto())
		{
			float L = cst.left.resolveH(in.width, in.height, vp);
			float R = cst.right.resolveH(in.width, in.height, vp);
			w = std::max(0.0f, in.width - L - R - ml - mr);
			x = in.x + L + ml;
		}
		else if (hasL)
		{
			x = in.x + cst.left.resolveH(in.width, in.height, vp) + ml;
		}
		else if (hasR)
		{
			x = in.x + in.width - w - cst.right.resolveH(in.width, in.height, vp) - mr;
		}
		else
		{
			x = in.x + ml;
		}

		if (hasT && hasB && cst.height.isAuto())
		{
			float T = cst.top.resolveV(in.width, in.height, vp);
			float B = cst.bottom.resolveV(in.width, in.height, vp);
			h = std::max(0.0f, in.height - T - B - mt - mb);
			y = in.y + T + mt;
		}
		else if (hasT)
		{
			y = in.y + cst.top.resolveV(in.width, in.height, vp) + mt;
		}
		else if (hasB)
		{
			y = in.y + in.height - h - cst.bottom.resolveV(in.width, in.height, vp) - mb;
		}
		else
		{
			y = in.y + mt;
		}

		c->arrange({x, y, w, h});
	}

	void Layout::arrangeVerticalFlow(const Rect &inner, float gapY)
	{
		std::vector<FlexItem> flexItems;
		flexItems.reserve(children.size());

		float totalMarginsY = 0.0f;
		float totalGrow = 0.0f;
		int visibleChildren = 0;
		int totalAutoY = 0;

		const Vec2 vp = viewportSize();

		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float mt = cst.margin.top.isAuto() ? 0.0f : cst.margin.top.resolveV(inner.width, inner.height, vp);
			float mb = cst.margin.bottom.isAuto() ? 0.0f : cst.margin.bottom.resolveV(inner.width, inner.height, vp);

			if (cst.margin.top.isAuto())
				totalAutoY++;
			if (cst.margin.bottom.isAuto())
				totalAutoY++;

			float baseH = cst.flexBasis.isAuto() ? c->measuredSize.y : cst.flexBasis.resolveV(inner.width, inner.height, vp);
			float minH = resolveFlexMin(cst, true, c->measuredSize.y, inner.width, inner.height, vp);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height, vp);

			flexItems.push_back({baseH, minH, maxH, cst.grow, cst.shrink});
			totalMarginsY += mt + mb;
			totalGrow += cst.grow;
			visibleChildren++;
		}

		float totalGaps = (visibleChildren > 1) ? gapY * (visibleChildren - 1) : 0.0f;
		float availMain = inner.height - totalMarginsY - totalGaps;
		std::vector<float> finalHeights = resolveFlexSizes(flexItems, availMain);

		float actualUsedHeight = totalMarginsY + totalGaps;
		for (float h : finalHeights)
			actualUsedHeight += h;

		float freeSpace = inner.height - actualUsedHeight;
		float startY = inner.y;
		float extraGap = 0.0f;
		float autoMarginYSize = 0.0f;

		if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0)
		{
			if (totalAutoY > 0)
			{
				autoMarginYSize = freeSpace / totalAutoY;
			}
			else
			{
				switch (style_.currentStyle.justify)
				{
				case Justify::Center:
					startY += freeSpace * 0.5f;
					break;
				case Justify::End:
					startY += freeSpace;
					break;
				case Justify::SpaceBetween:
					if (visibleChildren > 1)
						extraGap = freeSpace / (visibleChildren - 1);
					break;
				default:
					break;
				}
			}
		}

		float curY = startY;
		size_t flowIdx = 0;
		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.isAuto() ? 0.0f : cst.margin.left.resolveH(inner.width, inner.height, vp);
			float mr = cst.margin.right.isAuto() ? 0.0f : cst.margin.right.resolveH(inner.width, inner.height, vp);
			float mt = (cst.margin.top.isAuto() ? autoMarginYSize : cst.margin.top.resolveV(inner.width, inner.height, vp));
			float mb = (cst.margin.bottom.isAuto() ? autoMarginYSize : cst.margin.bottom.resolveV(inner.width, inner.height, vp));

			float ch = finalHeights[flowIdx++];
			float cw = c->measuredSize.x;

			int autoX = (cst.margin.left.isAuto() ? 1 : 0) + (cst.margin.right.isAuto() ? 1 : 0);
			float freeW = std::max(0.0f, inner.width - cw - ml - mr);
			if (autoX > 0 && freeW > 0.0f)
			{
				if (cst.margin.left.isAuto())
					ml += freeW / autoX;
				if (cst.margin.right.isAuto())
					mr += freeW / autoX;
			}

			float availW = std::max(0.0f, inner.width - ml - mr);
			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height, vp);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height, vp);
			cw = clampSafe(cw, minW, std::min(availW, maxW));

			float cx = inner.x + ml;
			Align aH = (autoX > 0) ? Align::Start : ((cst.alignH == Align::Auto) ? style_.currentStyle.alignItems : cst.alignH);

			if (aH == Align::Stretch && cst.width.isAuto())
				cw = clampSafe(availW, minW, std::min(availW, maxW));
			else if (aH == Align::Center)
				cx += (availW - cw) * 0.5f;
			else if (aH == Align::End)
				cx += availW - cw;

			c->arrange({cx, curY + mt, cw, ch});
			curY += mt + ch + mb + gapY + extraGap;
		}
	}

	void Layout::arrangeHorizontalFlow(const Rect &inner, float gapX)
	{
		std::vector<FlexItem> flexItems;
		flexItems.reserve(children.size());

		float totalMarginsX = 0.0f;
		float totalGrow = 0.0f;
		int visibleChildren = 0;
		int totalAutoX = 0;
		const Vec2 vp = viewportSize();

		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.isAuto() ? 0.0f : cst.margin.left.resolveH(inner.width, inner.height, vp);
			float mr = cst.margin.right.isAuto() ? 0.0f : cst.margin.right.resolveH(inner.width, inner.height, vp);

			if (cst.margin.left.isAuto())
				totalAutoX++;
			if (cst.margin.right.isAuto())
				totalAutoX++;

			float baseW = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(inner.width, inner.height, vp);
			float minW = resolveFlexMin(cst, false, c->measuredSize.x, inner.width, inner.height, vp);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height, vp);

			flexItems.push_back({baseW, minW, maxW, cst.grow, cst.shrink});
			totalMarginsX += ml + mr;
			totalGrow += cst.grow;
			visibleChildren++;
		}

		float totalGaps = (visibleChildren > 1) ? gapX * (visibleChildren - 1) : 0.0f;
		float availMain = inner.width - totalMarginsX - totalGaps;
		std::vector<float> finalWidths = resolveFlexSizes(flexItems, availMain);

		float actualUsedWidth = totalMarginsX + totalGaps;
		for (float w : finalWidths)
			actualUsedWidth += w;

		float finalFreeSpace = inner.width - actualUsedWidth;
		float startX = inner.x;
		float extraGap = 0.0f;
		float autoMarginXSize = 0.0f;

		if (finalFreeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0)
		{
			if (totalAutoX > 0)
			{
				autoMarginXSize = finalFreeSpace / totalAutoX;
			}
			else
			{
				switch (style_.currentStyle.justify)
				{
				case Justify::Center:
					startX += finalFreeSpace * 0.5f;
					break;
				case Justify::End:
					startX += finalFreeSpace;
					break;
				case Justify::SpaceBetween:
					if (visibleChildren > 1)
						extraGap = finalFreeSpace / (visibleChildren - 1);
					break;
				default:
					break;
				}
			}
		}

		float curX = startX;
		size_t flowIdx = 0;

		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (!c->isVisible() || cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.isAuto() ? autoMarginXSize : cst.margin.left.resolveH(inner.width, inner.height, vp);
			float mr = cst.margin.right.isAuto() ? autoMarginXSize : cst.margin.right.resolveH(inner.width, inner.height, vp);
			float mt = cst.margin.top.isAuto() ? 0.0f : cst.margin.top.resolveV(inner.width, inner.height, vp);
			float mb = cst.margin.bottom.isAuto() ? 0.0f : cst.margin.bottom.resolveV(inner.width, inner.height, vp);

			float cw = finalWidths[flowIdx++];
			float ch = c->measuredSize.y;

			int autoY = (cst.margin.top.isAuto() ? 1 : 0) + (cst.margin.bottom.isAuto() ? 1 : 0);
			float freeH = std::max(0.0f, inner.height - ch - mt - mb);
			if (autoY > 0 && freeH > 0.0f)
			{
				if (cst.margin.top.isAuto())
					mt += freeH / autoY;
				if (cst.margin.bottom.isAuto())
					mb += freeH / autoY;
			}

			float availH = std::max(0.0f, inner.height - mt - mb);
			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height, vp);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height, vp);
			ch = clampSafe(ch, minH, std::min(availH, maxH));

			float cy = inner.y + mt;
			Align aV = (autoY > 0) ? Align::Start : ((cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV);
			if (aV == Align::Stretch && cst.height.isAuto())
				ch = clampSafe(availH, minH, std::min(availH, maxH));
			else if (aV == Align::Center)
				cy += (availH - ch) * 0.5f;
			else if (aV == Align::End)
				cy += availH - ch;

			c->arrange({curX + ml, cy, cw, ch});
			curX += ml + cw + mr + gapX + extraGap;
		}
	}

	void Layout::arrangeStackFlow(const Rect &inner)
	{
		const Vec2 vp = viewportSize();
		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.isAuto() ? 0.0f : cst.margin.left.resolveH(inner.width, inner.height, vp);
			float mr = cst.margin.right.isAuto() ? 0.0f : cst.margin.right.resolveH(inner.width, inner.height, vp);
			float mt = cst.margin.top.isAuto() ? 0.0f : cst.margin.top.resolveV(inner.width, inner.height, vp);
			float mb = cst.margin.bottom.isAuto() ? 0.0f : cst.margin.bottom.resolveV(inner.width, inner.height, vp);

			float availW = std::max(0.0f, inner.width - ml - mr);
			float availH = std::max(0.0f, inner.height - mt - mb);

			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height, vp);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height, vp);
			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height, vp);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height, vp);

			float cw = clampSafe(c->measuredSize.x, minW, std::min(availW, maxW));
			float ch = clampSafe(c->measuredSize.y, minH, std::min(availH, maxH));

			int autoX = (cst.margin.left.isAuto() ? 1 : 0) + (cst.margin.right.isAuto() ? 1 : 0);
			int autoY = (cst.margin.top.isAuto() ? 1 : 0) + (cst.margin.bottom.isAuto() ? 1 : 0);

			float freeW = std::max(0.0f, inner.width - cw - ml - mr);
			float freeH = std::max(0.0f, inner.height - ch - mt - mb);

			if (autoX > 0 && freeW > 0.0f)
			{
				if (cst.margin.left.isAuto())
					ml += freeW / autoX;
				if (cst.margin.right.isAuto())
					mr += freeW / autoX;
			}
			if (autoY > 0 && freeH > 0.0f)
			{
				if (cst.margin.top.isAuto())
					mt += freeH / autoY;
				if (cst.margin.bottom.isAuto())
					mb += freeH / autoY;
			}

			float cx = inner.x + ml;
			float cy = inner.y + mt;

			Align aH = (autoX > 0) ? Align::Start : ((cst.alignH == Align::Auto) ? style_.currentStyle.alignItems : cst.alignH);
			Align aV = (autoY > 0) ? Align::Start : ((cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV);

			if (aH == Align::Stretch && cst.width.isAuto())
				cw = clampSafe(availW, minW, std::min(availW, maxW));
			else if (aH == Align::Center)
				cx += (availW - cw) * 0.5f;
			else if (aH == Align::End)
				cx += availW - cw;

			if (aV == Align::Stretch && cst.height.isAuto())
				ch = clampSafe(availH, minH, std::min(availH, maxH));
			else if (aV == Align::Center)
				cy += (availH - ch) * 0.5f;
			else if (aV == Align::End)
				cy += availH - ch;

			c->arrange({cx, cy, cw, ch});
		}
	}

	Layout *Layout::hitTest(Vec2 p, bool ancestorBlocked, const Rect *clipLocal)
	{
		if (ancestorBlocked || !config.enabled || !config.visible)
			return nullptr;

		if (clipLocal && !clipLocal->contains(p))
			return nullptr;

		if (!hasParent())
		{
			for (auto it = UIContext::get().activePortals.rbegin();
				 it != UIContext::get().activePortals.rend(); ++it)
			{
				auto sp = it->lock();
				if (!sp)
					continue;
				bool wasPortal = sp->isPortal();
				sp->setPortal(false);
				Layout *hit = sp->hitTest(p, false, nullptr);
				sp->setPortal(wasPortal);
				if (hit)
					return hit;
			}
		}
		if (config.portal)
			return nullptr;

		Transform2D tr = currentTransform(style_.currentStyle);
		Vec2 pChildren = applyInverseTransform(tr, p);

		const bool needsClip =
			style_.currentStyle.overflowX != Overflow::Visible ||
			style_.currentStyle.overflowY != Overflow::Visible;

		Rect newClipStorage;
		const Rect *newClip = clipLocal;
		const Vec2 vp = viewportSize();
		if (needsClip)
		{
			Rect base = clipLocal
							? *clipLocal
							: Rect{0, 0, vp.x, vp.y};
			newClipStorage = intersectRect(base, rect);
			newClip = &newClipStorage;
		}

		std::vector<Layout *> negZ, normalFlow, posZ;
		for (auto &child : children)
		{
			if (child->isStackingContext())
			{
				if (child->getZIndex() < 0)
					negZ.push_back(child.get());
				else
					posZ.push_back(child.get());
			}
			else
			{
				normalFlow.push_back(child.get());
			}
		}
		auto sortByZ = [](Layout *a, Layout *b)
		{ return a->getZIndex() < b->getZIndex(); };
		std::stable_sort(negZ.begin(), negZ.end(), sortByZ);
		std::stable_sort(posZ.begin(), posZ.end(), sortByZ);

		for (auto it = posZ.rbegin(); it != posZ.rend(); ++it)
			if (Layout *hit = (*it)->hitTest(pChildren, false, newClip))
				return hit;
		for (auto it = normalFlow.rbegin(); it != normalFlow.rend(); ++it)
			if (Layout *hit = (*it)->hitTest(pChildren, false, newClip))
				return hit;
		for (auto it = negZ.rbegin(); it != negZ.rend(); ++it)
			if (Layout *hit = (*it)->hitTest(pChildren, false, newClip))
				return hit;

		if (rect.width > 0 && rect.height > 0 &&
			rect.contains(p) &&
			(config.interactive || config.blocksRaycast))
		{
			return this;
		}
		return nullptr;
	}

	bool Layout::scrollIntoView(float margin)
	{
		return scrollIntoView(getRect(), margin);
	}

	bool Layout::scrollIntoView(const Rect &subRect, float margin)
	{
		Layout *container = getParent().get();
		while (container)
		{
			if (container->isScrollContainer())
				break;
			container = container->getParent().get();
		}
		if (!container)
			return false;

		const Rect &view = container->rect;

		float dx = 0.0f;
		float dy = 0.0f;

		if (subRect.x < view.x + margin)
			dx = subRect.x - (view.x + margin);
		else if (subRect.x + subRect.width > view.x + view.width - margin)
			dx = (subRect.x + subRect.width) - (view.x + view.width - margin);

		if (subRect.y < view.y + margin)
			dy = subRect.y - (view.y + margin);
		else if (subRect.y + subRect.height > view.y + view.height - margin)
			dy = (subRect.y + subRect.height) - (view.y + view.height - margin);

		if (dx == 0.0f && dy == 0.0f)
			return false;

		float targetX = container->getScrollX() + dx;
		float targetY = container->getScrollY() + dy;

		targetX = std::clamp(targetX, 0.0f, container->getMaxScrollX());
		targetY = std::clamp(targetY, 0.0f, container->getMaxScrollY());

		if (targetX == container->getScrollX() &&
			targetY == container->getScrollY())
			return false;

		container->scrollTo(targetX, targetY);
		return true;
	}

	void Layout::arrangeWrapFlow(const Rect &inner, float gapX, float gapY)
	{
		struct Line
		{
			std::vector<Layout *> items;
			float width = 0.0f;
			float height = 0.0f;
			float totalGrow = 0.0f;
		};
		std::vector<Line> lines;
		Line curLine;

		const Vec2 vp = viewportSize();

		for (auto &c : children)
		{
			if (!c->isVisible())
				continue;
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.resolveH(inner.width, inner.height, vp);
			float mr = cst.margin.right.resolveH(inner.width, inner.height, vp);
			float mt = cst.margin.top.resolveV(inner.width, inner.height, vp);
			float mb = cst.margin.bottom.resolveV(inner.width, inner.height, vp);

			float baseW = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(inner.width, inner.height, vp);
			float childW = baseW + ml + mr;
			float childH = c->measuredSize.y + mt + mb;

			float spaceNeeded = childW + (curLine.items.empty() ? 0.0f : gapX);

			if (!curLine.items.empty() && curLine.width + spaceNeeded > inner.width && inner.width > 0.0f)
			{
				lines.push_back(curLine);
				curLine = Line();
				spaceNeeded = childW;
			}

			curLine.width += spaceNeeded;
			curLine.height = std::max(curLine.height, childH);
			curLine.totalGrow += cst.grow;
			curLine.items.push_back(c.get());
		}
		if (!curLine.items.empty())
			lines.push_back(curLine);

		float curY = inner.y;
		for (auto &line : lines)
		{
			float freeSpace = inner.width - line.width;
			float startX = inner.x;
			float extraGap = 0.0f;

			if (freeSpace > 0.0f && line.totalGrow <= 0.0f && !line.items.empty())
			{
				switch (style_.currentStyle.justify)
				{
				case Justify::Center:
					startX += freeSpace * 0.5f;
					break;
				case Justify::End:
					startX += freeSpace;
					break;
				case Justify::SpaceBetween:
					if (line.items.size() > 1)
						extraGap = freeSpace / (line.items.size() - 1);
					break;
				default:
					break;
				}
			}

			float curX = startX;
			for (auto *c : line.items)
			{
				const auto &cst = c->getStyle();
				float ml = cst.margin.left.resolveH(inner.width, inner.height, vp);
				float mr = cst.margin.right.resolveH(inner.width, inner.height, vp);
				float mt = cst.margin.top.resolveV(inner.width, inner.height, vp);
				float mb = cst.margin.bottom.resolveV(inner.width, inner.height, vp);

				float cw = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(inner.width, inner.height, vp);
				if (freeSpace > 0.0f && line.totalGrow > 0.0f && cst.grow > 0.0f)
				{
					cw += freeSpace * (cst.grow / line.totalGrow);
				}
				float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height, vp);
				float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height, vp);
				cw = clampSafe(cw, minW, maxW);

				float ch = c->measuredSize.y;
				float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height, vp);
				float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height, vp);
				float availH = std::max(0.0f, line.height - mt - mb);
				ch = clampSafe(ch, minH, std::min(availH, maxH));

				float cy = curY + mt;
				Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV;
				if (aV == Align::Stretch && cst.height.isAuto())
					ch = clampSafe(availH, minH, std::min(availH, maxH));
				else if (aV == Align::Center)
					cy += (availH - ch) * 0.5f;
				else if (aV == Align::End)
					cy += availH - ch;

				c->arrange({curX + ml, cy, cw, ch});
				curX += ml + cw + mr + gapX + extraGap;
			}
			curY += line.height + gapY;
		}
	}

	Vec2 Layout::measureConstrainedW(float constrainedW, float parent_w, float parent_h)
	{
		float prev = forcedMeasureW_;
		forcedMeasureW_ = constrainedW;
		Vec2 s = measure(parent_w, parent_h);
		forcedMeasureW_ = prev;
		return s;
	}

} // namespace ZenitUI