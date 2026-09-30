#include "Layout.hpp"

namespace ZenitUI {

	ComputedStyle Layout::resolveTargetStyle(UIState state) {
		Style finalStyle;
		for (const auto& cls : styleClasses) {
			auto it = Theme::get().classes.find(cls);
			if (it != Theme::get().classes.end()) {
				finalStyle.overlay(it->second.base);
				if (state == UIState::Hover || state == UIState::Pressed) finalStyle.overlay(it->second.hover);
				if (state == UIState::Pressed) finalStyle.overlay(it->second.pressed);
			}
		}
		finalStyle.overlay(inlineBase);
		if (state == UIState::Hover || state == UIState::Pressed) finalStyle.overlay(inlineHover);
		if (state == UIState::Pressed) finalStyle.overlay(inlinePressed);

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
		ComputedStyle calcStyle = pendingTransition ? resolveTargetStyle(currentState) : currentStyle;

		float pl = calcStyle.padding.left.resolve(parent_w); float pr = calcStyle.padding.right.resolve(parent_w);
		float pt = calcStyle.padding.top.resolve(parent_h); float pb = calcStyle.padding.bottom.resolve(parent_h);

		float availW = calcStyle.width.isAuto() ? parent_w : calcStyle.width.resolve(parent_w);
		float availH = calcStyle.height.isAuto() ? parent_h : calcStyle.height.resolve(parent_h);

		float innerW = std::max(0.0f, availW - pl - pr); float innerH = std::max(0.0f, availH - pt - pb);

		Vec2 contentSize = computeIntrinsicSize(innerW, innerH);

		if (type == LayoutType::Stack) {
			for (auto& child : children) {
				Vec2 cs = child->measure(innerW, innerH);
				contentSize.x = std::max(contentSize.x, cs.x); contentSize.y = std::max(contentSize.y, cs.y);
			}
		} else if (type == LayoutType::Vertical) {
			float gap = calcStyle.gap.resolve(parent_h);
			for (size_t i = 0; i < children.size(); ++i) {
				Vec2 cs = children[i]->measure(innerW, innerH);
				contentSize.x = std::max(contentSize.x, cs.x); contentSize.y += cs.y + (i > 0 ? gap : 0.0f);
			}
		} else if (type == LayoutType::Horizontal) {
			float gap = calcStyle.gap.resolve(parent_w);
			for (size_t i = 0; i < children.size(); ++i) {
				Vec2 cs = children[i]->measure(innerW, innerH);
				contentSize.x += cs.x + (i > 0 ? gap : 0.0f); contentSize.y = std::max(contentSize.y, cs.y);
			}
		}

		float finalW = calcStyle.width.isAuto() ? contentSize.x + pl + pr : availW;
		float finalH = calcStyle.height.isAuto() ? contentSize.y + pt + pb : availH;
		measuredSize = { finalW, finalH };
		return measuredSize;
	}

	void Layout::arrange(Rect space) {
		rect = space;

		float pl = currentStyle.padding.left.resolve(rect.width); float pr = currentStyle.padding.right.resolve(rect.width);
		float pt = currentStyle.padding.top.resolve(rect.height); float pb = currentStyle.padding.bottom.resolve(rect.height);

		Rect inner = { rect.x + pl, rect.y + pt, std::max(0.0f, rect.width - pl - pr), std::max(0.0f, rect.height - pt - pb) };

		if (type == LayoutType::Vertical) {
			float current_y = inner.y; float gap = currentStyle.gap.resolve(inner.height);
			for (auto& child : children) {
				float cw = child->measuredSize.x; float ch = child->measuredSize.y; float cx = inner.x;
				if (currentStyle.itemsH == Align::Center) cx += (inner.width - cw) * 0.5f;
				else if (currentStyle.itemsH == Align::End) cx += inner.width - cw;
				else if (currentStyle.itemsH == Align::Stretch) cw = inner.width;
				child->arrange({ cx, current_y, cw, ch });
				current_y += ch + gap;
			}
		} else if (type == LayoutType::Horizontal) {
			float current_x = inner.x; float gap = currentStyle.gap.resolve(inner.width);
			for (auto& child : children) {
				float cw = child->measuredSize.x; float ch = child->measuredSize.y; float cy = inner.y;
				if (currentStyle.itemsV == Align::Center) cy += (inner.height - ch) * 0.5f;
				else if (currentStyle.itemsV == Align::End) cy += inner.height - ch;
				else if (currentStyle.itemsV == Align::Stretch) ch = inner.height;
				child->arrange({ current_x, cy, cw, ch });
				current_x += cw + gap;
			}
		} else if (type == LayoutType::Stack) {
			for (auto& child : children) {
				float cw = child->measuredSize.x; float ch = child->measuredSize.y;
				float cx = inner.x; float cy = inner.y;
				if (currentStyle.itemsH == Align::Stretch) cw = inner.width;
				if (currentStyle.itemsV == Align::Stretch) ch = inner.height;
				if (currentStyle.itemsH == Align::Center) cx += (inner.width - cw) * 0.5f;
				else if (currentStyle.itemsH == Align::End) cx += inner.width - cw;
				if (currentStyle.itemsV == Align::Center) cy += (inner.height - ch) * 0.5f;
				else if (currentStyle.itemsV == Align::End) cy += inner.height - ch;
				child->arrange({ cx, cy, cw, ch });
			}
		}
	}

