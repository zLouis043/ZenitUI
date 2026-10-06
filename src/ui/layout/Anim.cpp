#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  ANIMAZIONI
//  Imperative: track programmatiche su valori (addAnimation/playAnimation).
//  CSS: derivate da `animations` nello stile; sincronizzate quando cambia
//  il targetStyle e tickate ogni frame.
// =========================================================================

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

bool Layout::tickImperativeAnimations(float dt)
{
	bool localBlocks = false;

	for (auto &[n, s] : activeAnimations)
	{
		if (!s.playing)
			continue;

		if (s.anim->blocksInput)
			localBlocks = true;

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

	return localBlocks;
}

void Layout::tickCssAnimations(float dt)
{
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
}

void Layout::syncCssAnimations()
{
	const auto &refs = style_.targetStyle.animations;

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

} // namespace ZenitUI