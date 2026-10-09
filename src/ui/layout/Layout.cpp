#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

	// =========================================================================
	//  ORCHESTRAZIONE
	//  update() coordina i sottosistemi. Non contiene logica di dettaglio:
	//  quella vive in Style/Anim/Input/Scroll/Measure/Render.
	// =========================================================================

	void Layout::update(float dt, bool ancestorBlocked, bool ancestorScrolling)
	{
		bool wasPending = pendingTransition;
		ComputedStyle styleBefore = style_.currentStyle;

		initStyleIfNeeded();

		// Se il viewport è cambiato dall'ultimo frame, le media query CSS
		// possono aver cambiato l'esito: forziamo una ri-risoluzione.
		uint32_t gen = UIContext::get().viewportGeneration;
		if (gen != lastViewportGen_)
		{
			lastViewportGen_ = gen;
			pendingTransition = true;
		}

		bool localAnimBlocks = anim_.tickImperative(*this, dt);
		bool blockSubtree = ancestorBlocked || localAnimBlocks;
		bool selfBlocked = blockSubtree || !isEnabled || (!isInteractive && blocksRaycast);

		const bool scrolling = ancestorScrolling || scroll_.scrolling;
		input_.updateFlags(*this, selfBlocked, scrolling);

		UIState prevState = style_.currentState;
		UIState nextState = input_.computeNextState(*this);
		bool stateChanged = (style_.currentState != nextState);

		if (stateChanged)
		{
			style_.beginStateTransition(*this, nextState);
			anim_.syncCss(*this);
		}
		else if (pendingTransition)
		{
			style_.resolvePendingTransition(*this);
			anim_.syncCss(*this);
		}

		style_.tick(*this, dt);
		anim_.tickCss(*this, dt);
		style_.propagateInheritance(*this);

		scroll_.resetIfOverflowChanged(*this);

		{
			bool anyLocal = anim_.hasActiveLocal();
			if (hadLocalAnimationsLast_ && !anyLocal && onAnimationsFinished)
				onAnimationsFinished();
			hadLocalAnimationsLast_ = anyLocal;
		}

		{
			size_t n = children.size();
			for (size_t i = 0; i < n; ++i)
				children[i]->update(dt, blockSubtree, scrolling);
		}

		scroll_.tickInput(*this);
		input_.handleKeyInput(*this);
		input_.fireCallbacks(*this, prevState, nextState, stateChanged);
		cullRemovedChildren();
		recomputeDirty(wasPending, styleBefore);

		if (isEnabled || updateWhenDisabled_)
			onUpdate(dt);
	}

	void Layout::initStyleIfNeeded()
	{
		if (!styleInitialized)
		{
			styleInitialized = true;
			style_.currentStyle = style_.targetStyle = style_.transitionStartStyle = style_.resolveFor(*this);
			pendingTransition = false;
			anim_.syncCss(*this);
		}
	}
	void Layout::cullRemovedChildren()
	{
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
	}

	void Layout::recomputeDirty(bool wasPending, const ComputedStyle &styleBefore)
	{
		bool anyChildDirty = false;
		for (auto &c : children)
		{
			if (c->subtreeDirty_)
			{
				anyChildDirty = true;
				break;
			}
		}

		bool styleChanged = (style_.currentStyle != styleBefore);
		bool inTransition = (style_.transitionTimer < 1.0f);

		subtreeDirty_ = wasPending || pendingTransition || anyChildDirty || styleChanged || inTransition;
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

	void Layout::runFrame(float dt)
	{
		UIContext::get().beginFrame(dt);
		updateTree(dt);
		measure(Metrics::viewport.x, Metrics::viewport.y);
		arrange({0, 0, Metrics::viewport.x, Metrics::viewport.y});
	}

	void Layout::renderFrame()
	{
		auto *r = UIContext::get().renderer;
		if (!r)
			return;
		r->beginFrame();
		draw();
		r->endFrame();
	}
} // namespace ZenitUI

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