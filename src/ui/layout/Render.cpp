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
			float m = 40.0f;

			if (rect.x > clip.x + clip.width + m ||
				rect.x + rect.width < clip.x - m ||
				rect.y > clip.y + clip.height + m ||
				rect.y + rect.height < clip.y - m)
			{
				return;
			}
		}

		ComputedStyle renderStyle = style_.currentStyle;
		Anim::overlayCssComputed(renderStyle, anim_.css);

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

		bool needsClip = renderStyle.overflowX != Overflow::Visible || renderStyle.overflowY != Overflow::Visible;
		if (needsClip)
			renderer->pushClip(rect);

		renderChrome(globalOp, renderStyle);
		renderContent(globalOp, renderStyle);

		drawNegZ_.clear();
		drawNormal_.clear();
		drawPosZ_.clear();
		for (auto &child : children)
		{
			if (child->isStackingContext())
			{
				if (child->getZIndex() < 0)
					drawNegZ_.push_back(child.get());
				else
					drawPosZ_.push_back(child.get());
			}
			else
			{
				drawNormal_.push_back(child.get());
			}
		}

		auto sortByZ = [](Layout *a, Layout *b)
		{ return a->getZIndex() < b->getZIndex(); };
		std::stable_sort(drawNegZ_.begin(), drawNegZ_.end(), sortByZ);
		std::stable_sort(drawPosZ_.begin(), drawPosZ_.end(), sortByZ);

		for (auto *child : drawNegZ_)
			child->draw(globalOp);
		for (auto *child : drawNormal_)
			child->draw(globalOp);
		for (auto *child : drawPosZ_)
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