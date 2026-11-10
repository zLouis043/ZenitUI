#pragma once

/**
 * \file Layout.hpp
 * \brief The node class: tree structure, layout pass, style resolution,
 *        input dispatch, render orchestration, and scroll handling.
 *
 * `Layout` is the single node type in ZenitUI. There is no separate "widget"
 * base class: every visual element is a `Layout` subclass (either directly,
 * via `TLayout<Derived>`, or by composition). The node owns or co-owns:
 *
 *  - **Tree structure**: parent, children, dirty flags, removal requests.
 *  - **Style subsystem** (as `style_`): resolution, transitions, caching.
 *  - **Animation subsystem** (as `anim_`): imperative and CSS animations.
 *  - **Input subsystem** (as `input_`): hover/press/focus dispatch.
 *  - **Scroll subsystem** (as `scroll_`): offset, inertia, scrollbars.
 *
 * The public API mirrors the four-pass frame cycle, each function called
 * from `Layout::runFrame`:
 *
 *   1. `update` — input state, style transitions, animations, culling.
 *   2. `measure` — compute intrinsic sizes bottom-up.
 *   3. `arrange` — assign screen rects top-down.
 *   4. `draw` — render with style overlays and clip/effect stacks.
 *
 * Subclasses hook in via the virtuals (`computeIntrinsicSize`,
 * `renderContent`, `renderChrome`, `onLayout`, `onUpdate`, …) rather than by
 * overriding the four main passes, except in rare cases (`VirtualView`,
 * `Canvas`, `Popup`, `Tooltip` override `arrange` or `draw`).
 *
 * \note `enable_shared_from_this` is required: `addChild` checks
 *       `weak_from_this()` and warns if a node is stack-allocated. Always
 *       create nodes through `TLayout<Derived>::create` or
 *       `std::make_shared<Layout>`.
 */

#include "Common.hpp"

#include "Style.hpp"
#include "Theme.hpp"
#include "UIContext.hpp"
#include "AnimPrimitives.hpp"
#include "Logger.hpp"
#include "ScrollState.hpp"
#include "StyleResolver.hpp"
#include "AnimationPlayer.hpp"
#include "InputController.hpp"
#include "ScrollController.hpp"
#include "UIEnums.hpp"
#include "Media.hpp"
#include "SmallVector.hpp"

namespace ZenitUI
{

	/**
	 * \struct NodeState
	 * \brief Per-frame interaction flags derived from pointer / keyboard state.
	 *
	 * Packed as bitfields to keep the node struct small. Written by
	 * `InputController::updateFlags`, read by `StyleResolver` and by
	 * components that need to render state-dependent visuals.
	 */
	struct NodeState
	{
		uint32_t hovered : 1;
		uint32_t pressed : 1;
		uint32_t focused : 1;
		uint32_t checked : 1;
	};

	/**
	 * \struct NodeConfig
	 * \brief Static configuration bits: interaction behaviour, render behaviour.
	 *
	 * Set by subclass constructors (or by inline setters) and read on every
	 * frame. Changing any of these fields is safe at runtime, but should be
	 * done through the setter methods rather than by writing the bitfields
	 * directly, so that bookkeeping (dirty flags, focus cleanup) stays
	 * consistent.
	 */
	struct NodeConfig
	{
		uint32_t interactive : 1;
		uint32_t blocksRaycast : 1;
		uint32_t enabled : 1;
		uint32_t focusable : 1;
		uint32_t focusScope : 1;
		uint32_t keyboardActivates : 1;
		uint32_t updateWhenDisabled : 1;
		uint32_t portal : 1;
		uint32_t passThrough : 1;
	};

	/**
	 * \struct NodeInternal
	 * \brief Transient bookkeeping bits used by the layout / style / culling passes.
	 *
	 * Not part of the public configuration surface; subclass constructors and
	 * the framework mutate these directly.
	 */
	struct NodeInternal
	{
		uint32_t subtreeDirty : 1;
		uint32_t styleInitialized : 1;
		uint32_t pendingTransition : 1;
		uint32_t hadLocalAnimationsLast : 1;
		uint32_t wantsRemoval : 1;
		uint32_t pressedInChain : 1;
		uint32_t hadRemovedChildren : 1;
	};

	/**
	 * \class Layout
	 * \brief The base node type: tree, style, layout, input, scroll, render.
	 *
	 * See the file-level documentation for a description of the four main
	 * passes and the composition points for subclasses.
	 *
	 * \note `Layout` is not intended to be instantiated directly in user code;
	 *       use `std::make_shared<Layout>` for layout containers
	 *       (`VStack`, `HStack`, `Stack`) or `TLayout<Derived>::create` for
	 *       custom components.
	 */
	class Layout : public std::enable_shared_from_this<Layout>
	{
	public:
		/**
		 * \brief Construct a node with the given layout strategy.
		 *
		 * Defaults: `interactive = 1`, `enabled = 1`, `subtreeDirty = 1`,
		 * `pendingTransition = 1`. Style is lazily initialized on first
		 * `update` / `measure`.
		 */
		Layout(LayoutType type = LayoutType::Stack) : type(type)
		{
			config.interactive = 1;
			config.enabled = 1;
			internal_.subtreeDirty = 1;
			internal_.pendingTransition = 1;
		}

		/**
		 * \brief Destroy the node and clean up framework-side references.
		 *
		 * Clears the global pointers (`pressTarget`, `releaseTarget`,
		 * `topmostConsumer`, `hoverTarget`) if they pointed to this node, and
		 * releases any render targets owned by a layer draw.
		 *
		 * Does **not** detach children: the parent owns children via
		 * `shared_ptr`, so a child's lifetime is tied to the parent unless
		 * reparented.
		 */
		virtual ~Layout()
		{
			auto &ctx = UIContext::get();
			if (ctx.pressTarget == this)
				ctx.pressTarget = nullptr;
			if (ctx.releaseTarget == this)
				ctx.releaseTarget = nullptr;
			if (ctx.topmostConsumer == this)
				ctx.topmostConsumer = nullptr;
			if (ctx.hoverTarget == this)
				ctx.hoverTarget = nullptr;

			if (auto *r = ctx.renderer)
			{
				if (layerTarget_.valid())
					r->destroyTarget(layerTarget_);
				if (layerScratch_.valid())
					r->destroyTarget(layerScratch_);
				if (layerSeparable_.valid())
					r->destroyTarget(layerSeparable_);
			}
		}

