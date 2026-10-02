#include "Layout.hpp"

namespace ZenitUI {

	ComputedStyle Layout::resolveTargetStyle() {
		auto& theme = Theme::get();

		// Raccogli gli StyleSet applicabili in ordine di specificità
		std::vector<const StyleSet*> sets;
		if (!styleTag.empty()) {
			auto it = theme.tags.find(styleTag);
			if (it != theme.tags.end()) sets.push_back(&it->second);
		}
		for (const auto& cls : styleClasses) {
			auto it = theme.classes.find(cls);
			if (it != theme.classes.end()) sets.push_back(&it->second);
		}
		if (!nodeId.empty()) {
			auto it = theme.ids.find(nodeId);
			if (it != theme.ids.end()) sets.push_back(&it->second);
		}

		Style finalStyle;

		// Fase 1: base di tutti i livelli (specificità crescente)
		for (auto* s : sets) finalStyle.overlay(s->base);
		finalStyle.overlay(inlineBase);

		if (!isEnabled) {
			for (auto* s : sets) finalStyle.overlay(s->disabled);
			finalStyle.overlay(inlineDisabled);
		} else {
			if (isHovered || isPressed) {
				for (auto* s : sets) finalStyle.overlay(s->hover);
				finalStyle.overlay(inlineHover);
			}
			if (isPressed) {
				for (auto* s : sets) finalStyle.overlay(s->pressed);
				finalStyle.overlay(inlinePressed);
			}
			if (isFocused) {
				for (auto* s : sets) finalStyle.overlay(s->focus);
				finalStyle.overlay(inlineFocus);
			}
		}

		return ComputedStyle::from(finalStyle);
	}

	void Layout::playAnimation(const std::string& name, bool playReverse) {
		auto it = activeAnimations.find(name);
		if (it == activeAnimations.end()) return;
		auto& state = it->second;
		state.reverse = playReverse;
		state.playing = true;
		if (!state.reverse && state.elapsed >= state.anim->duration) state.elapsed = 0.0f;
		else if (state.reverse && state.elapsed <= 0.0f) state.elapsed = state.anim->duration;
		state.delayElapsed = state.anim->delay;
	}

	bool Layout::hasActiveAnimations() const {
		for (const auto& [n, s] : activeAnimations) if (s.playing) return true;
		if (!activeCssAnimations.empty()) return true;
		for (const auto& c : children) if (c->hasActiveAnimations()) return true;
		return false;
	}

