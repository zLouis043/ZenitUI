#include "Layout.hpp"

namespace ZenitUI
{

	ComputedStyle Layout::resolveTargetStyle()
	{
		auto &theme = Theme::get();

		// 1) Raccogli le regole che matchano (esclusi i ::part).
		struct Match
		{
			const ThemeRule *rule;
		};
		std::vector<Match> matches;

		for (const auto &r : theme.rules)
		{
			if (!r.part.empty())
				continue; // i part li gestisce partStyle
			if (!ruleMatches(r, this))
				continue;
			matches.push_back({&r});
		}

		// 2) Ordina per specificità crescente, poi per ordine di dichiarazione.
		std::stable_sort(matches.begin(), matches.end(),
						 [](const Match &a, const Match &b)
						 {
							 if (a.rule->specificity != b.rule->specificity)
								 return a.rule->specificity < b.rule->specificity;
							 return a.rule->order < b.rule->order;
						 });

		// 3) Overlay in ordine.
		Style finalStyle;
		for (const auto &m : matches)
			finalStyle.overlay(m.rule->style);

		// 4) Inline (vince su qualsiasi regola, indipendentemente dalla specificità).
		finalStyle.overlay(inlineBase);
		if (!isEnabled)
			finalStyle.overlay(inlineDisabled);
		else
		{
			if (isHovered || isPressed)
				finalStyle.overlay(inlineHover);
			if (isPressed)
				finalStyle.overlay(inlinePressed);
			if (isFocused)
				finalStyle.overlay(inlineFocus);
			if (isChecked_)
				finalStyle.overlay(inlineChecked);
		}

		// 5) Root come fallback (via ComputedStyle::from).
		const ComputedStyle *parentStyle = nullptr;
		if (auto p = getParent())
			parentStyle = &p->currentStyle;

		return ComputedStyle::from(finalStyle, parentStyle, &theme.root);
	}

	Style Layout::partStyle(const std::string &partName)
	{
		auto &theme = Theme::get();

		// 1) Raccogli le regole che matchano, filtrando per part.
		std::vector<const ThemeRule *> matches;

		for (const auto &r : theme.rules)
		{
			if (r.part != partName)
				continue;
			if (!ruleMatches(r, this))
				continue;
			matches.push_back(&r);
		}

		// 2) Ordina per specificità crescente, poi per ordine di dichiarazione.
		std::stable_sort(matches.begin(), matches.end(),
						 [](const ThemeRule *a, const ThemeRule *b)
						 {
							 if (a->specificity != b->specificity)
								 return a->specificity < b->specificity;
							 return a->order < b->order;
						 });

		// 3) Target = overlay in ordine.
		Style target;
		for (const auto *r : matches)
			target.overlay(r->style);

		// ---- Transizione di proprietà sul part ----
		double now = UIContext::get().time;
		auto &st = partTransitions[partName];

		if (!st.initialized)
		{
			st.target = target;
			st.current = target;
			st.active = false;
			st.initialized = true;
		}
		else if (stylesDiffer(target, st.target))
		{
			// Congela il valore interpolato attuale come nuovo "start"
			float tp = 1.0f;
			if (st.active && st.duration > 0.001f)
			{
				tp = (float)std::clamp((now - st.startTime) / st.duration, 0.0, 1.0);
			}
			st.current = lerpStyleParts(st.current, st.target, getRatio(tp, st.ease));

			st.target = target;
			st.startTime = now;

			float maxDur = 0.0f;
			TransitionFunction ease = TransitionFunction::Linear;
			if (target.transitions.is_set)
			{
				for (const auto &spec : target.transitions.value)
				{
					if (spec.duration > maxDur)
					{
						maxDur = spec.duration;
						ease = spec.ease;
					}
				}
			}
			st.duration = maxDur;
			st.ease = ease;
			st.active = (maxDur > 0.001f);
		}

		float partT = 1.0f;
		if (st.active)
		{
			partT = (float)std::clamp((now - st.startTime) / st.duration, 0.0, 1.0);
			if (partT >= 1.0f)
			{
				st.active = false;
				partT = 1.0f;
			}
		}

		Style result = lerpStyleParts(st.current, st.target, getRatio(partT, st.ease));

		// ---- Sync + tick + overlay delle animazioni CSS (invariato) ----
		auto &anims = activePartAnimations[partName];
		const auto &refs = result.animations.is_set
							   ? result.animations.value
							   : std::vector<AnimationRef>{};

		anims.erase(
			std::remove_if(anims.begin(), anims.end(),
						   [&](const ActiveCssAnimation &a)
						   {
							   for (const auto &r : refs)
								   if (r.name == a.name)
									   return false;
							   return true;
						   }),
			anims.end());

		// Sync: aggiungi/aggiorna
		for (const auto &r : refs)
		{
			bool found = false;
			for (auto &a : anims)
			{
				if (a.name == r.name)
				{
					a.duration = r.duration;
					a.delay = r.delay;
					a.iterations = r.iterations;
					a.alternate = r.alternate;
					a.fillForwards = r.fillForwards;
					a.ease = r.ease;
					found = true;
					break;
				}
			}
			if (!found)
			{
				ActiveCssAnimation a;
				a.name = r.name;
				a.startTime = now; // <-- start "assoluto"
				a.duration = r.duration;
				a.delay = r.delay;
				a.iterations = r.iterations;
				a.alternate = r.alternate;
				a.fillForwards = r.fillForwards;
				a.ease = r.ease;
				anims.push_back(std::move(a));
			}
		}

		// Tick: ricalcola elapsed da startTime, poi delega a sampleActive
		for (auto &anim : anims)
		{
			if (anim.finished)
				continue;
			anim.elapsed = (float)(now - anim.startTime);
			auto it = theme.keyframes.find(anim.name);
			if (it == theme.keyframes.end())
			{
				anim.finished = true;
				continue;
			}
			bool fin = false;
			(void)sampleActive(anim, fin);
			if (fin)
				anim.finished = true;
		}

		anims.erase(
			std::remove_if(anims.begin(), anims.end(),
						   [](const ActiveCssAnimation &a)
						   { return a.finished && !a.fillForwards; }),
			anims.end());

		for (const auto &anim : anims)
		{
			auto it = theme.keyframes.find(anim.name);
			if (it == theme.keyframes.end())
				continue;
			bool fin = false;
			float localT = sampleActive(anim, fin);
			if (fin && !anim.fillForwards)
				continue;
			Style frame = evaluateKeyframes(it->second, localT, anim.ease);
			result.overlay(frame);
		}

		return result;
	}