		/**
		 * \brief `static_pointer_cast` helper returning `shared_ptr<T>`.
		 *
		 * Convenience for `std::static_pointer_cast<T>(shared_from_this())`.
		 * No runtime type check; use with care if the dynamic type is not known.
		 */
		template <typename T = Layout>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast<T>(shared_from_this());
		}

		// Subsystems need write access to private members for performance.
		friend struct StyleResolver;
		friend struct AnimationPlayer;
		friend struct InputController;
		friend struct ScrollController;

		// ---- Class / id / tag management ----

		/**
		 * \brief Add a class name to the node's style class list.
		 *
		 * No-op if the class is already present. Invalidates the style cache
		 * and propagates the change to descendants that might match rules
		 * with this class in their selector chain (via
		 * `invalidateIfRelevant`).
		 */
		void addClass(const std::string &className)
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return;
			styleClasses.push_back(className);

			style_.invalidateCache();
			internal_.pendingTransition = 1;

			for (auto &c : children)
				c->invalidateIfRelevant(SimpleSelector::Kind::Class, className);
		}

		/**
		 * \brief Remove a class name; no-op if absent.
		 *
		 * Invalidates the style cache and propagates the change to relevant
		 * descendants, same as `addClass`.
		 */
		void removeClass(const std::string &className)
		{
			auto it = std::remove(styleClasses.begin(),
								  styleClasses.end(),
								  className);
			if (it == styleClasses.end())
				return;
			styleClasses.erase(it, styleClasses.end());

			style_.invalidateCache();
			internal_.pendingTransition = 1;

			for (auto &c : children)
				c->invalidateIfRelevant(SimpleSelector::Kind::Class, className);
		}

		/** \brief Remove every class name. Equivalent to calling `removeClass` for each. */
		void clearStyleClasses()
		{
			if (styleClasses.empty())
				return;

			auto removed = styleClasses;
			styleClasses.clear();

			style_.invalidateCache();
			internal_.pendingTransition = 1;

			for (const auto &name : removed)
				for (auto &c : children)
					c->invalidateIfRelevant(SimpleSelector::Kind::Class, name);
		}

		/**
		 * \brief Set the node's style tag.
		 *
		 * The tag is matched against `SimpleSelector::Kind::Tag` in the
		 * stylesheet. Changing the tag invalidates both the old and new
		 * tag rules for every descendant.
		 */
		void setStyleTag(const std::string &t)
		{
			const std::string oldTag = styleTag;
			styleTag = t;

			style_.invalidateCache();
			internal_.pendingTransition = 1;

			if (oldTag != t)
			{
				for (auto &c : children)
				{
					c->invalidateIfRelevant(SimpleSelector::Kind::Tag, oldTag);
					c->invalidateIfRelevant(SimpleSelector::Kind::Tag, t);
				}
			}
		}

		/** \brief `true` if the node has the given class. */
		bool hasClass(const std::string &className) const
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return true;
			return false;
		}

		/** \brief `true` if the node's `nodeId` equals \p targetId. */
		bool hasId(const std::string &targetId) const
		{
			return !targetId.empty() && nodeId == targetId;
		}

		/** \brief Set the node's `#id` (used in selectors and by `UINode::find`). */
		void setId(const std::string &node_id)
		{
			nodeId = node_id;
		}

		/** \brief Set the node's `@ref` (used by `UINode::ref` / `refAll`). */
		void setRef(const std::string &r)
		{
			ref = r;
		}

		const std::string &getStyleTag() const { return styleTag; }
		const std::vector<std::string> &getStyleClasses() const { return styleClasses; }

		/** \brief Convenience: set explicit `width` and `height`. */
		void setSize(Value w, Value h)
		{
			style_.inlineBase.width = w;
			style_.inlineBase.height = h;
			internal_.pendingTransition = 1;
		}

		// ---- Fluent builder shortcuts ----

		/** \brief Fluent `addClass`; returns `shared_from_this()`. */
		std::shared_ptr<Layout> cls(const std::string &className)
		{
			addClass(className);
			return shared_from_this();
		}
		/** \brief Fluent `setId`; returns `shared_from_this()`. */
		std::shared_ptr<Layout> id(const std::string &node_id)
		{
			setId(node_id);
			return shared_from_this();
		}
		/** \brief Fluent `setSize`; returns `shared_from_this()`. */
		std::shared_ptr<Layout> size(Value w, Value h)
		{
			setSize(w, h);
			return shared_from_this();
		}
		/** \brief Fluent `addChild`; returns `shared_from_this()`. */
		std::shared_ptr<Layout> with(std::shared_ptr<Layout> child)
		{
			addChild(std::move(child));
			return shared_from_this();
		}

		/**
		 * \brief Append \p child to this node's child list.
		 *
		 * Sets `child->parent` to a weak reference of this node and marks
		 * the style as pending. Logs a warning and returns early if the node
		 * is not owned by a `shared_ptr` (i.e. `weak_from_this()` is expired).
		 */
		void addChild(std::shared_ptr<Layout> child)
		{
			if (!child)
				return;

			if (weak_from_this().expired())
			{
				logWarn("Layout", "", 0, 0,
						"addChild called on a node not handled with shared_ptr."
						"Use X::create() and put this logic in onBuild().");
				return;
			}

			child->parent = weak_from_this();
			children.push_back(std::move(child));
			internal_.pendingTransition = 1;
		}

		/**
		 * \brief Return the node that receives children when built from markup.
		 *
		 * Components that wrap their content in a sub-layout (e.g. `Modal`
		 * keeps its children inside `content`) override this to point at the
		 * wrapper. Default: `this`.
		 */
		virtual Layout *contentTarget() { return this; }

		/**
		 * \brief Request removal at the next culling pass.
		 *
		 * Does not detach immediately: `Layout::cullRemovedChildren` on the
		 * parent removes the node and its subtree on the next frame. This
		 * defers destruction to a safe point in the frame cycle.
		 */
		void removeFromParent() { internal_.wantsRemoval = 1; }

		/**
		 * \brief Detach every child immediately, clearing their parent pointers.
		 *
		 * Unlike `removeFromParent`, this is synchronous and does not defer.
		 * Overridden by components that cache child pointers (e.g.
		 * `VirtualView` clears its `materialized` list first).
		 */
		virtual void removeAllChildren()
		{
			for (auto &c : children)
				if (c)
					c->parent.reset();
			children.clear();
			internal_.pendingTransition = 1;
		}
		std::shared_ptr<Layout> getParent() const { return parent.lock(); }
		bool hasParent() const { return !parent.expired(); }

		/**
		 * \brief `true` if this node is an ancestor of \p other.
		 *
		 * Walks up from `other->getParent()`; `this` is an ancestor only if
		 * found strictly above `other`. Self is not an ancestor of self.
		 */
		bool isAncestorOf(const Layout *other) const
		{
			if (!other)
				return false;
			auto p = other->getParent();
			while (p)
			{
				if (p.get() == this)
					return true;
				p = p->getParent();
			}
			return false;
		}

		// ---- Configuration setters ----

		void setInteractive(bool interactive) { config.interactive = interactive ? 1 : 0; }
		void setBlocksRaycast(bool blocks) { config.blocksRaycast = blocks ? 1 : 0; }

		/**
		 * \brief Enable or disable the node.
		 *
		 * Disabling a node releases focus and pointer capture, fires
		 * `onEnabledChanged(false)`, and forces `UIState::Disabled`. Input to
		 * the subtree is suppressed by `Layout::update` via the
		 * `ancestorBlocked` chain.
		 */
		void setEnabled(bool e)
		{
			if ((config.enabled != 0) == e)
				return;
			config.enabled = e ? 1 : 0;
			internal_.pendingTransition = 1;

			if (!e)
			{
				auto &ctx = UIContext::get();
				if (ctx.focusedNode.lock().get() == this)
					ctx.focusedNode.reset();
				if (ctx.pointerCapture.lock().get() == this)
					ctx.pointerCapture.reset();
			}

			onEnabledChanged(e);
		}
		bool getEnabled() const { return config.enabled != 0; }

		/**
		 * \brief Set the `:checked` pseudo-class state.
		 *
		 * Used by `Checkbox`, `Toggle`, and any component that wants a
		 * `:checked` style rule to fire. Marks the style pending.
		 */
		void setChecked(bool c)
		{
			if ((state.checked != 0) == c)
				return;
			state.checked = c ? 1 : 0;
			internal_.pendingTransition = 1;
		}
		bool getChecked() const { return state.checked != 0; }

		bool isHoveredState() const { return state.hovered != 0; }
		bool isPressedState() const { return state.pressed != 0; }
		bool isFocusedState() const { return state.focused != 0; }
		bool isDisabledState() const { return config.enabled == 0; }
		bool isCheckedState() const { return state.checked != 0; }

		void setUpdateWhenDisabled(bool v) { config.updateWhenDisabled = v ? 1 : 0; }
		bool getUpdateWhenDisabled() const { return config.updateWhenDisabled != 0; }

		/**
		 * \brief Restart the current style transition from the current value.
		 *
		 * Resets `transitionTimer` to 0 and re-snapshots the start style.
		 * Equivalent to `StyleResolver::beginStateTransition` for the current
		 * state, but without changing the state.
		 */
		void beginTransition()
		{
			internal_.pendingTransition = 1;
			style_.transitionTimer = 0.0f;
			style_.transitionStartStyle = style_.currentStyle;
		}

		void setFocusable(bool f) { config.focusable = f ? 1 : 0; }
		bool isFocusable() const { return config.focusable != 0 && config.enabled != 0; }
		bool getFocused() const { return state.focused != 0; }

		void setFocusScope(bool s) { config.focusScope = s ? 1 : 0; }
		bool isFocusScope() const { return config.focusScope != 0; }

		/** \brief Notify the node that one of its descendants gained focus. */
		void notifyDescendantFocused(Layout *descendant) { onDescendantFocused(descendant); }

		/** \brief `true` if the node has a non-static `position` and a non-auto `zIndex`. */
		bool isStackingContext() const
		{
			return style_.currentStyle.position != Position::Static && !style_.currentStyle.zIndex.isAuto;
		}
		/** \brief Effective `zIndex`; returns 0 for the "auto" case. */
		int getZIndex() const
		{
			return style_.currentStyle.zIndex.isAuto ? 0 : style_.currentStyle.zIndex.value;
		}

		/** \brief `true` if either overflow axis is non-visible. */
		bool isScrollContainer() const
		{
			return style_.currentStyle.overflowX != Overflow::Visible || style_.currentStyle.overflowY != Overflow::Visible;
		}

		// ---- Scroll API ----

		/**
		 * \brief Set the scroll offset, clamped to the valid range.
		 *
		 * No-op if the node is not a scroll container. The change is applied
		 * to the subtree via `ScrollController::applyOffsetDelta` on the next
		 * arrange pass.
		 */
		void scrollTo(float x, float y)
		{
			if (!isScrollContainer())
				return;
			scroll_.state.offset.x = std::max(0.0f, x);
			scroll_.state.offset.y = std::max(0.0f, y);
			scroll_.state.clamp();
		}
		void scrollToX(float x) { scrollTo(x, scroll_.state.offset.y); }
		void scrollToY(float y) { scrollTo(scroll_.state.offset.x, y); }
		void scrollToTop()
		{
			if (isScrollContainer())
				scrollToY(0.0f);
		}
		void scrollToBottom()
		{
			if (isScrollContainer())
				scrollToY(scroll_.state.maxScroll.y);
		}
		void scrollToLeft()
		{
			if (isScrollContainer())
				scrollToX(0.0f);
		}
		void scrollToRight()
		{
			if (isScrollContainer())
				scrollToX(scroll_.state.maxScroll.x);
		}

		/**
		 * \brief Scroll the nearest scrolling ancestor so that this node's rect
		 *        becomes visible, respecting \p margin.
		 * \return `true` if the offset changed.
		 * \sa scrollIntoView(const Rect&, float)
		 */
		bool scrollIntoView(float margin = 0.0f);

		/**
		 * \brief Scroll the nearest scrolling ancestor so that \p subRect
		 *        becomes visible, respecting \p margin.
		 *
		 * Only one ancestor is adjusted (the first scrolling ancestor). For
		 * nested scroll views, callers must iterate manually or call this on
		 * each level.
		 *
		 * \return `true` if the offset changed.
		 */
		bool scrollIntoView(const Rect &subRect, float margin = 0.0f);

		float getScrollX() const { return scroll_.state.offset.x; }
		float getScrollY() const { return scroll_.state.offset.y; }
		float getMaxScrollX() const { return scroll_.state.maxScroll.x; }
		float getMaxScrollY() const { return scroll_.state.maxScroll.y; }

		// ---- Portal / capture / keyboard ----

		/**
		 * \brief Enable or disable portal rendering for this node.
		 *
		 * Portals draw at the root and hit-test above everything else. Used by
		 * `Dropdown`, `Popup`, `Tooltip` and any component that must escape
		 * its ancestors' clip rect.
		 */
		void setPortal(bool p)
		{
			config.portal = p ? 1 : 0;
			internal_.pendingTransition = 1;
		}
		bool isPortal() const { return config.portal != 0; }

		void setPassThrough(bool p) { config.passThrough = p ? 1 : 0; }
		bool getPassThrough() const { return config.passThrough != 0; }

		/**
		 * \brief Capture the pointer: all subsequent input events go to this node.
		 *
		 * Capture is released automatically on button-up (`Layout::updateTree`)
		 * or when the node is disabled / destroyed. Only one node can hold
		 * capture at a time; capturing overrides any previous owner.
		 */
		void capturePointer()
		{
			UIContext::get().pointerCapture = shared_from_this();
		}
		/** \brief Release the pointer capture if this node owns it. */
		void releasePointer()
		{
			auto &ctx = UIContext::get();
			auto sp = ctx.pointerCapture.lock();
			if (sp.get() == this)
				ctx.pointerCapture.reset();
		}
		/** \brief `true` if this node currently holds the pointer capture. */
		bool hasPointerCapture() const
		{
			return UIContext::get().pointerCapture.lock().get() == this;
		}

		void setKeyboardActivates(bool v) { config.keyboardActivates = v ? 1 : 0; }
		bool getKeyboardActivates() const { return config.keyboardActivates != 0; }

		LayoutType getLayoutType() const { return type; }

		// ---- Asset convenience helpers ----

		/**
		 * \brief Resolve a `FontHandle` from the node's style font stack.
		 * \param style    Computed style carrying the `font` field.
		 * \param fallback Returned if the stack is empty or resolution fails.
		 */
		inline FontHandle resolveFont(const ComputedStyle &style, FontHandle fallback = {})
		{
			if (style.font.empty() || !UIContext::get().assets)
				return fallback;
			FontHandle h = UIContext::get().assets->getFont(style.font);
			return h.id != 0 ? h : fallback;
		}

		/** \struct ResolvedBgTexture
		 *  \brief Result of `resolveBgTexture`: a texture plus its nine-slice insets. */
		struct ResolvedBgTexture
		{
			TextureHandle tex;
			NineSlice slice;
			bool valid() const { return tex.valid(); }
		};

		/**
		 * \brief Resolve the background texture from the style, falling back
		 *        to explicit parameters if the style has no `background-texture`.
		 */
		inline ResolvedBgTexture resolveBgTexture(
			const ComputedStyle &style,
			TextureHandle fallbackTex = {},
			NineSlice fallbackSlice = {0, 0, 0, 0})
		{
			if (!style.backgroundTexture.name.empty())
			{
				auto *a = UIContext::get().assets;
				if (a)
				{
					TextureHandle h = a->getTexture(style.backgroundTexture.name);
					if (h.valid())
					{
						const auto &tr = style.backgroundTexture;
						return {h, NineSlice{tr.left, tr.top, tr.right, tr.bottom}};
					}
				}
			}
			return {fallbackTex, fallbackSlice};
		}

		// ---- Inline style setters ----

		/** \brief Overlay \p s onto the node's inline style (highest priority). */
		void setInlineBase(const Style &s)
		{
			style_.inlineBase.overlay(s);
			internal_.pendingTransition = 1;
		}
		/**
		 * \brief Direct mutable access to the inline style.
		 *
		 * Marks the style as pending on every call, so mutating the returned
		 * reference and then dropping it is safe (the next frame will re-resolve).
		 */
		Style &getInlineBase()
		{
			internal_.pendingTransition = 1;
			return style_.inlineBase;
		}
		/**
		 * \brief Direct mutable access to the inline defaults (lowest-priority inline).
		 *
		 * Applied before theme rules; useful for overriding a component's
		 * hardcoded defaults without overriding user-supplied CSS.
		 */
		Style &getInlineDefaults()
		{
			internal_.pendingTransition = 1;
			return style_.inlineDefaults;
		}

		// ---- Animation shortcuts ----

		/**
		 * \brief Register an imperative animation under \p name.
		 *
		 * Does not start it; call `playAnimation` after registering. The
		 * animation object is shared, so it can be played simultaneously on
		 * multiple nodes.
		 */
		void addAnimation(const std::string &name, std::shared_ptr<UIAnimation> anim)
		{
			anim_.imperative[name] = {anim, 0.0f, 0.0f, false, false};
		}
		/**
		 * \brief Start (or restart) the imperative animation named \p name.
		 * \param playReverse When `true`, plays from end to start.
		 */
		void playAnimation(const std::string &name, bool playReverse = false)
		{
			anim_.play(*this, name, playReverse);
		}
		/** \brief `true` if this node or any descendant is currently animating. */
		bool hasActiveAnimations() const
		{
			return anim_.hasActive(*this);
		}

		/**
		 * \brief Manually start a CSS animation by name.
		 *
		 * Usually unnecessary: CSS animations are driven by the `animations`
		 * field of the resolved style and synced automatically. Provided for
		 * debug and for imperative overrides.
		 */
		void addCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					return;
			anim_.css.push_back({name, 0.0f, false});
		}
		/** \brief Mark the named CSS animation as finished. */
		void stopCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					a.finished = true;
		}

		// ---- Virtual pass entry points ----

		/**
		 * \brief Compute the intrinsic size given available space.
		 *
		 * Called bottom-up during `measure`. Subclasses override
		 * `computeIntrinsicSize` (which receives the available space *after*
		 * padding / border) rather than this method directly.
		 *
		 * \param parent_w Available width from the parent, in pixels.
		 * \param parent_h Available height from the parent, in pixels.
		 * \return The node's own measured size, including padding and border.
		 */
		virtual Vec2 measure(float parent_w, float parent_h);

		/**
		 * \brief Assign a screen rect to this node and lay out its children.
		 *
		 * For scroll containers, this triggers a full re-arrange when needed,
		 * then applies the scroll offset delta. Otherwise, it delegates to
		 * `arrangeInto` (the flex/stack flow dispatch).
		 */
		virtual void arrange(Rect space);

		/**
		 * \brief Per-frame update: input, style, animation, then recurse into children.
		 *
		 * This is where the frame begins for a subtree. The recursion passes
		 * down two propagated flags:
		 *
		 *  - `ancestorBlocked`: any ancestor disabled, animating a blocking
		 *    animation, or non-interactive with `blocksRaycast` on.
		 *  - `ancestorScrolling`: any ancestor is scrolling; children freeze
		 *    their hover/press state so a drag does not flicker buttons.
		 *
		 * After the recursion, the node fires its own input callbacks, its
		 * own `onUpdate`, and culling of any child that set `wantsRemoval`.
		 */
		virtual void update(float dt, bool ancestorBlocked = false, bool scrolling = false);

		/**
		 * \brief Draw the node and its children.
		 *
		 * Applies style overlays (CSS animations, if any), then dispatches to
		 * `drawInline` or `drawLayer` depending on whether the node has
		 * filters that need a layer pass. After drawing, calls
		 * `scroll_.drawScrollbar`.
		 *
		 * Root nodes also flush `framePortals`: portals that were registered
		 * during the tree walk are drawn at the root, above everything else.
		 */
		virtual void draw(float parentOpacity = 1.0f);

		/**
		 * \brief Hit-test a point against this subtree.
		 *
		 * Traversal order matches paint order reversed: front-most first.
		 * Within a node, stacking-context children are checked before
		 * normal-flow children; negative-Z children last. The self-rect is
		 * checked after all children, so a child always wins over its parent.
		 *
		 * \param p               Point in screen space.
		 * \param ancestorBlocked `true` if an ancestor already rejected the point.
		 * \param clipLocal       Optional clip rect in *this node's* local space.
		 * \return The frontmost interactive (or `blocksRaycast`) node under
		 *         \p p, or `nullptr`.
		 */
		Layout *hitTest(Vec2 p,
						bool ancestorBlocked = false,
						const Rect *clipLocal = nullptr);

		/**
		 * \brief Root-of-tree update: refresh pointer state and recurse.
		 *
		 * Entry point for the per-frame update. Handles hit-testing, focus
		 * acquisition/release, Tab-cycle focus navigation, and the
		 * `pressTarget`/`releaseTarget` bookkeeping that click dispatch depends on.
		 */
		void updateTree(float dt);

		/** \brief Run one full frame: update, measure, arrange. */
		void runFrame(float dt);

		/** \brief Render the current state: beginFrame, draw, endFrame. */
		void renderFrame();

		// ---- Accessors ----

		Rect getRect() const { return rect; }
		Vec2 getMeasuredSize() const { return measuredSize; }
		const ComputedStyle &getStyle() const { return style_.currentStyle; }
		std::vector<const ThemeRule *> getMatchingRules() const;

		// ---- Public callback hooks ----

		std::function<void()> onHoverEnter, onHoverExit, onPress, onRelease, onClick;

		/** \brief Right-click hook; fires on right-button release over the node. */
		std::function<void()> onRightClick;

		std::function<void(MouseButton)> onButtonPress;
		std::function<void(MouseButton)> onButtonRelease;
		std::function<void(MouseButton)> onButtonClick;

		/**
		 * \brief Called when the node had active local animations and now has none.
		 *
		 * Local means "on this node", not descendants. A parent can report
		 * finished while a child is still animating.
		 */
		std::function<void()> onAnimationsFinished;

		/** \brief Children of this node. Managed by `addChild` / `cullRemovedChildren`. */
		std::vector<std::shared_ptr<Layout>> children;

		/** \brief `#id` for selector matching and `UINode::find`. */
		std::string nodeId;

		/** \brief `@ref` for `UINode::ref` / `refAll` lookup. */
		std::string ref;

		/**
		 * \brief Override the background texture with a direct handle.
		 *
		 * Takes precedence over the style's `background-texture` field. Used
		 * by `ImageContainer` and components that set the texture at runtime.
		 */
		void setBackgroundTexture(TextureHandle tex, NineSlice np = {0, 0, 0, 0})
		{
			bgTexture = tex;
			bgPatchInfo = np;
		}
		/**
		 * \brief Set a custom shader effect applied to the node's content.
		 *
		 * Composed on top of any `effect` from the style. Both are pushed
		 * during `drawInline` / `drawLayer` when the renderer supports
		 * `Feature::Effects`.
		 */
		void setShader(EffectHandle shader)
		{
			customEffect = shader;
			hasShader = true;
		}

	protected:
		// ---- Internal state ----

		LayoutType type;
		uint32_t lastViewportGen_{0};
		Rect rect{0, 0, 0, 0};
		Vec2 measuredSize{0, 0};
		float lastMeasureW_{-1.0f};
		float lastMeasureH_{-1.0f};
		ScrollController scroll_;

		NodeState state{};
		NodeConfig config{};
		NodeInternal internal_{};
		std::string styleTag;
		std::vector<std::string> styleClasses;
		StyleResolver style_;
		SmallVector<std::pair<SimpleSelector::Kind, std::string_view>, 8> relevantSymbols_;
		uint32_t relevantThemeVersion_{~0u};
		InputController input_;

		TextureHandle bgTexture;
		NineSlice bgPatchInfo;
		TargetHandle layerTarget_;
		TargetHandle layerScratch_;
		TargetHandle layerSeparable_;
		EffectHandle customEffect;
		std::string effectName_;
		EffectHandle currentEffect_;
		bool hasShader{false};

		AnimationPlayer anim_;

		std::vector<Layout *> drawNegZ_, drawNormal_, drawPosZ_;

		std::weak_ptr<Layout> parent;

		/**
		 * \brief Mark this node and every descendant as needing style re-resolution.
		 *
		 * Called on ancestor state changes (theme, ancestor hover) that affect
		 * inherited properties.
		 */
		void markInheritanceDirty()
		{
			internal_.pendingTransition = 1;
			internal_.subtreeDirty = 1;
			for (auto &c : children)
				c->markInheritanceDirty();
		}

		/**
		 * \brief Propagate a class/tag change to descendants that may match it.
		 *
		 * Only descendants whose `relevantSymbols_` list mentions the changed
		 * symbol are marked. If the theme version changed since the last
		 * refresh, every descendant is invalidated conservatively.
		 */
		void invalidateIfRelevant(SimpleSelector::Kind kind, std::string_view name)
		{
			if (relevantThemeVersion_ != Theme::get().version)
			{
				invalidateStyleCacheRecursive();
				return;
			}

			bool relevant = false;
			for (auto &[k, n] : relevantSymbols_)
			{
				if (k == kind && n == name)
				{
					relevant = true;
					break;
				}
			}
			if (relevant)
			{
				style_.invalidateCache();
				internal_.pendingTransition = 1;
			}
			for (auto &c : children)
				c->invalidateIfRelevant(kind, name);
		}

		/** \brief Mark the whole subtree as needing style re-resolution. */
		void invalidateStyleCacheRecursive()
		{
			style_.invalidateCache();
			internal_.pendingTransition = 1;
			for (auto &c : children)
				c->invalidateStyleCacheRecursive();
		}

		/**
		 * \brief Resolve the style for pseudo-element \p partName.
		 *
		 * Delegates to `StyleResolver::partFor`. Parts are independent of the
		 * main style and have their own transition state.
		 */
		Style partStyle(const std::string &partName)
		{
			return style_.partFor(*this, partName);
		}

		// ---- Subclass hooks ----

		/**
		 * \brief Compute the intrinsic size, excluding padding and border.
		 *
		 * Called from `measure` with the available *inner* size. Subclasses
		 * override to report content-dependent sizing (text extents, image
		 * dimensions, etc.). Default: `{0, 0}` (size to zero content).
		 */
		virtual Vec2 computeIntrinsicSize(float /*availW*/, float /*availH*/) { return {0, 0}; }

		/** \brief Called after `arrange` assigns the node's rect. */
		virtual void onLayout() {}

		/** \brief Called every frame from `update`, after input dispatch. */
		virtual void onUpdate(float /*dt*/) {}

		/** \brief Called from `setEnabled` after the state change. */
		virtual void onEnabledChanged(bool /*nowEnabled*/) {}

		/**
		 * \brief Resolve the currently active effect handle.
		 *
		 * Prefers the style's `effect` over the `setShader` override. Returns
		 * an invalid handle if neither is set.
		 */
		EffectHandle resolveActiveEffect()
		{
			resolveEffectIfNeeded(style_.currentStyle);
			if (currentEffect_.valid())
				return currentEffect_;
			if (hasShader)
				return customEffect;
			return {};
		}

		// ---- Render hooks ----

		/**
		 * \brief Draw the node's content (text, images, custom geometry).
		 *
		 * Called after `renderChrome` (background, border, shadow) and before
		 * children are drawn. The clip rect is already pushed if overflow
		 * requires it.
		 */
		virtual void renderContent(float /*op*/, const ComputedStyle & /*style*/) {}

		/**
		 * \brief Draw the node's background, border, and shadow.
		 *
		 * The default implementation reads `background`, `backgroundTexture`,
		 * `borderColor` / `borderWidth`, `boxShadow`, and `radius` from the
		 * style and emits the corresponding draw calls. Subclasses with custom
		 * chrome can override, but should usually keep the default and add
		 * their rendering to `renderContent`.
		 */
		virtual void renderChrome(float op, const ComputedStyle &style)
		{
			auto r = UIContext::get().renderer;
			if (!r)
				return;
			if (rect.width <= 0 || rect.height <= 0)
				return;

			float maxRadius = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolveH(maxRadius * 2.0f, maxRadius * 2.0f), 0.0f, maxRadius);

			if (style.boxShadow.enabled)
			{
				const auto &sh = style.boxShadow;
				float sx = sh.x.resolveSelfH(rect.width, rect.height);
				float sy = sh.y.resolveSelfV(rect.width, rect.height);
				float blur = sh.blur.resolveSelfH(rect.width, rect.height);

				Rect shadowRect = {
					rect.x + sx,
					rect.y + sy,
					rect.width,
					rect.height};

				Color baseColor = sh.color.withAlpha(op);

				if (blur <= 0.5f)
				{
					r->fillRoundedRect(shadowRect, rPx, baseColor);
				}
				else
				{
					constexpr int LAYERS = 5;
					for (int i = 0; i < LAYERS; ++i)
					{
						float expand = blur * (float)(i + 1) / (float)LAYERS;
						float alpha = (1.0f - (float)i / (float)LAYERS) * 0.35f;
						Rect r2 = {
							shadowRect.x - expand,
							shadowRect.y - expand,
							shadowRect.width + expand * 2.0f,
							shadowRect.height + expand * 2.0f};
						float rPx2 = std::clamp(rPx + expand, 0.0f, expand + maxRadius);
						r->fillRoundedRect(r2, rPx2, sh.color.withAlpha(op * alpha));
					}
				}
			}

			Color bg = style.background.withAlpha(op);
			if (bg.a > 0)
			{
				if (rPx > 0.0f && maxRadius > 0.0f)
					r->fillRoundedRect(rect, rPx, bg);
				else
					r->fillRect(rect, bg);
			}

			auto resolved = resolveBgTexture(style, bgTexture, bgPatchInfo);
			if (resolved.valid())
			{
				Color tint = style.tint.withAlpha(op);
				if (resolved.slice.left || resolved.slice.top ||
					resolved.slice.right || resolved.slice.bottom)
				{
					r->drawNineSlice(resolved.tex, resolved.slice, rect, tint);
				}
				else
				{
					Rect src{0, 0, (float)resolved.tex.width, (float)resolved.tex.height};
					r->drawTexture(resolved.tex, src, rect, tint);
				}
			}

			Color bc = style.borderColor.withAlpha(op);
			const auto &bws = style.borderWidth;
			float bwRef = maxRadius * 2.0f;
			float bwl = bws.left.resolveH(bwRef, bwRef);
			float bwr = bws.right.resolveH(bwRef, bwRef);
			float bwt = bws.top.resolveH(bwRef, bwRef);
			float bwb = bws.bottom.resolveH(bwRef, bwRef);

			const bool anyBorder = (bwl > 0.0f || bwr > 0.0f || bwt > 0.0f || bwb > 0.0f);
			const bool uniform = (bwl == bwr && bwr == bwb && bwb == bwt);

			if (bc.a > 0 && anyBorder)
			{
				if (uniform && rPx > 0.0f && maxRadius > 0.0f)
					r->strokeRoundedRect(rect, rPx, bwl, bc);
				else if (uniform)
					r->strokeRect(rect, bwl, bc);
				else
					r->fillBorder(rect, EdgeInsets{bwt, bwr, bwb, bwl}, bc);
			}
		}

		/** \brief Called on ancestors when a descendant gains focus. */
		virtual void onDescendantFocused(Layout * /*descendant*/) {}

		// ---- Layout internals ----

		/**
		 * \brief Dispatch to the layout-type-specific arrange helper.
		 *
		 * Computes padding / border, then calls `arrangeAbsoluteAndPortals`
		 * for out-of-flow children and then one of:
		 *  - `arrangeVerticalFlow` for `Vertical`
		 *  - `arrangeHorizontalFlow` (or `arrangeWrapFlow` if `wrap`) for `Horizontal`
		 *  - `arrangeStackFlow` for `Stack`
		 */
		void arrangeInto(Rect space);

		/**
		 * \brief Compute the current transform from the style's
		 *        `translateX`/`translateY`/`rotation`/`scale`.
		 */
		Transform2D currentTransform(const ComputedStyle &style) const
		{
			Transform2D tr;
			tr.pivot = rect.center();
			tr.translate = {
				style.translateX.resolveSelfH(rect.width, rect.height),
				style.translateY.resolveSelfV(rect.width, rect.height)};
			tr.rotationDeg = style.rotation;
			tr.scale = style.scale;
			return tr;
		}

	private:
		// ---- Private internals ----

		/** \brief Lazily initialize `style_` on the first `update` / `measure`. */
		void initStyleIfNeeded();

		/** \brief Remove children that requested removal via `removeFromParent`. */
		void cullRemovedChildren();

		/** \brief Recompute `internal_.subtreeDirty` from child and style diffs. */
		void recomputeDirty(bool wasPending, const ComputedStyle &styleBefore);

		/** \brief Re-resolve `currentEffect_` from the style's `effect` field if changed. */
		void resolveEffectIfNeeded(const ComputedStyle &style);

		/** \brief Top-level draw: chooses layer vs inline based on filters. */
		void drawWithStyle(const ComputedStyle &renderStyle, float parentOpacity);

		/** \brief Draw without an off-screen layer (no filters or unsupported). */
		void drawInline(const ComputedStyle &renderStyle, float globalOp);

		/**
		 * \brief Draw into an off-screen target, then apply filters and composite.
		 *
		 * Layer size is the axis-aligned bounding box of the transformed rect,
		 * clipped to the current clip rect. Targets are cached and reused
		 * across frames; they are resized (destroyed + recreated) only when
		 * the size changes. Layer dimensions above 4096 fall back to inline
		 * drawing to avoid GPU limits.
		 */
		void drawLayer(const ComputedStyle &renderStyle, float globalOp);

		/** \brief Draw children in stacking order (negative Z, normal, positive Z). */
		void drawChildren(float globalOp);

		// ---- Flow helpers ----

		/** \brief Lay out absolute children and portals before the main flow. */
		void arrangeAbsoluteAndPortals(const Rect &inner);

		/**
		 * \brief Lay out one absolute child.
		 *
		 * Resolution rules (CSS-like):
		 *  - `left` + `right` with `width: auto` → stretch horizontally.
		 *  - `left` or `right` alone → position from that side.
		 *  - Neither → left-anchored at `inner.x + margin-left`.
		 *  - Same symmetrically for `top` / `bottom` / vertical.
		 */
		void arrangeAbsolute(Layout *c, const ComputedStyle &cst, const Rect &in);

		/**
		 * \brief Lay out children in a vertical flex flow.
		 *
		 * Distributes `freeSpace` per `grow` / `shrink`, applies
		 * `justify-content` distribution, and honors per-child `alignH`.
		 * Children with `position: absolute` or `isPortal()` are skipped
		 * (laid out in `arrangeAbsoluteAndPortals`).
		 */
		void arrangeVerticalFlow(const Rect &inner, float gapY);

		/** \brief Symmetric to `arrangeVerticalFlow`, along the horizontal axis. */
		void arrangeHorizontalFlow(const Rect &inner, float gapX);

		/**
		 * \brief Lay out children with `flex-wrap: wrap`.
		 *
		 * Items are grouped into lines that fit `inner.width`; each line is
		 * then distributed by `justify-content` and aligned vertically per
		 * child's `alignV`.
		 */
		void arrangeWrapFlow(const Rect &inner, float gapX, float gapY);

		/** \brief Lay out children in a Stack: all centered/anchored in the same box. */
		void arrangeStackFlow(const Rect &inner);

		/**
		 * \brief Translate the subtree (children and their descendants) by `(dx, dy)`.
		 *
		 * Used by the scroll fast path. Portals are skipped: their position
		 * is independent of the scroll container.
		 */
		void translateSubtree(float dx, float dy);

		/**
		 * \brief `std::clamp` that handles the case where `hi < lo` by returning `hi`.
		 *
		 * The standard `std::clamp` has undefined behavior if `lo > hi`, which
		 * happens when the available space is negative (a child with
		 * `min-width > parent-width`). This variant returns `hi` in that case,
		 * which lets the child overflow the parent rather than dropping to 0.
		 */
		static float clampSafe(float v, float lo, float hi);
	};

	/**
	 * \class TLayout
	 * \tparam Derived Concrete component type.
	 * \tparam Base    Base to inherit from; defaults to `Layout`. Used by
	 *                 components like `Popup` that inherit from `Panel`.
	 *
	 * \brief CRTP base for components that want fluent builders and a static
	 *        `create()` factory.
	 *
	 * Provides:
	 *  - `static create(Args...)`: constructs a `shared_ptr<Derived>` and calls
	 *    `onBuild()` once the object is fully constructed.
	 *  - Fluent `cls`, `id`, `withRef`, `size`, `passThrough`, `with` that
	 *    return `shared_ptr<Derived>` for chaining.
	 *  - `onBuild()` virtual, called exactly once at the end of `create()`.
	 *
	 * Subclass constructors should do only initialization; structural work
	 * (creating child widgets, wiring callbacks that need `shared_from_this`)
	 * belongs in `onBuild`, because `shared_from_this` is valid only after
	 * `create()` returns from `make_shared`.
	 */
	template <typename Derived, typename Base = Layout>
	class TLayout : public Base
	{
	public:
		template <typename... Args>
		TLayout(Args &&...args) : Base(std::forward<Args>(args)...) {}

		/**
		 * \brief Construct a `shared_ptr<Derived>` and call `onBuild()`.
		 *
		 * The two-step construct-then-build is required because
		 * `shared_from_this()` is only valid once `make_shared` has returned.
		 */
		template <typename... Args>
		static std::shared_ptr<Derived> create(Args &&...args)
		{
			auto p = std::make_shared<Derived>(std::forward<Args>(args)...);
			static_cast<TLayout<Derived, Base> *>(p.get())->onBuild();
			return p;
		}
		std::shared_ptr<Derived> cls(const std::string &name)
		{
			this->addClass(name);
			return self();
		}
		std::shared_ptr<Derived> id(const std::string &node_id)
		{
			this->setId(node_id);
			return self();
		}
		std::shared_ptr<Derived> withRef(const std::string &r)
		{
			this->setRef(r);
			return self();
		}
		std::shared_ptr<Derived> size(Value w, Value h)
		{
			this->setSize(w, h);
			return self();
		}
		std::shared_ptr<Derived> passThrough(bool p = true)
		{
			this->setPassThrough(p);
			return self();
		}
		std::shared_ptr<Derived> with(std::shared_ptr<Layout> c)
		{
			this->addChild(std::move(c));
			return self();
		}
		std::shared_ptr<Derived> self()
		{
			return std::static_pointer_cast<Derived>(this->shared_from_this());
		}

	protected:
		/** \brief Hook: called once after `create()` finishes constructing the object. */
		virtual void onBuild() {}
	};

	/**
	 * \brief `true` if \p node matches the primary selector and all of its extras.
	 *
	 * Checks the pseudo-class requirements first (cheap), then tag / class / id
	 * components. A compound selector like `.a.b#c` requires all components to
	 * match; a state pseudo-class like `:hover` requires the corresponding flag.
	 */
	inline bool nodeMatchesSimple(const Layout *node, const SimpleSelector &ss)
	{
		if (!node)
			return false;

		if (ss.requireHover && !node->isHoveredState())
			return false;
		if (ss.requirePressed && !node->isPressedState())
			return false;
		if (ss.requireFocus && !node->isFocusedState())
			return false;
		if (ss.requireDisabled && !node->isDisabledState())
			return false;
		if (ss.requireChecked && !node->isCheckedState())
			return false;

		auto matchesComponent = [node](SimpleSelector::Kind kind,
									   const std::string &name) -> bool
		{
			if (kind == SimpleSelector::Kind::Tag)
				return node->getStyleTag() == name;
			if (kind == SimpleSelector::Kind::Id)
				return node->nodeId == name;
			for (const auto &c : node->getStyleClasses())
				if (c == name)
					return true;
			return false;
		};

		if (!matchesComponent(ss.kind, ss.name))
			return false;

		for (const auto &[kind, name] : ss.extras)
		{
			if (!matchesComponent(kind, name))
				return false;
		}

		return true;
	}

	/**
	 * \brief `true` if \p node matches the full rule selector chain.
	 *
	 * The chain is evaluated right-to-left (innermost selector first, matching
	 * the node itself), then each ancestor is checked against the previous
	 * component in the chain. Any node in between that does not match is
	 * skipped (matches the CSS descendant combinator `A B`). Only descendant
	 * combinators are supported; child (`>`) and sibling (`+`, `~`) combinators
	 * are not.
	 *
	 * \note Media queries on the rule are evaluated against the current
	 *       `Metrics::viewport` at call time, so a viewport change naturally
	 *       re-evaluates all rules on the next frame.
	 */
	inline bool ruleMatches(const ThemeRule &r, const Layout *node)
	{
		if (r.chain.empty() || !node)
			return false;
		if (r.media.has_value() && !evaluateMedia(*r.media, Metrics::viewport))
			return false;
		if (!nodeMatchesSimple(node, r.chain.back()))
			return false;

		int i = (int)r.chain.size() - 2;
		const Layout *cur = node->getParent().get();
		while (i >= 0 && cur)
		{
			if (nodeMatchesSimple(cur, r.chain[i]))
				--i;
			cur = cur->getParent().get();
		}
		return i < 0;
	}
}