#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

	void Layout::update(float dt, bool ancestorBlocked, bool ancestorScrolling)
	{
		auto selfGuard = weak_from_this().lock();

		if (!config.visible)
		{
			cullRemovedChildren();
			internal_.subtreeDirty = 0;
			internal_.layoutDirty = 0;
			return;
		}

		bool wasPending = internal_.pendingTransition;
		ComputedStyle styleBefore = style_.currentStyle;

		initStyleIfNeeded();

		auto *ctx = context();
		uint32_t gen = ctx ? ctx->viewportGeneration : 0u;

		bool viewportChanged = (gen != lastViewportGen_);
		if (viewportChanged)
		{
			lastViewportGen_ = gen;
			internal_.pendingTransition = 1;
		}

		bool localAnimBlocks = anim_.tickImperative(*this, dt);
		bool blockSubtree = ancestorBlocked || localAnimBlocks;
		bool selfBlocked = blockSubtree || !config.enabled || (!config.interactive && config.blocksRaycast);

		const bool scrolling = ancestorScrolling || scroll_.scrolling;
		input_.updateFlags(*this, selfBlocked, scrolling);

		UIState prevState = style_.currentState;
		UIState nextState = input_.computeNextState(*this);
		bool stateChanged = (style_.currentState != nextState);

		if (viewportChanged)
		{
			style_.currentState = nextState;
			style_.resolvePendingTransition(*this, true);
			anim_.syncCss(*this);
		}
		else if (stateChanged)
		{
			style_.beginStateTransition(*this, nextState);
			anim_.syncCss(*this);
		}
		else if (internal_.pendingTransition)
		{
			style_.resolvePendingTransition(*this, false);
			anim_.syncCss(*this);
		}

		style_.tick(*this, dt);
		anim_.tickCss(*this, dt);
		style_.propagateInheritance(*this);

		scroll_.resetIfOverflowChanged(*this);

		{
			bool anyLocal = anim_.hasActiveLocal();
			if (internal_.hadLocalAnimationsLast && !anyLocal && onAnimationsFinished)
				onAnimationsFinished();
			internal_.hadLocalAnimationsLast = anyLocal;
		}

		{
			auto childrenSnapshot = children;
			for (auto &child : childrenSnapshot)
			{
				if (child && child->getParent().get() == this && !child->internal_.wantsRemoval)
					child->update(dt, blockSubtree, scrolling);
			}
		}

		scroll_.tickInput(*this);
		input_.handleKeyInput(*this);
		input_.fireCallbacks(*this, prevState, nextState, stateChanged);
		cullRemovedChildren();
		recomputeDirty(wasPending, styleBefore);

		if (config.enabled || config.updateWhenDisabled)
			onUpdate(dt);
	}

	void Layout::initStyleIfNeeded()
	{
		if (!internal_.styleInitialized)
		{
			internal_.styleInitialized = 1;
			style_.currentStyle = style_.targetStyle = style_.transitionStartStyle = style_.resolveFor(*this);
			style_.transitionDirtyMask_ = 0;
			internal_.pendingTransition = 0;
			anim_.syncCss(*this);
		}
	}
	void Layout::cullRemovedChildren()
	{
		bool removed = false;
		for (auto it = children.begin(); it != children.end();)
		{
			if ((*it)->internal_.wantsRemoval)
			{
				(*it)->detachContext();
				(*it)->parent.reset();
				it = children.erase(it);
				removed = true;
			}
			else
				++it;
		}
		if (removed)
			internal_.hadRemovedChildren = 1;
	}

	void Layout::recomputeDirty(bool wasPending, const ComputedStyle &styleBefore)
	{
		bool anyChildDirty = false;
		for (auto &c : children)
		{
			if (c->internal_.subtreeDirty)
			{
				anyChildDirty = true;
				break;
			}
		}

		bool styleChanged = (style_.currentStyle != styleBefore);
		bool inTransition = (style_.transitionTimer < 1.0f);

		internal_.subtreeDirty = wasPending || internal_.pendingTransition ||
								 anyChildDirty || styleChanged || inTransition ||
								 internal_.hadRemovedChildren || internal_.layoutDirty;
		internal_.hadRemovedChildren = 0;
		internal_.layoutDirty = 0;
	}

	std::vector<const ThemeRule *> Layout::getMatchingRules() const
	{
		std::vector<const ThemeRule *> out;
		const auto &t = theme();
		for (const auto &r : t.rules)
		{
			if (!r.part.empty())
				continue;
			if (ruleMatches(r, this, viewportSize()))
				out.push_back(&r);
		}
		return out;
	}

	static void collectFocusables(Layout *node, std::vector<Layout *> &out)
	{
		if (!node->getEnabled() || !node->isVisible())
			return;
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
		auto *ctx = context();
		if (!ctx) return;
		auto captured = ctx->pointerCapture.lock();
		if (!captured || !captured->getEnabled() || !captured->isVisible())
		{
			ctx->pointerCapture.reset();
			ctx->topmostConsumer = hitTest(ctx->pointer.pos, false, nullptr);
		}
		else
		{
			ctx->topmostConsumer = captured.get();
		}

		ctx->hoverTarget = ctx->topmostConsumer;

		bool anyPressed = false;
		bool anyReleased = false;
		bool anyDown = false;
		for (size_t i = 0; i < (size_t)MouseButton::Count; ++i)
		{
			const auto &b = ctx->pointer.buttons[i];
			if (b.pressed)
				anyPressed = true;
			if (b.released)
				anyReleased = true;
			if (b.down)
				anyDown = true;
		}

		if (anyPressed)
			ctx->pressTarget = ctx->hoverTarget;
		if (anyReleased)
			ctx->releaseTarget = ctx->hoverTarget;

		if (ctx->pointer[MouseButton::Left].pressed)
		{
			if (ctx->topmostConsumer && ctx->topmostConsumer->isFocusable())
			{
				ctx->requestFocus(ctx->topmostConsumer->shared_from_this());
				notifyFocusAncestors(ctx->focusedNode.lock().get());
			}
			else
			{
				ctx->releaseFocus();
			}
		}

		for (int k : ctx->inputEvents.keys)
		{
			if (k != Key::Tab)
				continue;

			Layout *scope = this;
			{
				auto focused = ctx->focusedNode.lock();
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

			auto cur = ctx->focusedNode.lock();
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
				next = ctx->modifiers.shift ? (idx - 1 + n) % n
										   : (idx + 1) % n;
			}
			ctx->requestFocus(focusables[next]->shared_from_this());
			notifyFocusAncestors(ctx->focusedNode.lock().get());
		}

		update(dt, false);

		if (anyReleased)
			ctx->pressTarget = nullptr;
		if (!anyDown && !ctx->pointerCapture.expired())
			ctx->pointerCapture.reset();
	}

	void Layout::runFrame(float dt)
	{
		if (auto *ctx = context()) ctx->runFrame(*this, dt);
	}

	void Layout::renderFrame()
	{
		if (auto *ctx = context()) ctx->renderFrame(*this);
	}

	void Layout::attachContext(UIContext *newCtx)
	{
		if (!newCtx)
		{
			detachContext();
			return;
		}
		if (ctx_ == newCtx)
		{
			if (!hasParent())
				newCtx->registerRoot(this);
			return;
		}
		if (ctx_)
			detachContext();

		ctx_ = newCtx;
		if (!hasParent())
			newCtx->registerRoot(this);

		style_.invalidateCache();
		internal_.pendingTransition = 1;
		lastViewportGen_ = 0;

		for (auto &c : children)
			if (c)
				c->attachContext(newCtx);
	}

	void Layout::detachContext()
	{
		if (!ctx_)
			return;

		UIContext *oldCtx = ctx_;
		oldCtx->unregisterRoot(this);

		if (oldCtx->pressTarget == this)
			oldCtx->pressTarget = nullptr;
		if (oldCtx->releaseTarget == this)
			oldCtx->releaseTarget = nullptr;
		if (oldCtx->topmostConsumer == this)
			oldCtx->topmostConsumer = nullptr;
		if (oldCtx->hoverTarget == this)
			oldCtx->hoverTarget = nullptr;
		if (oldCtx->focusedNode.lock().get() == this)
			oldCtx->focusedNode.reset();
		if (oldCtx->pointerCapture.lock().get() == this)
			oldCtx->pointerCapture.reset();

		releaseLayerCache();
		ctx_ = nullptr;

		for (auto &c : children)
			if (c)
				c->detachContext();
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

	UIContext::~UIContext()
	{
		detachAllRoots();
		detachBackend();
	}

	void UIContext::detachAllRoots()
	{
		while (!attachedRoots_.empty())
			attachedRoots_.back()->detachContext();
	}

	void UIContext::mount(Layout &root)
	{
		root.attachContext(this);
	}

	void UIContext::unmount(Layout &root)
	{
		if (root.context() == this)
			root.detachContext();
	}

	void UIContext::runFrame(Layout &root, float dt)
	{
		if (root.context() != this)
			root.attachContext(this);
		beginFrame(dt);
		root.updateTree(dt);
		root.measure(viewport.x, viewport.y);
		root.arrange({0.0f, 0.0f, viewport.x, viewport.y});
	}

	void UIContext::renderFrame(Layout &root)
	{
		if (root.context() != this)
			root.attachContext(this);
		if (!renderer)
			return;
		renderer->beginFrame();
		root.draw();
		endFrame();
		renderer->endFrame();
	}
}