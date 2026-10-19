#pragma once

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

namespace ZenitUI
{
	// =========================================================================
	//  Node flag groups
	//
	//  Layout carries three conceptually distinct groups of boolean state.
	//  Splitting them into bitfield structs has three effects:
	//
	//    1. Memory: 3 * uint32_t = 12 bytes, vs ~19-24 bytes for individual
	//       bool fields (padding included). Matters at 1000+ node counts.
	//
	//    2. Readability: `node.state.hovered` vs `node.isHovered`,
	//       `node.config.interactive` vs `node.isInteractive`. The prefix
	//       tells you at a glance which group a flag belongs to.
	//
	//    3. Extensibility: adding a new flag is a new bit in one of the
	//       three structs, not a new bool field in Layout. Future features
	//       (mouse buttons, additional state) can be added without
	//       growing Layout's field count.
	//
	//  Group meanings:
	//    - NodeState:    per-frame runtime state, computed by the framework
	//                    from input, animated by the style system.
	//    - NodeConfig:   static configuration, set by the constructor or
	//                    by user code via setters. Changes rarely.
	//    - NodeInternal: framework bookkeeping. Not part of the public
	//                    surface, not meaningful to user code.
	// =========================================================================

	struct NodeState
	{
		uint32_t hovered : 1;
		uint32_t pressed : 1;
		uint32_t focused : 1;
		uint32_t checked : 1;
	};

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

	class Layout : public std::enable_shared_from_this<Layout>
	{
	public:
		Layout(LayoutType type = LayoutType::Stack) : type(type)
		{
			// Non-zero defaults, matching the previous bool field
			// initializers: interactive, enabled, subtree dirty, and
			// pending transition all start true.
			config.interactive = 1;
			config.enabled = 1;
			internal_.subtreeDirty = 1;
			internal_.pendingTransition = 1;
		}

		virtual ~Layout()
		{
			// Clear any dangling pointer in the context. During a
			// recursive update, a node can be destroyed (via
			// cullRemovedChildren) while a later ancestor still needs
			// to consult pressTarget / releaseTarget / topmostConsumer
			// / hoverTarget. If the destroyed node was one of those,
			// the pointers would dangle. Resetting here makes the
			// ancestor's check a no-op instead of a crash.
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

		template <typename T = Layout>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast<T>(shared_from_this());
		}

		friend struct StyleResolver;
		friend struct AnimationPlayer;
		friend struct InputController;
		friend struct ScrollController;

		// Adds a CSS class. No-op if the class is already present,
		// so calling it twice does not create duplicates.
		void addClass(const std::string &className)
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return;
			styleClasses.push_back(className);
			internal_.pendingTransition = 1;
		}

		// Removes every occurrence of the class. No-op if the class
		// isn't present.
		void removeClass(const std::string &className)
		{
			auto it = std::remove(styleClasses.begin(),
								  styleClasses.end(),
								  className);
			if (it == styleClasses.end())
				return;
			styleClasses.erase(it, styleClasses.end());
			internal_.pendingTransition = 1;
		}