	void Layout::playAnimation(const std::string &name, bool playReverse)
	{
		auto it = activeAnimations.find(name);
		if (it == activeAnimations.end())
			return;
		auto &state = it->second;
		state.reverse = playReverse;
		state.playing = true;
		if (!state.reverse && state.elapsed >= state.anim->duration)
			state.elapsed = 0.0f;
		else if (state.reverse && state.elapsed <= 0.0f)
			state.elapsed = state.anim->duration;
		state.delayElapsed = state.anim->delay;
	}

	bool Layout::hasActiveAnimations() const
	{
		for (const auto &[n, s] : activeAnimations)
			if (s.playing)
				return true;
		if (!activeCssAnimations.empty())
			return true;
		for (const auto &c : children)
			if (c->hasActiveAnimations())
				return true;
		return false;
	}

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
			currentStyle = targetStyle = transitionStartStyle = resolveTargetStyle();
			pendingTransition = false;
		}
		ComputedStyle calcStyle = pendingTransition ? resolveTargetStyle() : currentStyle;

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
		if (calcStyle.overflow == Overflow::Scroll)
		{
			if (type == LayoutType::Vertical)
				innerH = 1e9f;
			if (type == LayoutType::Horizontal)
				innerW = 1e9f;
		}

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
		rect = space;
		arrangeInto(space);
		onLayout();
	}

	void Layout::arrangeInto(Rect space)
	{
		float pl = currentStyle.padding.left.resolveH(space.width, space.height);
		float pr = currentStyle.padding.right.resolveH(space.width, space.height);
		float pt = currentStyle.padding.top.resolveV(space.width, space.height);
		float pb = currentStyle.padding.bottom.resolveV(space.width, space.height);

		Rect inner = {
			space.x + pl,
			space.y + pt,
			std::max(0.0f, space.width - pl - pr),
			std::max(0.0f, space.height - pt - pb)};

		float gapX = currentStyle.gap.resolveH(inner.width, inner.height);
		float gapY = currentStyle.gap.resolveV(inner.width, inner.height);

		// ---- Pre-pass: figli absolute (fuori dal flow) ----
		auto arrangeAbsolute = [](Layout *c, const ComputedStyle &cst, const Rect &in)
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
		};

		for (auto &c : children)
		{
			const auto &cst = c->getStyle();
			if (c->isPortal())
			{
				c->arrange({inner.x, inner.y, c->measuredSize.x, c->measuredSize.y});
				continue;
			}
			if (cst.position == Position::Absolute)
			{
				arrangeAbsolute(c.get(), cst, inner);
			}
		}

		auto clampSafe = [](float v, float lo, float hi) -> float
		{
			if (hi < lo)
				return hi;
			return std::clamp(v, lo, hi);
		};

		// =====================================================================
		//  VERTICAL
		// =====================================================================
		if (type == LayoutType::Vertical)
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
				switch (currentStyle.justify)
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
				Align aH = (cst.alignH == Align::Auto) ? currentStyle.itemsH : cst.alignH;
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
		// =====================================================================
		//  HORIZONTAL
		// =====================================================================
		else if (type == LayoutType::Horizontal)
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
				switch (currentStyle.justify)
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
				Align aV = (cst.alignV == Align::Auto) ? currentStyle.itemsV : cst.alignV;
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
		// =====================================================================
		//  STACK
		// =====================================================================
		else if (type == LayoutType::Stack)
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

				Align aH = (cst.alignH == Align::Auto) ? currentStyle.itemsH : cst.alignH;
				Align aV = (cst.alignV == Align::Auto) ? currentStyle.itemsV : cst.alignV;

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
	}

	void Layout::update(float dt, bool ancestorBlocked)
	{
		bool wasPending = pendingTransition;
		ComputedStyle styleBefore = currentStyle;

		// Snapshot delle prop ereditate PRIMA di ogni modifica
		struct InheritedSnap
		{
			std::string font;
			Value fontSize;
			Color color;
			Value letterSpacing;
			Align textAlign;
		} snapBefore{
			currentStyle.font,
			currentStyle.fontSize,
			currentStyle.color,
			currentStyle.letterSpacing,
			currentStyle.textAlign};

		// Init stile al primo frame
		if (!styleInitialized)
		{
			styleInitialized = true;
			currentStyle = targetStyle = transitionStartStyle = resolveTargetStyle();
			pendingTransition = false;
			syncCssAnimations();
		}

		// --- Animazioni imperative ---
		bool localAnimBlocks = false;
		for (auto &[n, s] : activeAnimations)
		{
			if (!s.playing)
				continue;
			if (s.anim->blocksInput)
				localAnimBlocks = true;
			if (s.delayElapsed > 0.0f)
			{
				s.delayElapsed -= dt;
				continue;
			}

			s.elapsed += (s.reverse ? -dt : dt);
			if (s.elapsed <= 0.0f)
			{
				s.elapsed = 0.0f;
				s.playing = false;
			}
			if (s.elapsed >= s.anim->duration)
			{
				s.elapsed = s.anim->duration;
				s.playing = false;
			}

			float t = (s.anim->duration > 0.0f) ? (s.elapsed / s.anim->duration) : 1.0f;
			for (auto &track : s.anim->tracks)
				track->apply(t, this);
			pendingTransition = true;
		}

		// --- Blocchi ---
		bool blockSubtree = ancestorBlocked || localAnimBlocks;
		bool selfBlocked = blockSubtree || !isEnabled || (!isInteractive && blocksRaycast);

		auto &ctx = UIContext::get();
		auto &pointer = ctx.pointer;

		// --- Hover / Pressed / Focus (usano topmostConsumer, già settato) ---
		isHovered = false;
		if (!selfBlocked && rect.width > 0 && rect.height > 0)
		{
			isHovered = rect.contains(pointer.pos);
		}

		isPressed = false;
		if (pointer.down && ctx.pressTarget)
		{
			Layout *n = ctx.pressTarget;
			while (n)
			{
				if (n == this)
				{
					isPressed = true;
					break;
				}
				if (!n->getPassThrough())
					break;
				n = n->getParent().get();
			}
		}

		bool focusNow = ctx.hasFocus(this);
		if (focusNow != isFocused)
		{
			isFocused = focusNow;
			pendingTransition = true;
			// for (auto &c : children)
			//	c->markInheritanceDirty();
		}

		// --- Cambio stato: SOLO setup, niente callback ---
		UIState nextState = UIState::Idle;
		if (!isEnabled)
			nextState = UIState::Disabled;
		else if (isPressed)
			nextState = UIState::Pressed;
		else if (isHovered)
			nextState = UIState::Hover;

		UIState prevState = currentState;
		bool stateChanged = (currentState != nextState);

		if (stateChanged)
		{
			currentState = nextState;
			transitionTimer = 0.0f;
			transitionStartStyle = currentStyle;
			targetStyle = resolveTargetStyle();
			pendingTransition = false;
			syncCssAnimations();
			// for (auto &c : children)
			//	c->markInheritanceDirty();
		}
		else if (pendingTransition)
		{
			ComputedStyle newTarget = resolveTargetStyle();
			pendingTransition = false;

			if (newTarget != targetStyle)
			{
				transitionStartStyle = currentStyle;
				transitionTimer = 0.0f;
				targetStyle = std::move(newTarget);
			}
			else
			{
				targetStyle = std::move(newTarget);
				if (transitionTimer >= 1.0f)
				{
					currentStyle = targetStyle;
					transitionStartStyle = currentStyle;
				}
			}
			syncCssAnimations();
		}
		// --- Avanzamento transizione per-property ---
		if (transitionTimer < 1.0f)
		{
			float maxDur = targetStyle.transitionTime;
			for (const auto &spec : targetStyle.transitions)
				maxDur = std::max(maxDur, spec.duration);

			if (maxDur <= 0.001f)
			{
				transitionTimer = 1.0f;
				currentStyle = targetStyle;
			}
			else
			{
				float elapsed = transitionTimer * maxDur;
				transitionTimer += dt / maxDur;
				if (transitionTimer >= 1.0f)
				{
					transitionTimer = 1.0f;
					currentStyle = targetStyle;
				}
				else
				{
					currentStyle = lerpStyleTimed(transitionStartStyle, targetStyle, elapsed);
				}
			}
		}

		// --- Tick animazioni CSS (nessuna callback utente) ---
		for (auto &anim : activeCssAnimations)
		{
			if (anim.finished)
				continue;
			anim.elapsed += dt;
			auto it = Theme::get().keyframes.find(anim.name);
			if (it == Theme::get().keyframes.end())
			{
				anim.finished = true;
				continue;
			}
			bool fin = false;
			(void)sampleActive(anim, fin);
			if (fin)
				anim.finished = true;
		}
		activeCssAnimations.erase(
			std::remove_if(activeCssAnimations.begin(), activeCssAnimations.end(),
						   [](const ActiveCssAnimation &a)
						   { return a.finished && !a.fillForwards; }),
			activeCssAnimations.end());

		// ============================================================
		//  Propagazione al sottoalbero.
		//  Unico punto di verità: confronta lo snapshot del frame
		//  precedente con lo stato attuale. Se qualcosa che influenza
		//  i discendenti è cambiato, marca i figli per il ricalcolo.
		//  Copre: hover, pressed, focus, enabled, checked e prop ereditate.
		// ============================================================
		bool subtreeChanged =
			lastSnapshot.hovered != isHovered ||
			lastSnapshot.pressed != isPressed ||
			lastSnapshot.focused != isFocused ||
			lastSnapshot.enabled != isEnabled ||
			lastSnapshot.checked != isChecked_ ||
			lastSnapshot.font != currentStyle.font ||
			!(lastSnapshot.fontSize == currentStyle.fontSize) ||
			!(lastSnapshot.color == currentStyle.color) ||
			!(lastSnapshot.letterSpacing == currentStyle.letterSpacing) ||
			!(lastSnapshot.textAlign == currentStyle.textAlign);

		if (subtreeChanged)
		{
			for (auto &c : children)
			{
				c->pendingTransition = true;
				c->markInheritanceDirty();
			}

			lastSnapshot.hovered = isHovered;
			lastSnapshot.pressed = isPressed;
			lastSnapshot.focused = isFocused;
			lastSnapshot.enabled = isEnabled;
			lastSnapshot.checked = isChecked_;
			lastSnapshot.font = currentStyle.font;
			lastSnapshot.fontSize = currentStyle.fontSize;
			lastSnapshot.color = currentStyle.color;
			lastSnapshot.letterSpacing = currentStyle.letterSpacing;
			lastSnapshot.textAlign = currentStyle.textAlign;
		}

		// ============================================================
		//  FASE 2: i figli vedono currentStyle già aggiornato
		// ============================================================
		{
			size_t n = children.size();
			for (size_t i = 0; i < n; ++i)
				children[i]->update(dt, blockSubtree);
		}

		// ============================================================
		//  FASE 3: callback + post-processing
		// ============================================================

		handleFocusInput();

		if (stateChanged)
		{
			if (nextState == UIState::Hover && prevState != UIState::Hover && onHoverEnter)
				onHoverEnter();
			else if (prevState == UIState::Hover && nextState != UIState::Hover && onHoverExit)
				onHoverExit();
		}

		if (pointer.pressed && !ctx.clickConsumed)
		{
			bool inPressPath = (ctx.pressTarget == this) || isAncestorOf(ctx.pressTarget);
			if (inPressPath)
			{
				if (onPress)
					onPress();
				if (!passThrough_)
					ctx.consumeClick();
			}
		}

		if (pointer.released)
		{
			bool pressedHere = (ctx.pressTarget == this) || isAncestorOf(ctx.pressTarget);
			bool releasedHere = (ctx.releaseTarget == this) || isAncestorOf(ctx.releaseTarget);
			bool validClick = pressedHere && releasedHere;

			if (validClick)
			{
				if (onRelease)
					onRelease();

				if (onClick && !ctx.clickConsumed)
				{
					onClick();
					if (!passThrough_)
						ctx.consumeClick();
				}
			}
		}

		if (pointer.rightPressed && isHovered && onRightClick)
		{
			if (!ctx.rightClickConsumed)
			{
				onRightClick();
				if (!passThrough_)
					ctx.consumeRightClick();
			}
		}

		// --- Rimozioni ---
		for (auto it = children.begin(); it != children.end();)
		{
			if ((*it)->wantsRemoval)
			{
				(*it)->parent.reset();
				it = children.erase(it);
			}
			else
				++it;
		}

		// ============================================================
		//  Ricalcola subtreeDirty_ per questo frame.
		//  Un nodo è "dirty" se lui stesso o un suo discendente ha
		//  cambiato qualcosa che richiede un nuovo measure.
		// ============================================================
		bool anyChildDirty = false;
		for (auto &c : children)
		{
			if (c->subtreeDirty_)
			{
				anyChildDirty = true;
				break;
			}
		}

		bool styleChanged = (currentStyle != styleBefore);
		bool inTransition = (transitionTimer < 1.0f);

		subtreeDirty_ = wasPending || pendingTransition || anyChildDirty || styleChanged || inTransition;

		if (isEnabled || updateWhenDisabled_)
			onUpdate(dt);
	}

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
			float m = 300.0f;

			if (rect.x > clip.x + clip.width + m ||
				rect.x + rect.width < clip.x - m ||
				rect.y > clip.y + clip.height + m ||
				rect.y + rect.height < clip.y - m)
			{
				return;
			}
		}

		ComputedStyle renderStyle = currentStyle;
		for (const auto &anim : activeCssAnimations)
		{
			auto it = Theme::get().keyframes.find(anim.name);
			if (it == Theme::get().keyframes.end())
				continue;
			bool fin = false;
			float localT = sampleActive(anim, fin);
			if (fin && !anim.fillForwards)
				continue;
			Style frame = evaluateKeyframes(it->second, localT, anim.ease);
			overlayComputed(renderStyle, frame);
		}

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f)
			return;

		Transform2D tr = currentTransform(renderStyle);

		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;

		if (!identity)
			renderer->pushTransform(tr);
		if (hasShader && renderer->supports(Feature::Effects))
			renderer->pushEffect(customEffect);

		bool needsClip = (renderStyle.overflow == Overflow::Hidden ||
						  renderStyle.overflow == Overflow::Scroll);
		if (needsClip)
			renderer->pushClip(rect);

		renderChrome(globalOp, renderStyle);
		renderContent(globalOp, renderStyle);

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

		for (auto *child : negZ)
			child->draw(globalOp);
		for (auto *child : normalFlow)
			child->draw(globalOp);
		for (auto *child : posZ)
			child->draw(globalOp);

		if (needsClip)
			renderer->popClip();

		if (hasShader && renderer->supports(Feature::Effects))
			renderer->popEffect();
		if (!identity)
			renderer->popTransform();

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

	void Layout::syncCssAnimations()
	{
		const auto &refs = targetStyle.animations;

		activeCssAnimations.erase(
			std::remove_if(activeCssAnimations.begin(), activeCssAnimations.end(),
						   [&](const ActiveCssAnimation &a)
						   {
							   for (const auto &r : refs)
								   if (r.name == a.name)
									   return false;
							   return true;
						   }),
			activeCssAnimations.end());

		for (const auto &r : refs)
		{
			bool found = false;
			for (auto &a : activeCssAnimations)
			{
				if (a.name == r.name)
				{
					a.duration = r.duration;
					a.delay = r.delay;
					a.iterations = r.iterations;
					a.alternate = r.alternate;
					a.fillForwards = r.fillForwards;
					a.ease = r.ease;
					found = true;
					break;
				}
			}
			if (!found)
			{
				ActiveCssAnimation a;
				a.name = r.name;
				a.duration = r.duration;
				a.delay = r.delay;
				a.iterations = r.iterations;
				a.alternate = r.alternate;
				a.fillForwards = r.fillForwards;
				a.ease = r.ease;
				activeCssAnimations.push_back(std::move(a));
			}
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

		Transform2D tr = currentTransform(currentStyle);
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

	static void collectFocusables(Layout *node, std::vector<Layout *> &out)
	{
		if (node->isFocusable())
			out.push_back(node);
		for (auto &c : node->children)
			collectFocusables(c.get(), out);
	}

	static void notifyFocusAncestors(Layout *node)
	{
		if (!node)
			return;
		auto parent = node->getParent();
		while (parent)
		{
			parent->notifyDescendantFocused(node);
			parent = parent->getParent();
		}
	}

	void Layout::updateTree(float dt)
	{
		auto &ctx = UIContext::get();

		auto captured = ctx.pointerCapture.lock();
		if (!captured || !captured->getEnabled())
		{
			ctx.pointerCapture.reset();
			ctx.topmostConsumer = hitTest(ctx.pointer.pos, false);
		}
		else
		{
			ctx.topmostConsumer = captured.get();
		}

		ctx.hoverTarget = ctx.topmostConsumer;
		if (ctx.pointer.pressed)
			ctx.pressTarget = ctx.hoverTarget;
		if (ctx.pointer.released)
			ctx.releaseTarget = ctx.hoverTarget;

		if (ctx.pointer.pressed)
		{
			if (ctx.topmostConsumer && ctx.topmostConsumer->isFocusable())
			{
				ctx.requestFocus(ctx.topmostConsumer->shared_from_this());
			}
			else
			{
				ctx.releaseFocus();
			}
		}

		for (int k : ctx.inputEvents.keys)
		{
			if (k != Key::Tab)
				continue;

			Layout *scope = this;
			{
				auto focused = ctx.focusedNode.lock();
				if (focused)
				{
					Layout *n = focused.get();
					while (n)
					{
						if (n->isFocusScope())
						{
							scope = n;
							break;
						}
						n = n->getParent().get();
					}
				}
			}

			std::vector<Layout *> focusables;
			collectFocusables(scope, focusables);
			if (focusables.empty())
				break;

			auto cur = ctx.focusedNode.lock();
			Layout *curPtr = cur.get();

			int idx = -1;
			for (size_t i = 0; i < focusables.size(); ++i)
				if (focusables[i] == curPtr)
				{
					idx = (int)i;
					break;
				}

			int next = 0;
			if (idx >= 0)
			{
				int n = (int)focusables.size();
				next = ctx.shiftHeld ? (idx - 1 + n) % n
									 : (idx + 1) % n;
			}
			ctx.requestFocus(focusables[next]->shared_from_this());
			notifyFocusAncestors(ctx.focusedNode.lock().get());
		}

		update(dt, false);

		if (ctx.pointer.released)
			ctx.pressTarget = nullptr;
		if (!ctx.pointer.down && !ctx.pointerCapture.expired())
			ctx.pointerCapture.reset();
	}

	void Layout::handleFocusInput()
	{
		if (!isFocused || !isEnabled || !keyboardActivates_)
			return;

		auto &ev = UIContext::get().inputEvents;
		for (int k : ev.keys)
		{
			if (k == Key::Enter || k == Key::Space)
			{
				if (onClick)
					onClick();
			}
		}
	}

	std::vector<const ThemeRule *> Layout::getMatchingRules() const
	{
		std::vector<const ThemeRule *> out;
		auto &theme = Theme::get();
		for (const auto &r : theme.rules)
		{
			if (!r.part.empty())
				continue;
			if (ruleMatches(r, this))
				out.push_back(&r);
		}
		return out;
	}
}

namespace ZenitUI
{
	void UIContext::requestFocus(std::shared_ptr<Layout> n)
	{
		if (n && n->isFocusable())
			focusedNode = n;
		else
			focusedNode.reset();
	}
}