	Vec2 Layout::measure(float parent_w, float parent_h) {
		if (!styleInitialized) {
			styleInitialized = true;
			currentStyle = targetStyle = transitionStartStyle = resolveTargetStyle();
			pendingTransition = false;
		}
		ComputedStyle calcStyle = pendingTransition ? resolveTargetStyle() : currentStyle;

		// --- Spazio disponibile interno ---
		float pl = calcStyle.padding.left.resolve(parent_w);
		float pr = calcStyle.padding.right.resolve(parent_w);
		float pt = calcStyle.padding.top.resolve(parent_h);
		float pb = calcStyle.padding.bottom.resolve(parent_h);

		float availW = calcStyle.width.isAuto() ? parent_w : calcStyle.width.resolve(parent_w);
		float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolve(parent_h);

		float innerW = std::max(0.0f, availW - pl - pr);
		float innerH = std::max(0.0f, availH - pt - pb);

		// Se abbiamo lo scroll, togliamo i limiti di misurazione sull'asse principale
		if (calcStyle.overflow == Overflow::Scroll) {
			if (type == LayoutType::Vertical)   innerH = 1e9f;
			if (type == LayoutType::Horizontal) innerW = 1e9f;
		}

		// --- Misurazione Intrinseca del Nodo (es. Testo) ---
		Vec2 contentSize = computeIntrinsicSize(innerW, innerH);
		
		float gapX = calcStyle.gap.resolve(innerW);
		float gapY = calcStyle.gap.resolve(innerH);

		float totalMainSize = 0.0f;
		float maxCrossSize = 0.0f;
		int visibleChildren = 0;

		// --- Misura tutti i figli ---
		for (auto& c : children) {
			Vec2 cs = c->measure(innerW, innerH);
			if (c->getStyle().position == Position::Absolute || c->isPortal()) continue;

			const auto& cst = c->getStyle();
			float ml = cst.margin.left.resolve(innerW);
			float mr = cst.margin.right.resolve(innerW);
			float mt = cst.margin.top.resolve(innerH);
			float mb = cst.margin.bottom.resolve(innerH);

			float childTotalW = cs.x + ml + mr;
			float childTotalH = cs.y + mt + mb;

			if (type == LayoutType::Horizontal) {
				totalMainSize += childTotalW;
				maxCrossSize = std::max(maxCrossSize, childTotalH);
			} else if (type == LayoutType::Vertical) {
				totalMainSize += childTotalH;
				maxCrossSize = std::max(maxCrossSize, childTotalW);
			} else if (type == LayoutType::Stack) {
				contentSize.x = std::max(contentSize.x, childTotalW);
				contentSize.y = std::max(contentSize.y, childTotalH);
			}
			visibleChildren++;
		}

		// --- Aggiungi i gap ---
		if (type == LayoutType::Horizontal) {
			if (visibleChildren > 1) totalMainSize += gapX * (visibleChildren - 1);
			contentSize.x = std::max(contentSize.x, totalMainSize);
			contentSize.y = std::max(contentSize.y, maxCrossSize);
		} else if (type == LayoutType::Vertical) {
			if (visibleChildren > 1) totalMainSize += gapY * (visibleChildren - 1);
			contentSize.x = std::max(contentSize.x, maxCrossSize);
			contentSize.y = std::max(contentSize.y, totalMainSize);
		}

		// Salviamo l'ingombro reale non tagliato per gestire i limiti dello ScrollView
		scrollContentSize = contentSize;

		// --- Risoluzione Dimensioni Finali del Genitore ---
		float finalW = calcStyle.width.isAuto() ? (contentSize.x + pl + pr) : availW;
		float finalH = calcStyle.height.isAuto() ? (contentSize.y + pt + pb) : availH;

		float minW = calcStyle.minWidth.isAuto()  ? 0.0f : calcStyle.minWidth.resolve(parent_w);
		float maxW = calcStyle.maxWidth.isAuto()  ? 1e9f : calcStyle.maxWidth.resolve(parent_w);
		float minH = calcStyle.minHeight.isAuto() ? 0.0f : calcStyle.minHeight.resolve(parent_h);
		float maxH = calcStyle.maxHeight.isAuto() ? 1e9f : calcStyle.maxHeight.resolve(parent_h);

		measuredSize = { std::clamp(finalW, minW, maxW), std::clamp(finalH, minH, maxH) };
		return measuredSize;
	}

	void Layout::arrange(Rect space) {
		rect = space;
		arrangeInto(space);
	}