		// True if the class is currently set on this node.
		bool hasClass(const std::string &className) const
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return true;
			return false;
		}

		// True if the node's id matches. The id is set via setId() (or the
		// `id` fluent helper, or `#name` in ZMarkup). Returns false if the
		// node has no id, or if the given id is empty.
		bool hasId(const std::string &targetId) const
		{
			return !targetId.empty() && nodeId == targetId;
		}

		// Removes every class. No-op if there were none.
		void clearStyleClasses()
		{
			if (styleClasses.empty())
				return;
			styleClasses.clear();
			internal_.pendingTransition = 1;
		}

		void setId(const std::string &node_id)
		{
			nodeId = node_id;
		}

		void setRef(const std::string &r)
		{
			ref = r;
		}

		void setStyleTag(const std::string &t) { styleTag = t; }
		const std::string &getStyleTag() const { return styleTag; }
		const std::vector<std::string> &getStyleClasses() const { return styleClasses; }

		void setSize(Value w, Value h)
		{
			style_.inlineBase.width = w;
			style_.inlineBase.height = h;
			internal_.pendingTransition = 1;
		}

		// --- API fluent (ritornano shared_from_this, NON usare nel costruttore) ---
		std::shared_ptr<Layout> cls(const std::string &className)
		{
			addClass(className);
			return shared_from_this();
		}
		std::shared_ptr<Layout> id(const std::string &node_id)
		{
			setId(node_id);
			return shared_from_this();
		}
		std::shared_ptr<Layout> size(Value w, Value h)
		{
			setSize(w, h);
			return shared_from_this();
		}
		std::shared_ptr<Layout> with(std::shared_ptr<Layout> child)
		{
			addChild(std::move(child));
			return shared_from_this();
		}

		void addChild(std::shared_ptr<Layout> child)
		{
			if (!child)
				return;

			// Rete di sicurezza: se il chiamante è un costruttore o uno stato
			// dove shared_from_this() non è ancora valido, avvisa e ignora.
			// Usa X::create() e metti questa logica in onBuild().
			if (weak_from_this().expired())
			{
				logWarn("Layout", "", 0, 0,
						"addChild chiamato su un nodo non gestito da shared_ptr. "
						"Usa X::create() e metti questa logica in onBuild().");
				return;
			}

			child->parent = weak_from_this();
			children.push_back(std::move(child));
			internal_.pendingTransition = 1;
		}

		// Returns the node to which ZMarkup should attach children. Default
		// returns `this`. Composite widgets (Modal) override it to point at
		// their internal content slot, so that markup children end up
		// inside the widget's styled content area rather than as siblings
		// of the widget's internal nodes.
		//
		// Regular C++ code should not need this: use addChild() directly on
		// the widget and it will route to the same place the widget considers
		// its content area.
		virtual Layout *contentTarget() { return this; }

		void removeFromParent() { internal_.wantsRemoval = 1; }
		// Removes every child in one shot. Unlike `removeFromParent` on
		// each child, this is immediate: the `children` vector is cleared
		// right away, and each removed child's `parent` pointer is reset
		// to null.
		//
		// Use this to rebuild the content of a container (a list, a
		// settings panel) without having to allocate a new node.
		//
		// Note: any shared_ptr the caller still holds to a removed child
		// keeps it alive; only the tree relationship is broken.
		void removeAllChildren()
		{
			for (auto &c : children)
				if (c)
					c->parent.reset();
			children.clear();
			internal_.pendingTransition = 1;
		}
		std::shared_ptr<Layout> getParent() const { return parent.lock(); }
		bool hasParent() const { return !parent.expired(); }

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

		// --- Stato / stile ---
		void setInteractive(bool interactive) { config.interactive = interactive ? 1 : 0; }
		void setBlocksRaycast(bool blocks) { config.blocksRaycast = blocks ? 1 : 0; }

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

		void notifyDescendantFocused(Layout *descendant) { onDescendantFocused(descendant); }

		bool isStackingContext() const
		{
			return style_.currentStyle.position != Position::Static && !style_.currentStyle.zIndex.isAuto;
		}
		int getZIndex() const
		{
			return style_.currentStyle.zIndex.isAuto ? 0 : style_.currentStyle.zIndex.value;
		}

		// --- Scroll (attivo se currentStyle.overflow != Visible) ---
		bool isScrollContainer() const
		{
			return style_.currentStyle.overflowX != Overflow::Visible || style_.currentStyle.overflowY != Overflow::Visible;
		}

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

		// Scrolls the nearest scrollable ancestor so this node becomes
		// visible. Does nothing if the node has no scrollable ancestor,
		// or if the node is already fully visible.
		//
		// `margin` reserves that many pixels of padding around the node
		// inside the container's viewport. Use it to avoid the node
		// being flush against the container's edge.
		//
		// Returns true if the scroll offset was changed.
		bool scrollIntoView(float margin = 0.0f);

		// Same as above, but for a sub-region of the node. Useful when
		// the "interesting" part of the node is smaller than its full
		// rect (a label inside a large padding, a specific line of text).
		bool scrollIntoView(const Rect &subRect, float margin = 0.0f);

		float getScrollX() const { return scroll_.state.offset.x; }
		float getScrollY() const { return scroll_.state.offset.y; }
		float getMaxScrollX() const { return scroll_.state.maxScroll.x; }
		float getMaxScrollY() const { return scroll_.state.maxScroll.y; }

		void setPortal(bool p)
		{
			config.portal = p ? 1 : 0;
			internal_.pendingTransition = 1;
		}
		bool isPortal() const { return config.portal != 0; }

		void setPassThrough(bool p) { config.passThrough = p ? 1 : 0; }
		bool getPassThrough() const { return config.passThrough != 0; }

		// Pointer capture: finché un nodo ha il capture attivo, ogni evento
		// pointer viene indirizzato a lui, indipendentemente dalla posizione
		// del cursore. Il capture viene rilasciato automaticamente quando il
		// tasto del mouse viene sollevato.
		void capturePointer()
		{
			UIContext::get().pointerCapture = shared_from_this();
		}
		void releasePointer()
		{
			auto &ctx = UIContext::get();
			auto sp = ctx.pointerCapture.lock();
			if (sp.get() == this)
				ctx.pointerCapture.reset();
		}
		bool hasPointerCapture() const
		{
			return UIContext::get().pointerCapture.lock().get() == this;
		}

		void setKeyboardActivates(bool v) { config.keyboardActivates = v ? 1 : 0; }
		bool getKeyboardActivates() const { return config.keyboardActivates != 0; }

		inline FontHandle resolveFont(const ComputedStyle &style, FontHandle fallback = {})
		{
			if (style.font.empty() || !UIContext::get().assets)
				return fallback;
			FontHandle h = UIContext::get().assets->getFont(style.font);
			return h.id != 0 ? h : fallback;
		}

		struct ResolvedBgTexture
		{
			TextureHandle tex;
			NineSlice slice;
			bool valid() const { return tex.valid(); }
		};

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

		void setInlineBase(const Style &s)
		{
			style_.inlineBase.overlay(s);
			internal_.pendingTransition = 1;
		}
		Style &getInlineBase()
		{
			internal_.pendingTransition = 1;
			return style_.inlineBase;
		}
		Style &getInlineDefaults()
		{
			internal_.pendingTransition = 1;
			return style_.inlineDefaults;
		}

		// --- Animazioni imperative ---
		void addAnimation(const std::string &name, std::shared_ptr<UIAnimation> anim)
		{
			anim_.imperative[name] = {anim, 0.0f, 0.0f, false, false};
		}
		void playAnimation(const std::string &name, bool playReverse = false)
		{
			anim_.play(*this, name, playReverse);
		}
		bool hasActiveAnimations() const
		{
			return anim_.hasActive(*this);
		}

		// --- Animazioni CSS ---
		void addCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					return;
			anim_.css.push_back({name, 0.0f, false});
		}
		void stopCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					a.finished = true;
		}

		// ============================================================
		//  CICLO DI VITA
		//  Chiamati dal runtime (main loop + updateTree). Non override.
		// ============================================================
		virtual Vec2 measure(float parent_w, float parent_h);
		virtual void arrange(Rect space);
		virtual void update(float dt, bool ancestorBlocked = false, bool scrolling = false);
		virtual void draw(float parentOpacity = 1.0f);

		Layout *hitTest(Vec2 p,
						bool ancestorBlocked = false,
						const Rect *clipLocal = nullptr);
		void updateTree(float dt);

		// Ciclo di logica: beginFrame + updateTree + measure + arrange.
		// Chiamato dal root una volta per frame, prima di BeginDrawing.
		void runFrame(float dt);

		// Ciclo di disegno: renderer->beginFrame + draw + renderer->endFrame.
		// Chiamato dal root una volta per frame, tra BeginDrawing e EndDrawing.
		void renderFrame();

		Rect getRect() const { return rect; }
		Vec2 getMeasuredSize() const { return measuredSize; }
		const ComputedStyle &getStyle() const { return style_.currentStyle; }
		std::vector<const ThemeRule *> getMatchingRules() const;

		// ---- Callbacks (Left button, legacy names) ----
		std::function<void()> onHoverEnter, onHoverExit, onPress, onRelease, onClick;

		// ---- Callbacks (Right button, legacy semantic) ----
		//
		// Fires on right-button press when the node is hovered. Not a
		// full click (press + release on the same target): the right
		// button is meant for one-shot events like context menus, and
		// the historical semantic is "press-on-hover".
		std::function<void()> onRightClick;

		// ---- Callbacks (unified, Left + Middle) ----
		//
		// Same click model as onPress/onRelease/onClick (press and
		// release on the same target), but parameterized by MouseButton
		// so that Middle is covered without a new pair of callback
		// fields. Currently fires for Left and Middle. Right is NOT
		// covered here — its semantic is different (see onRightClick
		// above). Adding a new button to the MouseButton enum extends
		// onButton* automatically, as long as the button follows the
		// press+release model.
		std::function<void(MouseButton)> onButtonPress;
		std::function<void(MouseButton)> onButtonRelease;
		std::function<void(MouseButton)> onButtonClick;

		std::function<void()> onAnimationsFinished;
		std::vector<std::shared_ptr<Layout>> children;

		// Style id: matches #id in ZMarkup, participates in CSS
		// specificity as an id selector.
		std::string nodeId;

		// Logic ref: matches @ref in ZMarkup, does NOT participate in
		// CSS. Used by UINode::ref / UINode::refAll for C++ lookup.
		// Can be shared by multiple nodes.
		std::string ref;

		void setBackgroundTexture(TextureHandle tex, NineSlice np = {0, 0, 0, 0})
		{
			bgTexture = tex;
			bgPatchInfo = np;
		}
		void setShader(EffectHandle shader)
		{
			customEffect = shader;
			hasShader = true;
		}

	protected:
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
		InputController input_;

		TextureHandle bgTexture;
		NineSlice bgPatchInfo;
		TargetHandle layerTarget_;	  // ping-pong A
		TargetHandle layerScratch_;	  // ping-pong B
		TargetHandle layerSeparable_; // intermediate for Separable filters
		EffectHandle customEffect;	  // cache dell'handle risolto
		std::string effectName_;	  // nome stile, per detect cambio
		EffectHandle currentEffect_;  // cache dell'handle risolto
		bool hasShader{false};

		AnimationPlayer anim_;

		// Buffer riusati in draw() per evitare allocazioni per-frame.
		std::vector<Layout *> drawNegZ_, drawNormal_, drawPosZ_;

		std::weak_ptr<Layout> parent;

		void markInheritanceDirty()
		{
			internal_.pendingTransition = 1;
			internal_.subtreeDirty = 1;
			for (auto &c : children)
				c->markInheritanceDirty();
		}

		Style partStyle(const std::string &partName)
		{
			return style_.partFor(*this, partName);
		}

		// Ritorna la size intrinseca del widget (es. dimensione del testo).
		// Chiamato in fase di measure, prima di arrangiare i figli.
		// Default: {0,0}. Override solo se il widget ha una dimensione "naturale".
		virtual Vec2 computeIntrinsicSize(float /*availW*/, float /*availH*/) { return {0, 0}; }

		// Chiamato dopo che `rect` è stato settato (subito dopo arrangeInto).
		// Usalo per ricalcolare cose che dipendono dalla size FINALE, come il
		// wrap del testo. Non deve modificare `measuredSize`.
		virtual void onLayout() {}

		// Per-frame update. Chiamato una volta per frame, dopo tutto il
		// processing di stato (hover/pressed/focus). Usalo per input custom,
		// animazioni locali, logica di drag, ecc.
		virtual void onUpdate(float /*dt*/) {}

		// Chiamato quando isEnabled cambia. Usalo per pulire cache di stato o
		// chiudere popup/menu. Non serve per la logica per-frame: per quella
		// c'è onUpdate, che di default non gira quando il nodo è disabilitato.
		virtual void onEnabledChanged(bool /*nowEnabled*/) {}

		// Resolves the effect to apply to this node's draw call,
		// considering both possible sources.
		//
		// Priority:
		//   1. `effect:` style property (resolved against assets).
		//   2. `setShader()` handle set from C++.
		//   3. Invalid handle (no effect).
		//
		// Subclasses that override draw() — currently only CanvasLayout —
		// should call this instead of touching `currentEffect_` or
		// `customEffect` directly, so that the priority stays consistent
		// with the rest of the framework.
		EffectHandle resolveActiveEffect()
		{
			resolveEffectIfNeeded(style_.currentStyle);
			if (currentEffect_.valid())
				return currentEffect_;
			if (hasShader)
				return customEffect;
			return {};
		}

		// Disegno del contenuto del widget.
		// Chiamato DOPO renderChrome, PRIMA dei figli.
		virtual void renderContent(float /*op*/, const ComputedStyle & /*style*/) {}

		// Sfondo + bordo. Default: fill del background + stroke del border dallo
		// Style. Override solo per texture, nine-slice o "vestiti" particolari.
		virtual void renderChrome(float op, const ComputedStyle &style)
		{
			auto r = UIContext::get().renderer;
			if (!r)
				return;
			if (rect.width <= 0 || rect.height <= 0)
				return;

			float maxRadius = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolveH(maxRadius * 2.0f, maxRadius * 2.0f), 0.0f, maxRadius);

			// 0) Box-shadow (dietro a tutto)
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
					// Approssimazione: 5 strati concentrici con alpha decrescente.
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

			// 1) Colore di sfondo (sotto la texture)
			Color bg = style.background.withAlpha(op);
			if (bg.a > 0)
			{
				if (rPx > 0.0f && maxRadius > 0.0f)
					r->fillRoundedRect(rect, rPx, bg);
				else
					r->fillRect(rect, bg);
			}

			// 2) Texture: CSS se presente, altrimenti il fallback del widget
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
					// stretch semplice: src = dimensioni intere, dst = rect
					Rect src{0, 0, (float)resolved.tex.width, (float)resolved.tex.height};
					r->drawTexture(resolved.tex, src, rect, tint);
				}
			}

			// 3) Bordo
			Color bc = style.borderColor.withAlpha(op);
			float bw = style.borderWidth.resolveH(maxRadius * 2.0f, maxRadius * 2.0f);
			if (bc.a > 0 && bw > 0.0f)
			{
				if (rPx > 0.0f && maxRadius > 0.0f)
					r->strokeRoundedRect(rect, rPx, bw, bc);
				else
					r->strokeRect(rect, bw, bc);
			}
		}

		// Chiamato quando un discendente ottiene il focus.
		virtual void onDescendantFocused(Layout * /*descendant*/) {}

		// ============================================================
		//  LAYOUT ENGINE (interno)
		// ============================================================

		// Layout dei figli. Non override: se hai bisogno di un layout custom
		// (scroll, portal, ...) override `arrange` e chiama `arrangeInto`
		// con lo spazio che vuoi.
		void arrangeInto(Rect space);

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
		void initStyleIfNeeded();
		void cullRemovedChildren();
		void recomputeDirty(bool wasPending, const ComputedStyle &styleBefore);
		void resolveEffectIfNeeded(const ComputedStyle &style);
		void drawWithStyle(const ComputedStyle &renderStyle, float parentOpacity);
		void drawInline(const ComputedStyle &renderStyle, float globalOp);
		void drawLayer(const ComputedStyle &renderStyle, float globalOp);
		void drawChildren(float globalOp);

		// ------------------------------------------------------------
		//  Arrange: fasi (estratte da arrangeInto() per leggibilità)
		// ------------------------------------------------------------
		void arrangeAbsoluteAndPortals(const Rect &inner);
		void arrangeAbsolute(Layout *c, const ComputedStyle &cst, const Rect &in);
		void arrangeVerticalFlow(const Rect &inner, float gapY);
		void arrangeHorizontalFlow(const Rect &inner, float gapX);
		void arrangeStackFlow(const Rect &inner);

		void translateSubtree(float dx, float dy);

		static float clampSafe(float v, float lo, float hi);
	};

	// CRTP helper: aggiunge la fluent API tipizzata su una Base qualsiasi.
	// Uso:
	//   class Text   : public TLayout<Text>              // Base = Layout
	//   class Panel  : public TLayout<Panel>             // Base = Layout
	//   class Button : public TLayout<Button, Panel>     // Base = Panel
	template <typename Derived, typename Base = Layout>
	class TLayout : public Base
	{
	public:
		template <typename... Args>
		TLayout(Args &&...args) : Base(std::forward<Args>(args)...) {}

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
		virtual void onBuild() {}
	};

	inline bool nodeMatchesSimple(const Layout *node, const SimpleSelector &ss)
	{
		if (!node)
			return false;

		// Le pseudo-classi sono richieste a livello di compound intero:
		// se sono presenti, il nodo deve soddisfarle tutte.
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

		// Helper: verifica che un singolo componente (tipo, nome) matchi
		// il nodo.
		auto matchesComponent = [node](SimpleSelector::Kind kind,
									   const std::string &name) -> bool
		{
			if (kind == SimpleSelector::Kind::Tag)
				return node->getStyleTag() == name;
			if (kind == SimpleSelector::Kind::Id)
				return node->nodeId == name;
			// Class
			for (const auto &c : node->getStyleClasses())
				if (c == name)
					return true;
			return false;
		};

		// Componente principale.
		if (!matchesComponent(ss.kind, ss.name))
			return false;

		// Componenti aggiuntivi del compound selector: tutti devono matchare
		// lo stesso nodo.
		for (const auto &[kind, name] : ss.extras)
		{
			if (!matchesComponent(kind, name))
				return false;
		}

		return true;
	}

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