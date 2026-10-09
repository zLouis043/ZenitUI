#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

	// =========================================================================
	//  STILE (StyleResolver)
	//  Cascata CSS (defaults + regole + inline + stato) e transizioni di stato.
	//  I ::part hanno una propria transizione interna per-part.
	// =========================================================================

	ComputedStyle StyleResolver::resolveFor(Layout &node)
	{
		auto &theme = Theme::get();

		struct Match
		{
			const ThemeRule *rule;
		};
		std::vector<Match> matches;

		for (const auto &r : theme.rules)
		{
			if (!r.part.empty())
				continue;
			if (!ruleMatches(r, &node))
				continue;
			matches.push_back({&r});
		}

		std::stable_sort(matches.begin(), matches.end(),
						 [](const Match &a, const Match &b)
						 {
							 if (a.rule->specificity != b.rule->specificity)
								 return a.rule->specificity < b.rule->specificity;
							 return a.rule->order < b.rule->order;
						 });

		Style finalStyle;
		finalStyle.overlay(inlineDefaults);
		for (const auto &m : matches)
			finalStyle.overlay(m.rule->style);
		finalStyle.overlay(inlineBase);

		// --- Custom properties ---
		// 1) Eredita dal parent (o dal root se siamo top-level).
		// 2) Merge con quelle dichiarate localmente (vincono).
		std::unordered_map<std::string, std::string> allCustomProps;
		if (auto p = node.getParent())
			allCustomProps = p->getStyle().customProps;
		else
			allCustomProps = theme.root.customProps;

		for (const auto &[k, v] : finalStyle.customProps)
			allCustomProps[k] = v;

		// --- Risoluzione delle prop che contengono var() ---
		if (!finalStyle.unresolvedProps.empty())
		{
			Style resolved = resolveUnresolvedProps(finalStyle.unresolvedProps, allCustomProps);
			finalStyle.overlay(resolved);
			finalStyle.unresolvedProps.clear();
		}

		// --- Stato (inline hover/pressed/focus/checked) ---
		if (!node.getEnabled())
			finalStyle.overlay(inlineDisabled);
		else
		{
			if (node.isHoveredState() || node.isPressedState())
				finalStyle.overlay(inlineHover);
			if (node.isPressedState())
				finalStyle.overlay(inlinePressed);
			if (node.isFocusedState())
				finalStyle.overlay(inlineFocus);
			if (node.isCheckedState())
				finalStyle.overlay(inlineChecked);
		}

		const ComputedStyle *parentStyle = nullptr;
		if (auto p = node.getParent())
			parentStyle = &p->getStyle();

		ComputedStyle result = ComputedStyle::from(finalStyle, parentStyle, &theme.root);
		result.customProps = std::move(allCustomProps);
		return result;
	}

	Style StyleResolver::partFor(Layout &node, const std::string &partName)
	{
		auto &theme = Theme::get();

		std::vector<const ThemeRule *> matches;
		for (const auto &r : theme.rules)
		{
			if (r.part != partName)
				continue;
			if (!ruleMatches(r, &node))
				continue;
			matches.push_back(&r);
		}

		std::stable_sort(matches.begin(), matches.end(),
						 [](const ThemeRule *a, const ThemeRule *b)
						 {
							 if (a->specificity != b->specificity)
								 return a->specificity < b->specificity;
							 return a->order < b->order;
						 });

		Style target;
		for (const auto *r : matches)
			target.overlay(r->style);

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
			float tp = 1.0f;
			if (st.active && st.duration > 0.001f)
				tp = (float)std::clamp((now - st.startTime) / st.duration, 0.0, 1.0);

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

		node.anim_.applyCssToPart(node, partName, result);

		return result;
	}

	void StyleResolver::beginStateTransition(Layout &node, UIState newState)
	{
		currentState = newState;
		transitionTimer = 0.0f;
		transitionStartStyle = currentStyle;
		targetStyle = resolveFor(node);
		node.pendingTransition = false; // <-- AGGIUNTO
	}

	void StyleResolver::resolvePendingTransition(Layout &node)
	{
		ComputedStyle newTarget = resolveFor(node);
		node.pendingTransition = false;

		if (newTarget != targetStyle)
		{
			if (node.anim_.hasActiveImperative())
			{
				// L'animazione imperativa riscrive lo stile ogni frame. Il
				// target cambia continuamente, quindi il sistema di
				// transizioni resetterebbe il timer a ogni frame e non
				// progredirebbe mai. Saltiamo la transizione e allineiamo
				// currentStyle al target immediatamente.
				currentStyle = newTarget;
				targetStyle = std::move(newTarget);
				transitionStartStyle = currentStyle;
				transitionTimer = 1.0f;
			}
			else
			{
				transitionStartStyle = currentStyle;
				transitionTimer = 0.0f;
				targetStyle = std::move(newTarget);
			}
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
	}

	void StyleResolver::tick(Layout & /*node*/, float dt)
	{
		if (transitionTimer >= 1.0f)
			return;

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

	void StyleResolver::propagateInheritance(Layout &node)
	{
		bool changed =
			lastInherited.hovered != node.isHoveredState() ||
			lastInherited.pressed != node.isPressedState() ||
			lastInherited.focused != node.isFocusedState() ||
			lastInherited.enabled != node.getEnabled() ||
			lastInherited.checked != node.isCheckedState() ||
			lastInherited.font != currentStyle.font ||
			!(lastInherited.fontSize == currentStyle.fontSize) ||
			!(lastInherited.color == currentStyle.color) ||
			!(lastInherited.letterSpacing == currentStyle.letterSpacing) ||
			!(lastInherited.textAlign == currentStyle.textAlign);

		if (!changed)
			return;

		for (auto &c : node.children)
			c->markInheritanceDirty();

		lastInherited.hovered = node.isHoveredState();
		lastInherited.pressed = node.isPressedState();
		lastInherited.focused = node.isFocusedState();
		lastInherited.enabled = node.getEnabled();
		lastInherited.checked = node.isCheckedState();
		lastInherited.font = currentStyle.font;
		lastInherited.fontSize = currentStyle.fontSize;
		lastInherited.color = currentStyle.color;
		lastInherited.letterSpacing = currentStyle.letterSpacing;
		lastInherited.textAlign = currentStyle.textAlign;
		lastInherited.overflowX = currentStyle.overflowX;
		lastInherited.overflowY = currentStyle.overflowY;
	}

} // namespace ZenitUI