	void Layout::arrangeInto(Rect space) {
		float pl = currentStyle.padding.left.resolve(space.width);
		float pr = currentStyle.padding.right.resolve(space.width);
		float pt = currentStyle.padding.top.resolve(space.height);
		float pb = currentStyle.padding.bottom.resolve(space.height);

		Rect inner = {
			space.x + pl,
			space.y + pt,
			std::max(0.0f, space.width  - pl - pr),
			std::max(0.0f, space.height - pt - pb)
		};

		float gapX = currentStyle.gap.resolve(inner.width);
		float gapY = currentStyle.gap.resolve(inner.height);

		// ---- Pre-pass: figli absolute (fuori dal flow) ----
		// Vengono posizionati rispetto a `inner` con le regole CSS top/left/right/bottom.
		auto arrangeAbsolute = [](Layout* c, const ComputedStyle& cst, const Rect& in) {
			float ml = cst.margin.left.resolve(in.width);
			float mr = cst.margin.right.resolve(in.width);
			float mt = cst.margin.top.resolve(in.height);
			float mb = cst.margin.bottom.resolve(in.height);

			bool hasL = !cst.left.isAuto();
			bool hasR = !cst.right.isAuto();
			bool hasT = !cst.top.isAuto();
			bool hasB = !cst.bottom.isAuto();

			float w = c->measuredSize.x;
			float h = c->measuredSize.y;
			float x, y;

			// ---- orizzontale ----
			if (hasL && hasR && cst.width.isAuto()) {
				float L = cst.left.resolve(in.width);
				float R = cst.right.resolve(in.width);
				w = std::max(0.0f, in.width - L - R - ml - mr);
				x = in.x + L + ml;
			} else if (hasL) {
				x = in.x + cst.left.resolve(in.width) + ml;
			} else if (hasR) {
				x = in.x + in.width - w - cst.right.resolve(in.width) - mr;
			} else {
				x = in.x + ml;
			}

			// ---- verticale ----
			if (hasT && hasB && cst.height.isAuto()) {
				float T = cst.top.resolve(in.height);
				float B = cst.bottom.resolve(in.height);
				h = std::max(0.0f, in.height - T - B - mt - mb);
				y = in.y + T + mt;
			} else if (hasT) {
				y = in.y + cst.top.resolve(in.height) + mt;
			} else if (hasB) {
				y = in.y + in.height - h - cst.bottom.resolve(in.height) - mb;
			} else {
				y = in.y + mt;
			}

			c->arrange({ x, y, w, h });
		};

		for (auto& c : children) {
			const auto& cst = c->getStyle();
			if (c->isPortal()) {
				// I portal li arrangia chi li gestisce (es. Dropdown::arrange);
				// qui diamo una posizione di default per compatibilità.
				c->arrange({ inner.x, inner.y, c->measuredSize.x, c->measuredSize.y });
				continue;
			}
			if (cst.position == Position::Absolute) {
				arrangeAbsolute(c.get(), cst, inner);
			}
		}

		// Clamp "safe": evita __glibcxx_assert quando lo > hi
		// (su MinGW-GCC 15 questo chiama abort()). Quando lo > hi prevale hi,
		// cioè lo spazio disponibile vince sul min-width/min-height dichiarato.
		auto clampSafe = [](float v, float lo, float hi) -> float {
			if (hi < lo) return hi;
			return std::clamp(v, lo, hi);
		};

		// =====================================================================
		//  VERTICAL: main axis = height, cross axis = width
		// =====================================================================
		if (type == LayoutType::Vertical) {
			float totalFixedH = 0.0f, totalGrow = 0.0f, totalShrink = 0.0f;
			int visibleChildren = 0;

			for (auto& c : children) {
				const auto& cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal()) continue;
				totalFixedH += cst.margin.top.resolve(inner.height)
							+ c->measuredSize.y
							+ cst.margin.bottom.resolve(inner.height);
				totalGrow   += cst.grow;
				totalShrink += cst.shrink;
				visibleChildren++;
			}
			if (visibleChildren > 1) totalFixedH += gapY * (visibleChildren - 1);

			float freeSpace = inner.height - totalFixedH;
			float startY = inner.y;
			float extraGap = 0.0f;

			if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0) {
				switch (currentStyle.justify) {
					case Justify::Center: startY += freeSpace * 0.5f; break;
					case Justify::End:    startY += freeSpace;        break;
					case Justify::SpaceBetween:
						if (visibleChildren > 1) extraGap = freeSpace / (visibleChildren - 1);
						break;
					default: break;
				}
			}

			float curY = startY;

			for (auto& c : children) {
				const auto& cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal()) continue;

				float ml = cst.margin.left.resolve(inner.width);
				float mr = cst.margin.right.resolve(inner.width);
				float mt = cst.margin.top.resolve(inner.height);
				float mb = cst.margin.bottom.resolve(inner.height);

				// ---- main axis: height ----
				float ch = c->measuredSize.y;
				if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
					ch += freeSpace * (cst.grow / totalGrow);
				else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
					ch += freeSpace * (cst.shrink / totalShrink);

				float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolve(inner.height);
				float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolve(inner.height);
				ch = clampSafe(ch, minH, maxH);

				// ---- cross axis: width ----
				float cw = c->measuredSize.x;
				float availW = std::max(0.0f, inner.width - ml - mr);
				float minW   = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolve(inner.width);
				float maxW   = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolve(inner.width);
				cw = clampSafe(cw, minW, std::min(availW, maxW));

