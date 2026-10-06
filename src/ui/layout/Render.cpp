#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  RENDER
//  draw() disegna chrome + content del nodo, poi i figli ordinati per
//  stacking context (z-index). Applica clip/transform/effect e infine
//  la scrollbar se il nodo è un overflow:scroll.
// =========================================================================

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

	ComputedStyle renderStyle = style_.currentStyle;
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

	bool needsClip = renderStyle.overflowX != Overflow::Visible
	              || renderStyle.overflowY != Overflow::Visible;
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

	drawScrollbar(parentOpacity);

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

} // namespace ZenitUI