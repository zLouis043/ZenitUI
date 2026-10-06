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

	struct Match { const ThemeRule *rule; };
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
	finalStyle.overlay(inlineDefaults);     // "user agent": batteribile
	for (const auto &m : matches)
		finalStyle.overlay(m.rule->style);  // CSS autore
	finalStyle.overlay(inlineBase);         // inline: vince su tutto

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

	return ComputedStyle::from(finalStyle, parentStyle, &theme.root);
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

	// ---- Animazioni CSS del part ----
	auto &anims = activePartAnimations[partName];
	const auto &refs = result.animations.is_set
						   ? result.animations.value
						   : std::vector<AnimationRef>{};

	anims.erase(
		std::remove_if(anims.begin(), anims.end(),
					   [&](const ActiveCssAnimation &a)
					   {
						   for (const auto &r : refs)
							   if (r.name == a.name) return false;
						   return true;
					   }),
		anims.end());

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
			a.startTime = now;
			a.duration = r.duration;
			a.delay = r.delay;
			a.iterations = r.iterations;
			a.alternate = r.alternate;
			a.fillForwards = r.fillForwards;
			a.ease = r.ease;
			anims.push_back(std::move(a));
		}
	}

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

void StyleResolver::beginStateTransition(Layout &node, UIState newState)
{
	currentState = newState;
	transitionTimer = 0.0f;
	transitionStartStyle = currentStyle;
	targetStyle = resolveFor(node);
}

void StyleResolver::resolvePendingTransition(Layout &node)
{
	ComputedStyle newTarget = resolveFor(node);

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
		lastInherited.hovered       != node.isHoveredState()   ||
		lastInherited.pressed       != node.isPressedState()   ||
		lastInherited.focused       != node.isFocusedState()   ||
		lastInherited.enabled       != node.getEnabled()       ||
		lastInherited.checked       != node.isCheckedState()   ||
		lastInherited.font          != currentStyle.font       ||
		!(lastInherited.fontSize    == currentStyle.fontSize)  ||
		!(lastInherited.color       == currentStyle.color)     ||
		!(lastInherited.letterSpacing == currentStyle.letterSpacing) ||
		!(lastInherited.textAlign   == currentStyle.textAlign);

	if (!changed)
		return;

	for (auto &c : node.children)
		c->markInheritanceDirty();

	lastInherited.hovered       = node.isHoveredState();
	lastInherited.pressed       = node.isPressedState();
	lastInherited.focused       = node.isFocusedState();
	lastInherited.enabled       = node.getEnabled();
	lastInherited.checked       = node.isCheckedState();
	lastInherited.font          = currentStyle.font;
	lastInherited.fontSize      = currentStyle.fontSize;
	lastInherited.color         = currentStyle.color;
	lastInherited.letterSpacing = currentStyle.letterSpacing;
	lastInherited.textAlign     = currentStyle.textAlign;
	lastInherited.overflowX     = currentStyle.overflowX;
	lastInherited.overflowY     = currentStyle.overflowY;
}

} // namespace ZenitUI