				float cx = inner.x + ml;
				Align aH = (cst.alignH == Align::Auto) ? currentStyle.itemsH : cst.alignH;
				if (aH == Align::Stretch)
					cw = clampSafe(availW, minW, std::min(availW, maxW));
				else if (aH == Align::Center) cx += (availW - cw) * 0.5f;
				else if (aH == Align::End)    cx += availW - cw;

				c->arrange({ cx, curY + mt, cw, ch });
				curY += mt + ch + mb + gapY + extraGap;
			}

		// =====================================================================
		//  HORIZONTAL: main axis = width, cross axis = height
		// =====================================================================
		} else if (type == LayoutType::Horizontal) {
			float totalFixedW = 0.0f, totalGrow = 0.0f, totalShrink = 0.0f;
			int visibleChildren = 0;

			for (auto& c : children) {
				const auto& cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal()) continue;
				totalFixedW += cst.margin.left.resolve(inner.width)
							+ c->measuredSize.x
							+ cst.margin.right.resolve(inner.width);
				totalGrow   += cst.grow;
				totalShrink += cst.shrink;
				visibleChildren++;
			}
			if (visibleChildren > 1) totalFixedW += gapX * (visibleChildren - 1);

			float freeSpace = inner.width - totalFixedW;
			float startX = inner.x;
			float extraGap = 0.0f;

			if (freeSpace > 0.0f && totalGrow <= 0.0f && visibleChildren > 0) {
				switch (currentStyle.justify) {
					case Justify::Center: startX += freeSpace * 0.5f; break;
					case Justify::End:    startX += freeSpace;        break;
					case Justify::SpaceBetween:
						if (visibleChildren > 1) extraGap = freeSpace / (visibleChildren - 1);
						break;
					default: break;
				}
			}

			float curX = startX;

			for (auto& c : children) {
				const auto& cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal()) continue;

				float ml = cst.margin.left.resolve(inner.width);
				float mr = cst.margin.right.resolve(inner.width);
				float mt = cst.margin.top.resolve(inner.height);
				float mb = cst.margin.bottom.resolve(inner.height);

				// ---- main axis: width ----
				float cw = c->measuredSize.x;
				if (freeSpace > 0.0f && totalGrow > 0.0f && cst.grow > 0.0f)
					cw += freeSpace * (cst.grow / totalGrow);
				else if (freeSpace < 0.0f && totalShrink > 0.0f && cst.shrink > 0.0f)
					cw += freeSpace * (cst.shrink / totalShrink);

				float minW = cst.minWidth.isAuto() ? 0.0f : cst.minWidth.resolve(inner.width);
				float maxW = cst.maxWidth.isAuto() ? 1e9f : cst.maxWidth.resolve(inner.width);
				cw = clampSafe(cw, minW, maxW);

				// ---- cross axis: height ----
				float ch = c->measuredSize.y;
				float availH = std::max(0.0f, inner.height - mt - mb);
				float minH   = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolve(inner.height);
				float maxH   = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolve(inner.height);
				ch = clampSafe(ch, minH, std::min(availH, maxH));

				float cy = inner.y + mt;
				Align aV = (cst.alignV == Align::Auto) ? currentStyle.itemsV : cst.alignV;
				if (aV == Align::Stretch)
					ch = clampSafe(availH, minH, std::min(availH, maxH));
				else if (aV == Align::Center) cy += (availH - ch) * 0.5f;
				else if (aV == Align::End)    cy += availH - ch;

				c->arrange({ curX + ml, cy, cw, ch });
				curX += ml + cw + mr + gapX + extraGap;
			}

		// =====================================================================
		//  STACK: figli sovrapposti
		// =====================================================================
		} else if (type == LayoutType::Stack) {
			for (auto& c : children) {
				const auto& cst = c->getStyle();
				if (cst.position == Position::Absolute || c->isPortal()) continue;

				float ml = cst.margin.left.resolve(inner.width);
				float mr = cst.margin.right.resolve(inner.width);
				float mt = cst.margin.top.resolve(inner.height);
				float mb = cst.margin.bottom.resolve(inner.height);

				float availW = std::max(0.0f, inner.width  - ml - mr);
				float availH = std::max(0.0f, inner.height - mt - mb);

				float minW = cst.minWidth.isAuto()  ? 0.0f : cst.minWidth.resolve(inner.width);
				float maxW = cst.maxWidth.isAuto()  ? 1e9f : cst.maxWidth.resolve(inner.width);
				float minH = cst.minHeight.isAuto() ? 0.0f : cst.minHeight.resolve(inner.height);
				float maxH = cst.maxHeight.isAuto() ? 1e9f : cst.maxHeight.resolve(inner.height);

				float cw = clampSafe(c->measuredSize.x, minW, std::min(availW, maxW));
				float ch = clampSafe(c->measuredSize.y, minH, std::min(availH, maxH));

				float cx = inner.x + ml;
				float cy = inner.y + mt;

				Align aH = (cst.alignH == Align::Auto) ? currentStyle.itemsH : cst.alignH;
				Align aV = (cst.alignV == Align::Auto) ? currentStyle.itemsV : cst.alignV;

				if (aH == Align::Stretch)
					cw = clampSafe(availW, minW, std::min(availW, maxW));
				else if (aH == Align::Center) cx += (availW - cw) * 0.5f;
				else if (aH == Align::End)    cx += availW - cw;

				if (aV == Align::Stretch)
					ch = clampSafe(availH, minH, std::min(availH, maxH));
				else if (aV == Align::Center) cy += (availH - ch) * 0.5f;
				else if (aV == Align::End)    cy += availH - ch;

				c->arrange({ cx, cy, cw, ch });
			}
		}
	}

	void Layout::update(float dt, bool ancestorBlocked) {

		if (!styleInitialized) {
			styleInitialized = true;
			currentStyle = targetStyle = transitionStartStyle = resolveTargetStyle();
			pendingTransition = false;
			syncCssAnimations();
		}

		onPreUpdate(dt);

		// --- 1) Animazioni imperative ---
		bool localAnimBlocks = false;
		for (auto& [n, s] : activeAnimations) {
			if (!s.playing) continue;
			if (s.anim->blocksInput) localAnimBlocks = true;
			if (s.delayElapsed > 0.0f) { s.delayElapsed -= dt; continue; }

			s.elapsed += (s.reverse ? -dt : dt);
			if (s.elapsed <= 0.0f)             { s.elapsed = 0.0f; s.playing = false; }
			if (s.elapsed >= s.anim->duration) { s.elapsed = s.anim->duration; s.playing = false; }

			float t = (s.anim->duration > 0.0f) ? (s.elapsed / s.anim->duration) : 1.0f;
			for (auto& track : s.anim->tracks) track->apply(t, this);
			pendingTransition = true;
		}

		// --- 2) Blocchi e UPDATE DEI FIGLI (Bottom-Up) ---
		bool blockSubtree = ancestorBlocked || localAnimBlocks;
		bool selfBlocked  = blockSubtree || !isEnabled || (!isInteractive && blocksRaycast);

		// Eseguiamo i figli PRIMA di risolvere gli eventi locali. Questo garantisce il Bubbling!
		size_t n = children.size();
		for (size_t i = 0; i < n; ++i) children[i]->update(dt, blockSubtree);

		auto& ctx = UIContext::get();
		auto& pointer = ctx.pointer;

		// --- 3) Calcolo Hover & Pressed (Bubbling Chain) ---
		// Un nodo è Hovered se lui STESSO o uno dei suoi SOTTO-NODI è il topmostConsumer
		isHovered = false;
		if (!selfBlocked && rect.width > 0 && rect.height > 0) {
			Layout* trace = ctx.topmostConsumer;
			while (trace) {
				if (trace == this) { isHovered = true; break; }
				
				// IL FIX CHIAVE:
				// Se il nodo lungo la catena blocca gli eventi (passThrough == false),
				// la propagazione VISIVA si ferma. Il padre non si illuminerà!
				if (!trace->getPassThrough()) {
					break;
				}
				
				trace = trace->getParent().get();
			}
		}

		isPressed = isHovered && pointer.down;

		bool focusNow = ctx.hasFocus(this);
		if (focusNow != isFocused) {
			isFocused = focusNow;
			pendingTransition = true;
		}

		handleFocusInput();

		// --- 4) Cambio stato ed esecuzione Callback ---
		UIState nextState = UIState::Idle;
		if (!isEnabled)         nextState = UIState::Disabled;
		else if (isPressed)     nextState = UIState::Pressed;
		else if (isHovered)     nextState = UIState::Hover;

		bool stateChanged = (currentState != nextState);
		if (stateChanged) {
			if (nextState == UIState::Hover && onHoverEnter) onHoverEnter();
			else if (currentState == UIState::Hover && onHoverExit) onHoverExit();

			if (nextState == UIState::Pressed && onPress) onPress();
			else if (currentState == UIState::Pressed) {
				if (onRelease) onRelease();
				
				if (nextState == UIState::Hover && pointer.released && onClick) {
					if (!ctx.clickConsumed) {
						onClick();
						
						// Se non è impostato il passthrough, blocchiamo la propagazione
						if (!passThrough_) {
							ctx.consumeClick();
						}
					}
				}
			}
			currentState = nextState;
			transitionTimer = 0.0f;
			transitionStartStyle = currentStyle;
			targetStyle = resolveTargetStyle();
			pendingTransition = false;
			syncCssAnimations();
		} else if (pendingTransition) {
			targetStyle = resolveTargetStyle();
			pendingTransition = false;

			if (transitionTimer >= 1.0f) {
				currentStyle = targetStyle;
				transitionStartStyle = currentStyle;
			}
			syncCssAnimations();
		}
		
		// --- 5) Transizione per-property ---
		if (transitionTimer < 1.0f) {
			float maxDur = targetStyle.transitionTime;
			for (const auto& spec : targetStyle.transitions) maxDur = std::max(maxDur, spec.duration);
			if (maxDur <= 0.001f) {
				transitionTimer = 1.0f;
				currentStyle = targetStyle;
			} else {
				float elapsed = transitionTimer * maxDur;
				transitionTimer += dt / maxDur;
				if (transitionTimer >= 1.0f) {
					transitionTimer = 1.0f;
					currentStyle = targetStyle;
				} else {
					currentStyle = lerpStyleTimed(transitionStartStyle, targetStyle, elapsed);
				}
			}
		}

		// --- 6) Tick animazioni CSS ---
		for (auto& anim : activeCssAnimations) {
			if (anim.finished) continue;
			anim.elapsed += dt;
			auto it = Theme::get().keyframes.find(anim.name);
			if (it == Theme::get().keyframes.end()) { anim.finished = true; continue; }
			bool fin = false;
			(void)sampleActive(anim, fin);
			if (fin) anim.finished = true;
		}
		activeCssAnimations.erase(
			std::remove_if(activeCssAnimations.begin(), activeCssAnimations.end(),
				[](const ActiveCssAnimation& a) { return a.finished && !a.fillForwards; }),
			activeCssAnimations.end()
		);

		// --- 7) Rimozioni ---
		for (auto it = children.begin(); it != children.end(); ) {
			if ((*it)->wantsRemoval) { (*it)->parent.reset(); it = children.erase(it); }
			else ++it;
		}

		onPostUpdate(dt);
	}

	void Layout::draw(float parentOpacity) {

		if (isPortal_) {
			UIContext::get().framePortals.push_back(weak_from_this());
			return;
		}

		auto renderer = UIContext::get().renderer;
		if (!renderer) return;

		if (rect.width > 0.0f && rect.height > 0.0f) {
			Rect clip = renderer->getClipRect();
			float m = 300.0f;

			if (rect.x > clip.x + clip.width + m ||
			    rect.x + rect.width < clip.x - m ||
			    rect.y > clip.y + clip.height + m ||
			    rect.y + rect.height < clip.y - m) {
				return; 
			}
		}

		ComputedStyle renderStyle = currentStyle;
		for (const auto& anim : activeCssAnimations) {
			auto it = Theme::get().keyframes.find(anim.name);
			if (it == Theme::get().keyframes.end()) continue;
			bool fin = false;
			float localT = sampleActive(anim, fin);
			if (fin && !anim.fillForwards) continue;
			Style frame = evaluateKeyframes(it->second, localT, anim.ease);
			overlayComputed(renderStyle, frame);
		}

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f) return;

		Transform2D tr = currentTransform(renderStyle);

		const bool identity =
			tr.translate.x == 0.0f && tr.translate.y == 0.0f &&
			tr.rotationDeg == 0.0f && tr.scale == 1.0f;

		if (!identity) renderer->pushTransform(tr);
		if (hasShader && renderer->supports(Feature::Effects)) renderer->pushEffect(customEffect);

		bool needsClip = (renderStyle.overflow == Overflow::Hidden ||
		                  renderStyle.overflow == Overflow::Scroll);
		if (needsClip) renderer->pushClip(rect);

		renderChrome(globalOp, renderStyle);
		renderContent(globalOp, renderStyle);

		// --- RENDERING CSS COMPLIANT (Stacking Context) ---
		std::vector<Layout*> negZ, normalFlow, posZ;
		for (auto& child : children) {
			if (child->isStackingContext()) {
				if (child->getZIndex() < 0) negZ.push_back(child.get());
				else posZ.push_back(child.get());
			} else {
				normalFlow.push_back(child.get());
			}
		}

		auto sortByZ = [](Layout* a, Layout* b) { return a->getZIndex() < b->getZIndex(); };
		std::stable_sort(negZ.begin(), negZ.end(), sortByZ);
		std::stable_sort(posZ.begin(), posZ.end(), sortByZ);

		for (auto* child : negZ)       child->draw(globalOp);
		for (auto* child : normalFlow) child->draw(globalOp);
		for (auto* child : posZ)       child->draw(globalOp);
		// ----------------------------------------------------

		if (needsClip) renderer->popClip();

		if (hasShader && renderer->supports(Feature::Effects)) renderer->popEffect();
		if (!identity) renderer->popTransform();

		if (!hasParent()) {
			auto portals = UIContext::get().framePortals; 
			for (auto& wp  : portals) {
				auto sp = wp.lock();
    			if (!sp) continue;
				bool wasPortal = sp->isPortal();
				sp->setPortal(false);
				sp->draw(1.0f);
				sp->setPortal(wasPortal);
			}
		}
	}

	void Layout::syncCssAnimations() {
		const auto& refs = targetStyle.animations;

		// 1) Rimuovi quelle non più dichiarate
		activeCssAnimations.erase(
			std::remove_if(activeCssAnimations.begin(), activeCssAnimations.end(),
				[&](const ActiveCssAnimation& a) {
					for (const auto& r : refs) if (r.name == a.name) return false;
					return true;
				}),
			activeCssAnimations.end()
		);

		// 2) Aggiungi/aggiorna quelle dichiarate
		for (const auto& r : refs) {
			bool found = false;
			for (auto& a : activeCssAnimations) {
				if (a.name == r.name) {
					a.duration     = r.duration;
					a.delay        = r.delay;
					a.iterations   = r.iterations;
					a.alternate    = r.alternate;
					a.fillForwards = r.fillForwards;
					a.ease         = r.ease;      // <-- AGGIUNTO
					found = true;
					break;
				}
			}
			if (!found) {
				ActiveCssAnimation a;
				a.name         = r.name;
				a.duration     = r.duration;
				a.delay        = r.delay;
				a.iterations   = r.iterations;
				a.alternate    = r.alternate;
				a.fillForwards = r.fillForwards;
				a.ease         = r.ease;      // <-- AGGIUNTO
				activeCssAnimations.push_back(std::move(a));
			}
		}
	}

	Layout* Layout::hitTest(Vec2 p, bool ancestorBlocked) {
		if (ancestorBlocked || !isEnabled) return nullptr;

		// Portali (invariato)
		if (!hasParent()) {
			for (auto it = UIContext::get().activePortals.rbegin(); it != UIContext::get().activePortals.rend(); ++it) {
				auto sp = it->lock();
				if (!sp) continue;
				bool wasPortal = sp->isPortal();
				sp->setPortal(false);
				Layout* hit = sp->hitTest(p, false);
				sp->setPortal(wasPortal);
				if (hit) return hit;
			}
		}
		if (isPortal_) return nullptr;

		// I figli sono disegnati DOPO che il mio transform è applicato.
		// Quindi per testarli devo trasformare p nello spazio dei figli.
		Transform2D tr = currentTransform(currentStyle); 
		Vec2 pChildren = applyInverseTransform(tr, p);

		// Ordine z (invariato)
		std::vector<Layout*> negZ, normalFlow, posZ;
		for (auto& child : children) {
			if (child->isStackingContext()) {
				if (child->getZIndex() < 0) negZ.push_back(child.get());
				else posZ.push_back(child.get());
			} else {
				normalFlow.push_back(child.get());
			}
		}
		auto sortByZ = [](Layout* a, Layout* b) { return a->getZIndex() < b->getZIndex(); };
		std::stable_sort(negZ.begin(), negZ.end(), sortByZ);
		std::stable_sort(posZ.begin(), posZ.end(), sortByZ);

		for (auto it = posZ.rbegin(); it != posZ.rend(); ++it)
			if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;
		for (auto it = normalFlow.rbegin(); it != normalFlow.rend(); ++it)
			if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;
		for (auto it = negZ.rbegin(); it != negZ.rend(); ++it)
			if (Layout* hit = (*it)->hitTest(pChildren, false)) return hit;

		// Self test: p (in coordinate del parent) contro il mio rect
		if (rect.width > 0 && rect.height > 0 &&
			rect.contains(p) &&
			(isInteractive || blocksRaycast)) {
			return this;
		}
		return nullptr;
	}

	static void collectFocusables(Layout* node, std::vector<Layout*>& out) {
		if (node->isFocusable()) out.push_back(node);
		for (auto& c : node->children) collectFocusables(c.get(), out);
	}

	static void notifyFocusAncestors(Layout* node) {
		if (!node) return;
		auto parent = node->getParent();
		while (parent) {
			parent->notifyDescendantFocused(node);
			parent = parent->getParent();
		}
	}

	void Layout::updateTree(float dt) {
		auto& ctx = UIContext::get();

		// 1) Hit-test topmost
		ctx.topmostConsumer = hitTest(ctx.pointer.pos, false);

		// 2) Focus management
		// 2a) Click con mouse → cambia focus
		if (ctx.pointer.pressed) {
			if (ctx.topmostConsumer && ctx.topmostConsumer->isFocusable()) {
				ctx.requestFocus(ctx.topmostConsumer->shared_from_this());
			} else {
				ctx.releaseFocus();
			}
		}

		// 2b) Tab navigation
		for (int k : ctx.inputEvents.keys) {
			if (k != Key::Tab) continue;

			// Trova lo scope: risali dal nodo focusato fino al primo focus-scope.
			// Se nessuno è focusato, o nessuno scope trovato, usa this (root).
			Layout* scope = this;
			{
				auto focused = ctx.focusedNode.lock();
				if (focused) {
					Layout* n = focused.get();
					while (n) {
						if (n->isFocusScope()) { scope = n; break; }
						n = n->getParent().get();
					}
				}
			}

			std::vector<Layout*> focusables;
			collectFocusables(scope, focusables);
			if (focusables.empty()) break;

			auto cur = ctx.focusedNode.lock();
			Layout* curPtr = cur.get();

			int idx = -1;
			for (size_t i = 0; i < focusables.size(); ++i)
				if (focusables[i] == curPtr) { idx = (int)i; break; }

			int next = 0;
			if (idx >= 0) {
				int n = (int)focusables.size();
				next = ctx.shiftHeld ? (idx - 1 + n) % n
				                     : (idx + 1) % n;
			}
			ctx.requestFocus(focusables[next]->shared_from_this());
			notifyFocusAncestors(ctx.focusedNode.lock().get());
		}

		// 3) Update normale (con defer automatico)
		update(dt, false);
	}

	void Layout::handleFocusInput() {
		if (!isFocused || !isEnabled || !keyboardActivates_) return;

		auto& ev = UIContext::get().inputEvents;
		for (int k : ev.keys) {
			if (k == Key::Enter || k == Key::Space) {
				if (onClick) onClick();
			}
		}
	}
}