#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

	// =========================================================================
	//  MISURA E ARRANGE
	//  Fase 1: ogni nodo calcola la propria dimensione intrinseca (measure).
	//  Fase 2: il genitore posiziona i figli nello spazio (arrange).
	//  Fase 3: hitTest usa i rect risultanti per individuare il nodo sotto il
	//          cursore, rispettando l'ordine z.
	// =========================================================================

	Vec2 Layout::measure(float parent_w, float parent_h)
	{
		if (!subtreeDirty_ &&
			parent_w == lastMeasureW_ &&
			parent_h == lastMeasureH_)
		{
			return measuredSize;
		}
		lastMeasureW_ = parent_w;
		lastMeasureH_ = parent_h;

		if (!styleInitialized)
		{
			styleInitialized = true;
			style_.currentStyle = style_.targetStyle = style_.transitionStartStyle = style_.resolveFor(*this);
			pendingTransition = false;
		}
		ComputedStyle calcStyle = pendingTransition ? style_.resolveFor(*this) : style_.currentStyle;

		// --- Spazio disponibile interno ---
		float pl = calcStyle.padding.left.resolveH(parent_w, parent_h);
		float pr = calcStyle.padding.right.resolveH(parent_w, parent_h);
		float pt = calcStyle.padding.top.resolveV(parent_w, parent_h);
		float pb = calcStyle.padding.bottom.resolveV(parent_w, parent_h);

		float availW = calcStyle.width.isAuto() ? parent_w : calcStyle.width.resolveH(parent_w, parent_h);
		float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolveV(parent_w, parent_h);

		float innerW = std::max(0.0f, availW - pl - pr);
		float innerH = std::max(0.0f, availH - pt - pb);

		// Se abbiamo lo scroll, togliamo i limiti di misurazione sull'asse principale
		if (type == LayoutType::Vertical && calcStyle.overflowY != Overflow::Visible)
			innerH = 1e9f;
		if (type == LayoutType::Horizontal && calcStyle.overflowX != Overflow::Visible)
			innerW = 1e9f;

		// --- Misurazione Intrinseca del Nodo (es. Testo) ---
		Vec2 contentSize = computeIntrinsicSize(innerW, innerH);

		float gapX = calcStyle.gap.resolveH(innerW, innerH);
		float gapY = calcStyle.gap.resolveV(innerW, innerH);

		float totalMainSize = 0.0f;
		float maxCrossSize = 0.0f;
		int visibleChildren = 0;

		// --- Misura tutti i figli ---
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

			if (type == LayoutType::Horizontal)
			{
				totalMainSize += childTotalW;
				maxCrossSize = std::max(maxCrossSize, childTotalH);
			}
			else if (type == LayoutType::Vertical)
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

		// --- Aggiungi i gap ---
		if (type == LayoutType::Horizontal)
		{
			if (visibleChildren > 1)
				totalMainSize += gapX * (visibleChildren - 1);
			contentSize.x = std::max(contentSize.x, totalMainSize);
			contentSize.y = std::max(contentSize.y, maxCrossSize);
		}
		else if (type == LayoutType::Vertical)
		{
			if (visibleChildren > 1)
				totalMainSize += gapY * (visibleChildren - 1);
			contentSize.x = std::max(contentSize.x, maxCrossSize);
			contentSize.y = std::max(contentSize.y, totalMainSize);
		}

		// Salviamo l'ingombro reale non tagliato per gestire i limiti dello ScrollView
		scrollContentSize = contentSize;

		// --- Risoluzione Dimensioni Finali del Genitore ---
		float finalW = calcStyle.width.isAuto() ? (contentSize.x + pl + pr) : availW;
		float finalH = calcStyle.height.isAuto() ? (contentSize.y + pt + pb) : availH;

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
			// positionChanged: il container è stato riposizionato (es. listContainer
			// del Dropdown spostato dall'arrange del padre). I figli vanno riallineati.
			bool needFullArrange = !arrangeInitialized_ || subtreeDirty_ || positionChanged;

			if (needFullArrange)
			{
				float pl = style_.currentStyle.padding.left.resolveH(space.width, space.height);
				float pr = style_.currentStyle.padding.right.resolveH(space.width, space.height);
				float pt = style_.currentStyle.padding.top.resolveV(space.width, space.height);
				float pb = style_.currentStyle.padding.bottom.resolveV(space.width, space.height);

				Rect arrangeSpace = space;

				if (type == LayoutType::Vertical && style_.currentStyle.overflowY != Overflow::Visible)
				{
					float contentH = scrollContentSize.y + pt + pb;
					arrangeSpace.height = std::max(space.height, contentH);
				}
				else if (type == LayoutType::Horizontal && style_.currentStyle.overflowX != Overflow::Visible)
				{
					float contentW = scrollContentSize.x + pl + pr;
					arrangeSpace.width = std::max(space.width, contentW);
				}

				arrangeInto(arrangeSpace);
				arrangeInitialized_ = true;
				appliedScroll_ = {0.0f, 0.0f};
			}

			float dx = appliedScroll_.x - scroll_.offset.x;
			float dy = appliedScroll_.y - scroll_.offset.y;
			if (dx != 0.0f || dy != 0.0f)
			{
				translateSubtree(dx, dy);
				appliedScroll_ = scroll_.offset;
			}

			float pl = style_.currentStyle.padding.left.resolveH(space.width, space.height);
			float pr = style_.currentStyle.padding.right.resolveH(space.width, space.height);
			float pt = style_.currentStyle.padding.top.resolveV(space.width, space.height);
			float pb = style_.currentStyle.padding.bottom.resolveV(space.width, space.height);

			float contentW = scrollContentSize.x + pl + pr;
			float contentH = scrollContentSize.y + pt + pb;
			scroll_.maxScroll.x = std::max(0.0f, contentW - rect.width);
			scroll_.maxScroll.y = std::max(0.0f, contentH - rect.height);
			scroll_.clamp();
		}
		else
		{
			arrangeInto(space);
			arrangeInitialized_ = true;
		}

		onLayout();
	}

	void Layout::translateSubtree(float dx, float dy)
	{
		// Sposta i figli (e i loro discendenti) senza toccare il rect di `this`.
		for (auto &c : children)
		{
			//if (c->isPortal())
			//	continue;
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

		Rect inner = {
			space.x + pl,
			space.y + pt,
			std::max(0.0f, space.width - pl - pr),
			std::max(0.0f, space.height - pt - pb)};

		float gapX = style_.currentStyle.gap.resolveH(inner.width, inner.height);
		float gapY = style_.currentStyle.gap.resolveV(inner.width, inner.height);

		arrangeAbsoluteAndPortals(inner);

		if (type == LayoutType::Vertical)
			arrangeVerticalFlow(inner, gapY);
		else if (type == LayoutType::Horizontal)
			arrangeHorizontalFlow(inner, gapX);
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

		// ---- orizzontale ----
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

		// ---- verticale ----
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
			totalFixedH += cst.margin.top.resolveV(inner.width, inner.height) + c->measuredSize.y + cst.margin.bottom.resolveV(inner.width, inner.height);
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

			// ---- main axis: height ----
			float ch = c->measuredSize.y;
			if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
				ch += freeSpace * (cst.grow / totalGrow);
			else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
				ch += freeSpace * (cst.shrink / totalShrink);

			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);
			ch = clampSafe(ch, minH, maxH);

			// ---- cross axis: width ----
			float cw = c->measuredSize.x;
			float availW = std::max(0.0f, inner.width - ml - mr);
			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
			cw = clampSafe(cw, minW, std::min(availW, maxW));

			float cx = inner.x + ml;
			Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.itemsH : cst.alignH;
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
			totalFixedW += cst.margin.left.resolveH(inner.width, inner.height) + c->measuredSize.x + cst.margin.right.resolveH(inner.width, inner.height);
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

			// ---- main axis: width ----
			float cw = c->measuredSize.x;
			if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
				cw += freeSpace * (cst.grow / totalGrow);
			else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
				cw += freeSpace * (cst.shrink / totalShrink);

			float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolveH(inner.width, inner.height);
			float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolveH(inner.width, inner.height);
			cw = clampSafe(cw, minW, maxW);

			// ---- cross axis: height ----
			float ch = c->measuredSize.y;
			float availH = std::max(0.0f, inner.height - mt - mb);
			float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolveV(inner.width, inner.height);
			float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolveV(inner.width, inner.height);
			ch = clampSafe(ch, minH, std::min(availH, maxH));

			float cy = inner.y + mt;
			Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.itemsV : cst.alignV;
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

			Align aH = (cst.alignH == Align::Auto) ? style_.currentStyle.itemsH : cst.alignH;
			Align aV = (cst.alignV == Align::Auto) ? style_.currentStyle.itemsV : cst.alignV;

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

	Layout *Layout::hitTest(Vec2 p, bool ancestorBlocked)
	{
		if (ancestorBlocked || !isEnabled)
			return nullptr;

		if (!hasParent())
		{
			for (auto it = UIContext::get().activePortals.rbegin(); it != UIContext::get().activePortals.rend(); ++it)
			{
				auto sp = it->lock();
				if (!sp)
					continue;
				bool wasPortal = sp->isPortal();
				sp->setPortal(false);
				Layout *hit = sp->hitTest(p, false);
				sp->setPortal(wasPortal);
				if (hit)
					return hit;
			}
		}
		if (isPortal_)
			return nullptr;

		Transform2D tr = currentTransform(style_.currentStyle);
		Vec2 pChildren = applyInverseTransform(tr, p);

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
			if (Layout *hit = (*it)->hitTest(pChildren, false))
				return hit;
		for (auto it = normalFlow.rbegin(); it != normalFlow.rend(); ++it)
			if (Layout *hit = (*it)->hitTest(pChildren, false))
				return hit;
		for (auto it = negZ.rbegin(); it != negZ.rend(); ++it)
			if (Layout *hit = (*it)->hitTest(pChildren, false))
				return hit;

		if (rect.width > 0 && rect.height > 0 &&
			rect.contains(p) &&
			(isInteractive || blocksRaycast))
		{
			return this;
		}
		return nullptr;
	}

} // namespace ZenitUI