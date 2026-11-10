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
								float contentSize, float parentW, float parentH)
	{
		const Value &mv = isVerticalAxis ? cst.minHeight : cst.minWidth;
		const Overflow ov = isVerticalAxis ? cst.overflowY : cst.overflowX;
		if (!mv.isAuto())
			return isVerticalAxis ? mv.resolveV(parentW, parentH)
								  : mv.resolveH(parentW, parentH);
		return (ov == Overflow::Visible) ? contentSize : 0.0f;
	}

	Vec2 Layout::measure(float parent_w, float parent_h)
	{
		if (!internal_.subtreeDirty && parent_w == lastMeasureW_ && parent_h == lastMeasureH_)
			return measuredSize;
		lastMeasureW_ = parent_w;
		lastMeasureH_ = parent_h;

		if (!internal_.styleInitialized)
		{
			internal_.styleInitialized = 1;
			style_.currentStyle = style_.targetStyle = style_.transitionStartStyle = style_.resolveFor(*this);
			internal_.pendingTransition = 0;
		}

		ComputedStyle calcStyle = internal_.pendingTransition ? style_.resolveFor(*this) : style_.currentStyle;
		float pl = calcStyle.padding.left.resolveH(parent_w, parent_h);
		float pr = calcStyle.padding.right.resolveH(parent_w, parent_h);
		float pt = calcStyle.padding.top.resolveV(parent_w, parent_h);
		float pb = calcStyle.padding.bottom.resolveV(parent_w, parent_h);
		const auto &bws = calcStyle.borderWidth;
		float bwl = bws.left.resolveH(parent_w, parent_h);
		float bwr = bws.right.resolveH(parent_w, parent_h);
		float bwt = bws.top.resolveV(parent_w, parent_h);
		float bwb = bws.bottom.resolveV(parent_w, parent_h);

		float availW = calcStyle.width.isAuto() ? parent_w : calcStyle.width.resolveH(parent_w, parent_h);
		float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolveV(parent_w, parent_h);
		float innerW = std::max(0.0f, availW - pl - pr - bwl - bwr);
		float innerH = std::max(0.0f, availH - pt - pb - bwt - bwb);

		if (type == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
			innerH = 1e9f;
		if (type == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
			innerW = 1e9f;

		Vec2 contentSize = computeIntrinsicSize(innerW, innerH);
		float gapX = calcStyle.gap.resolveH(innerW, innerH);
		float gapY = calcStyle.gap.resolveV(innerW, innerH);

		if (type == LayoutType::Horizontal && calcStyle.wrap)
		{
			float curX = 0, curY = 0, maxW = 0, maxLineH = 0;
			int visibleInLine = 0;
			for (auto &c : children)
			{
				Vec2 cs = c->measure(innerW, innerH);
				if (c->getStyle().position == Position::Absolute || c->isPortal())
					continue;
				const auto &cst = c->getStyle();
				float ml = cst.margin.left.resolveH(innerW, innerH);
				float mr = cst.margin.right.resolveH(innerW, innerH);
				float mt = cst.margin.top.resolveV(innerW, innerH);
				float mb = cst.margin.bottom.resolveV(innerW, innerH);
				float childW = cs.x + ml + mr;
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
		else if (type == LayoutType::Horizontal)
		{
			float fixedMainSize = 0.0f;
			float maxCrossSize = 0.0f;
			float totalGrow = 0.0f;
			int visibleChildren = 0;

			for (auto &c : children)
			{
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
					float ml = cst.margin.left.resolveH(innerW, innerH);
					float mr = cst.margin.right.resolveH(innerW, innerH);
					float mt = cst.margin.top.resolveV(innerW, innerH);
					float mb = cst.margin.bottom.resolveV(innerW, innerH);
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
					const auto &cst = c->getStyle();
					if (cst.position == Position::Absolute || c->isPortal())
						continue;
					if (cst.grow > 0.0f && cst.width.isAuto() && innerW < 1e8f)
					{
						float ml = cst.margin.left.resolveH(innerW, innerH);
						float mr = cst.margin.right.resolveH(innerW, innerH);
						float mt = cst.margin.top.resolveV(innerW, innerH);
						float mb = cst.margin.bottom.resolveV(innerW, innerH);
						float shareW = remW * (cst.grow / totalGrow);
						float childAvailW = std::max(0.0f, shareW - ml - mr);
						Vec2 cs = c->measure(childAvailW, innerH);
						totalMainSize += cs.x + ml + mr;
						maxCrossSize = std::max(maxCrossSize, cs.y + mt + mb);
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
				Vec2 cs = c->measure(innerW, innerH);
				if (c->getStyle().position == Position::Absolute || c->isPortal())
					continue;
				const auto &cst = c->getStyle();
				float ml = cst.margin.left.resolveH(innerW, innerH);
				float mr = cst.margin.right.resolveH(innerW, innerH);
				float mt = cst.margin.top.resolveV(innerW, innerH);
				float mb = cst.margin.bottom.resolveV(innerW, innerH);
				float childTotalW = cs.x + ml + mr;
				float childTotalH = cs.y + mt + mb;
				if (type == LayoutType::Vertical)
				{
					totalMainSize += childTotalH;
					maxCrossSize = std::max(maxCrossSize, childTotalW);
				}
				else if (type == LayoutType::Stack)
				{
					contentSize.x = std::max(contentSize.x, childTotalW);
					contentSize.y = std::max(contentSize.y, childTotalH);
				}
				visibleChildren++;
			}
			if (type == LayoutType::Vertical)
			{
				if (visibleChildren > 1)
					totalMainSize += gapY * (visibleChildren - 1);
				contentSize.x = std::max(contentSize.x, maxCrossSize);
				contentSize.y = std::max(contentSize.y, totalMainSize);
			}
		}

		scroll_.contentSize = contentSize;

		float finalW = calcStyle.width.isAuto()
						   ? (contentSize.x + pl + pr + bwl + bwr)
						   : availW;
		float finalH = calcStyle.height.isAuto()
						   ? (contentSize.y + pt + pb + bwt + bwb)
						   : availH;

		float minW = calcStyle.minWidth.isAuto() ? 0.0f : calcStyle.minWidth.resolveH(parent_w, parent_h);
		float maxW = calcStyle.maxWidth.isAuto() ? 1e9f : calcStyle.maxWidth.resolveH(parent_w, parent_h);
		float minH = calcStyle.minHeight.isAuto() ? 0.0f : calcStyle.minHeight.resolveV(parent_w, parent_h);
		float maxH = calcStyle.maxHeight.isAuto() ? 1e9f : calcStyle.maxHeight.resolveV(parent_w, parent_h);

		measuredSize = {std::clamp(finalW, minW, maxW), std::clamp(finalH, minH, maxH)};
		return measuredSize;
	}

	void Layout::arrange(Rect space)
	{
		const bool positionChanged = (space.x != rect.x || space.y != rect.y);
		rect = space;

		if (isScrollContainer() && type != LayoutType::Stack)
		{
			if (scroll_.needsFullArrange(*this, positionChanged))
			{
				Rect arrangeSpace = scroll_.computeArrangeSpace(*this, space);
				arrangeInto(arrangeSpace);
				scroll_.arrangeInitialized = true;
				scroll_.appliedOffset = {0.0f, 0.0f};
			}

			scroll_.applyOffsetDelta(*this);
			scroll_.updateMaxScroll(*this, space);
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
		float pl = style_.currentStyle.padding.left.resolveH(space.width, space.height);
		float pr = style_.currentStyle.padding.right.resolveH(space.width, space.height);
		float pt = style_.currentStyle.padding.top.resolveV(space.width, space.height);
		float pb = style_.currentStyle.padding.bottom.resolveV(space.width, space.height);
		const auto &bws = style_.currentStyle.borderWidth;
		float bwl = bws.left.resolveH(space.width, space.height);
		float bwr = bws.right.resolveH(space.width, space.height);
		float bwt = bws.top.resolveV(space.width, space.height);
		float bwb = bws.bottom.resolveV(space.width, space.height);

		Rect inner = {
			space.x + pl + bwl,
			space.y + pt + bwt,
			std::max(0.0f, space.width - pl - pr - bwl - bwr),
			std::max(0.0f, space.height - pt - pb - bwt - bwb)};

		float gapX = style_.currentStyle.gap.resolveH(inner.width, inner.height);
		float gapY = style_.currentStyle.gap.resolveV(inner.width, inner.height);

		arrangeAbsoluteAndPortals(inner);

		if (type == LayoutType::Vertical)
			arrangeVerticalFlow(inner, gapY);
		else if (type == LayoutType::Horizontal)
		{
			if (style_.currentStyle.wrap)
				arrangeWrapFlow(inner, gapX, gapY);
			else
				arrangeHorizontalFlow(inner, gapX);
		}
		else if (type == LayoutType::Stack)
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
		float ml = cst.margin.left.resolveH(in.width, in.height);
		float mr = cst.margin.right.resolveH(in.width, in.height);
		float mt = cst.margin.top.resolveV(in.width, in.height);
		float mb = cst.margin.bottom.resolveV(in.width, in.height);

		bool hasL = !cst.left.isAuto();
		bool hasR = !cst.right.isAuto();
		bool hasT = !cst.top.isAuto();
		bool hasB = !cst.bottom.isAuto();

		float w = c->measuredSize.x;
		float h = c->measuredSize.y;
		float x, y;

		if (hasL && hasR && cst.width.isAuto())
		{
			float L = cst.left.resolveH(in.width, in.height);
			float R = cst.right.resolveH(in.width, in.height);
			w = std::max(0.0f, in.width - L - R - ml - mr);
			x = in.x + L + ml;
		}
		else if (hasL)
		{
			x = in.x + cst.left.resolveH(in.width, in.height) + ml;
		}
		else if (hasR)
		{
			x = in.x + in.width - w - cst.right.resolveH(in.width, in.height) - mr;
		}
		else
		{
			x = in.x + ml;
		}

		if (hasT && hasB && cst.height.isAuto())
		{
			float T = cst.top.resolveV(in.width, in.height);
			float B = cst.bottom.resolveV(in.width, in.height);
			h = std::max(0.0f, in.height - T - B - mt - mb);
			y = in.y + T + mt;
		}
		else if (hasT)
		{
			y = in.y + cst.top.resolveV(in.width, in.height) + mt;
		}
		else if (hasB)
		{
			y = in.y + in.height - h - cst.bottom.resolveV(in.width, in.height) - mb;
		}
		else
		{
			y = in.y + mt;
		}

		c->arrange({x, y, w, h});
	}

	void Layout::arrangeVerticalFlow(const Rect &inner, float gapY)
	{
		float totalFixedH = 0.0f, totalGrow = 0.0f, totalShrink = 0.0f;
		int visibleChildren = 0;

		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float baseH = cst.flexBasis.isAuto() ? c->measuredSize.y : cst.flexBasis.resolveV(inner.width, inner.height);
			totalFixedH += cst.margin.top.resolveV(inner.width, inner.height) +
						   baseH +
						   cst.margin.bottom.resolveV(inner.width, inner.height);

			totalGrow += cst.grow;
			totalShrink += cst.shrink;
			visibleChildren++;
		}
		if (visibleChildren > 1)
			totalFixedH += gapY * (visibleChildren - 1);

		float freeSpace = inner.height - totalFixedH;
		float startY = inner.y;
		float extraGap = 0.0f;

		if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0)
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

		float curY = startY;

		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.resolveH(inner.width, inner.height);
			float mr = cst.margin.right.resolveH(inner.width, inner.height);
			float mt = cst.margin.top.resolveV(inner.width, inner.height);
			float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

			float baseH = cst.flexBasis.isAuto() ? c->measuredSize.y : cst.flexBasis.resolveV(inner.width, inner.height);
			float ch = baseH;
			if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
				ch += freeSpace * (cst.grow / totalGrow);
			else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
				ch += freeSpace * (cst.shrink / totalShrink);

			float minH = resolveFlexMin(cst, /*isVerticalAxis=*/true,
										c->measuredSize.y,
										inner.width, inner.height);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);
			ch = clampSafe(ch, minH, maxH);

			float cw = c->measuredSize.x;
			float availW = std::max(0.0f, inner.width - ml - mr);
			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
			cw = clampSafe(cw, minW, std::min(availW, maxW));

			float cx = inner.x + ml;
			Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.alignItems : cst.alignH;
			if (aH == Align::Stretch)
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
		float totalFixedW = 0.0f, totalGrow = 0.0f, totalShrink = 0.0f;
		int visibleChildren = 0;
		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float baseW = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(inner.width, inner.height);
			totalFixedW += cst.margin.left.resolveH(inner.width, inner.height) +
						   baseW +
						   cst.margin.right.resolveH(inner.width, inner.height);

			totalGrow += cst.grow;
			totalShrink += cst.shrink;
			visibleChildren++;
		}
		if (visibleChildren > 1)
			totalFixedW += gapX * (visibleChildren - 1);

		float freeSpace = inner.width - totalFixedW;
		float startX = inner.x;
		float extraGap = 0.0f;
		if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0)
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
				if (visibleChildren > 1)
					extraGap = freeSpace / (visibleChildren - 1);
				break;
			default:
				break;
			}
		}

		float curX = startX;
		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;
			float ml = cst.margin.left.resolveH(inner.width, inner.height);
			float mr = cst.margin.right.resolveH(inner.width, inner.height);
			float mt = cst.margin.top.resolveV(inner.width, inner.height);
			float mb = cst.margin.bottom.resolveV(inner.width, inner.height);
			float baseW = cst.flexBasis.isAuto() ? c->measuredSize.x : cst.flexBasis.resolveH(inner.width, inner.height);
			float cw = baseW;
			if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
				cw += freeSpace * (cst.grow / totalGrow);
			else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
				cw += freeSpace * (cst.shrink / totalShrink);
			float minW = resolveFlexMin(cst, /*isVerticalAxis=*/false,
										c->measuredSize.x,
										inner.width, inner.height);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
			cw = clampSafe(cw, minW, maxW);

			float ch = c->measuredSize.y;
			float availH = std::max(0.0f, inner.height - mt - mb);
			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);
			ch = clampSafe(ch, minH, std::min(availH, maxH));

			float cy = inner.y + mt;
			Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV;
			if (aV == Align::Stretch)
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
		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.resolveH(inner.width, inner.height);
			float mr = cst.margin.right.resolveH(inner.width, inner.height);
			float mt = cst.margin.top.resolveV(inner.width, inner.height);
			float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

			float availW = std::max(0.0f, inner.width - ml - mr);
			float availH = std::max(0.0f, inner.height - mt - mb);

			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);

			float cw = clampSafe(c->measuredSize.x, minW, std::min(availW, maxW));
			float ch = clampSafe(c->measuredSize.y, minH, std::min(availH, maxH));

			float cx = inner.x + ml;
			float cy = inner.y + mt;

			Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.alignItems : cst.alignH;
			Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV;

			if (aH == Align::Stretch)
				cw = clampSafe(availW, minW, std::min(availW, maxW));
			else if (aH == Align::Center)
				cx += (availW - cw) * 0.5f;
			else if (aH == Align::End)
				cx += availW - cw;

			if (aV == Align::Stretch)
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
		if (ancestorBlocked || !config.enabled)
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
		if (needsClip)
		{
			Rect base = clipLocal
							? *clipLocal
							: Rect{0, 0, Metrics::viewport.x, Metrics::viewport.y};
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

		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (cst.position == Position::Absolute || c->isPortal())
				continue;

			float ml = cst.margin.left.resolveH(inner.width, inner.height);
			float mr = cst.margin.right.resolveH(inner.width, inner.height);
			float mt = cst.margin.top.resolveV(inner.width, inner.height);
			float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

			float childW = c->measuredSize.x + ml + mr;
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
				float ml = cst.margin.left.resolveH(inner.width, inner.height);
				float mr = cst.margin.right.resolveH(inner.width, inner.height);
				float mt = cst.margin.top.resolveV(inner.width, inner.height);
				float mb = cst.margin.bottom.resolveV(inner.width, inner.height);

				float cw = c->measuredSize.x;
				if (freeSpace > 0.0f && line.totalGrow > 0.0f && cst.grow > 0.0f)
				{
					cw += freeSpace * (cst.grow / line.totalGrow);
				}
				float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height);
				float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
				cw = clampSafe(cw, minW, maxW);

				float ch = c->measuredSize.y;
				float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height);
				float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);
				float availH = std::max(0.0f, line.height - mt - mb);
				ch = clampSafe(ch, minH, std::min(availH, maxH));

				float cy = curY + mt;
				Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.alignItems : cst.alignV;
				if (aV == Align::Stretch)
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
} // namespace ZenitUI