	void Layout::update(float dt, bool ancestorBlocked) {
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

		// --- 2) Blocchi e hover ---
		bool blockSubtree = ancestorBlocked || localAnimBlocks;
		bool selfBlocked  = blockSubtree || (!isInteractive && blocksRaycast);

		auto& pointer = UIContext::get().pointer;
		isHovered = false;
		if (!selfBlocked && rect.width > 0 && rect.height > 0) isHovered = rect.contains(pointer.pos);

		UIState nextState = UIState::Idle;
		if (isHovered) nextState = pointer.down ? UIState::Pressed : UIState::Hover;

		// --- 3) Cambio stato ---
		bool stateChanged = (currentState != nextState);
		if (stateChanged) {
			if (nextState == UIState::Hover && onHoverEnter) onHoverEnter();
			else if (currentState == UIState::Hover && onHoverExit) onHoverExit();

			if (nextState == UIState::Pressed && onPress) onPress();
			else if (currentState == UIState::Pressed) {
				if (onRelease) onRelease();
				if (nextState == UIState::Hover && pointer.released && onClick) onClick();
			}
			currentState = nextState;
			transitionTimer = 0.0f;
			transitionStartStyle = currentStyle;
			targetStyle = resolveTargetStyle(currentState);
			syncCssAnimations();          // <-- AGGIUNTO
		} else if (pendingTransition) {
			targetStyle = resolveTargetStyle(currentState);
			pendingTransition = false;

			if (transitionTimer >= 1.0f) {
				currentStyle = targetStyle;
				transitionStartStyle = currentStyle;
			}
			syncCssAnimations();          // <-- AGGIUNTO
		}
		
		// --- 4) Transizione per-property ---
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

				// --- 5) Tick animazioni CSS ---
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

		// --- 6) Update figli ---
		size_t n = children.size();
		for (size_t i = 0; i < n; ++i) children[i]->update(dt, blockSubtree);

		// --- 7) Rimozioni ---
		for (auto it = children.begin(); it != children.end(); ) {
			if ((*it)->wantsRemoval) { (*it)->parent.reset(); it = children.erase(it); }
			else ++it;
		}

		onPostUpdate(dt);
	}

	void Layout::draw(float parentOpacity) {
		auto renderer = UIContext::get().renderer;
		if (!renderer) return;

		ComputedStyle renderStyle = currentStyle;
		for (const auto& anim : activeCssAnimations) {
			auto it = Theme::get().keyframes.find(anim.name);
			if (it == Theme::get().keyframes.end()) continue;
			bool fin = false;
			float localT = sampleActive(anim, fin);
			if (fin && !anim.fillForwards) continue;  // già finita e non persiste
			Style frame = evaluateKeyframes(it->second, localT, anim.alternate ? it->second.keyframes.size() > 0 ? TransitionFunction::Linear : TransitionFunction::Linear : TransitionFunction::Linear);
			// ↑ semplifichiamo: usiamo sempre Linear. L'ease è nel sampleActive se vuoi.
			overlayComputed(renderStyle, frame);
		}

		float globalOp = renderStyle.opacity * parentOpacity;
		if (globalOp <= 0.001f) return;

		Transform2D tr;
		tr.pivot = rect.center();
		tr.translate = { renderStyle.translateX.resolve(Metrics::viewport.x), renderStyle.translateY.resolve(Metrics::viewport.y) };
		tr.rotationDeg = renderStyle.rotation;
		tr.scale = renderStyle.scale;

		renderer->pushTransform(tr);
		if (hasShader && renderer->supports(Feature::Effects)) renderer->pushEffect(customEffect);

		renderSelf(globalOp, renderStyle);
		for (auto& child : children) child->draw(globalOp);

		if (hasShader && renderer->supports(Feature::Effects)) renderer->popEffect();
		renderer->popTransform();
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
}