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

	class Layout : public std::enable_shared_from_this<Layout>
	{
	public:
		Layout(LayoutType type = LayoutType::Stack) : type(type) {}
		virtual ~Layout()
		{
			if (auto *r = UIContext::get().renderer)
			{
				if (layerTarget_.valid())
					r->destroyTarget(layerTarget_);
				if (layerScratch_.valid())
					r->destroyTarget(layerScratch_);
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

		void addClass(const std::string &className)
		{
			styleClasses.push_back(className);
			pendingTransition = true;
		}
		void setId(const std::string &node_id)
		{
			nodeId = node_id;
		}

		void setStyleTag(const std::string &t) { styleTag = t; }
		const std::string &getStyleTag() const { return styleTag; }
		const std::vector<std::string> &getStyleClasses() const { return styleClasses; }

		void setSize(Value w, Value h)
		{
			style_.inlineBase.width = w;
			style_.inlineBase.height = h;
			pendingTransition = true;
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
			pendingTransition = true;
		}
		void removeFromParent() { wantsRemoval = true; }
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
		void setInteractive(bool interactive) { isInteractive = interactive; }
		void setBlocksRaycast(bool blocks) { blocksRaycast = blocks; }

		void setEnabled(bool e)
		{
			if (isEnabled == e)
				return;
			isEnabled = e;
			pendingTransition = true;

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
		bool getEnabled() const { return isEnabled; }

		void setChecked(bool c)
		{
			if (isChecked_ == c)
				return;
			isChecked_ = c;
			pendingTransition = true;
			// La propagazione al sottoalbero è gestita centralmente in update(),
			// tramite il confronto con lastSnapshot.
		}
		bool getChecked() const { return isChecked_; }

		bool isHoveredState() const { return isHovered; }
		bool isPressedState() const { return isPressed; }
		bool isFocusedState() const { return isFocused; }
		bool isDisabledState() const { return !isEnabled; }
		bool isCheckedState() const { return isChecked_; }

		void setUpdateWhenDisabled(bool v) { updateWhenDisabled_ = v; }
		bool getUpdateWhenDisabled() const { return updateWhenDisabled_; }

		void beginTransition()
		{
			pendingTransition = true;
			style_.transitionTimer = 0.0f;
			style_.transitionStartStyle = style_.currentStyle;
		}

		void setFocusable(bool f) { isFocusable_ = f; }
		bool isFocusable() const { return isFocusable_ && isEnabled; }
		bool getFocused() const { return isFocused; }

		void setFocusScope(bool s) { isFocusScope_ = s; }
		bool isFocusScope() const { return isFocusScope_; }

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

		float getScrollX() const { return scroll_.state.offset.x; }
		float getScrollY() const { return scroll_.state.offset.y; }
		float getMaxScrollX() const { return scroll_.state.maxScroll.x; }
		float getMaxScrollY() const { return scroll_.state.maxScroll.y; }

		void setPortal(bool p)
		{
			isPortal_ = p;
			pendingTransition = true;
		}
		bool isPortal() const { return isPortal_; }

		void setPassThrough(bool p) { passThrough_ = p; }
		bool getPassThrough() const { return passThrough_; }

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

		void setKeyboardActivates(bool v) { keyboardActivates_ = v; }
		bool getKeyboardActivates() const { return keyboardActivates_; }

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
			pendingTransition = true;
		}
		Style &getInlineBase()
		{
			pendingTransition = true;
			return style_.inlineBase;
		}
		Style &getInlineDefaults()
		{
			pendingTransition = true;
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

		Layout *hitTest(Vec2 p, bool ancestorBlocked = false);
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

		std::function<void()> onHoverEnter, onHoverExit, onPress, onRelease, onClick, onRightClick;
		std::vector<std::shared_ptr<Layout>> children;
		std::string nodeId;

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
		bool subtreeDirty_{true};
		float lastMeasureW_{-1.0f};
		float lastMeasureH_{-1.0f};
		ScrollController scroll_;
		bool styleInitialized{false};
		bool isInteractive{true}, blocksRaycast{false}, isHovered{false},
			wantsRemoval{false}, pendingTransition{true}, isEnabled{true};
		bool isFocused{false};
		bool isFocusable_{false};
		bool isFocusScope_{false};
		bool isPressed{false};
		bool isPortal_{false};
		bool passThrough_{false};
		bool keyboardActivates_{false};
		bool updateWhenDisabled_{false};
		bool pressedInChain_{false};
		bool isChecked_{false};

		std::string styleTag;
		std::vector<std::string> styleClasses;
		StyleResolver style_;
		InputController input_;

		TextureHandle bgTexture;
		NineSlice bgPatchInfo;
		TargetHandle layerTarget_;	 // render target per il path "filter"
		TargetHandle layerScratch_;	 // secondo target per blur separabile
		EffectHandle customEffect;	 // cache dell'handle risolto
		std::string effectName_;	 // nome stile, per detect cambio
		EffectHandle currentEffect_; // cache dell'handle risolto
		bool hasShader{false};

		AnimationPlayer anim_;

		// Buffer riusati in draw() per evitare allocazioni per-frame.
		std::vector<Layout *> drawNegZ_, drawNormal_, drawPosZ_;

		std::weak_ptr<Layout> parent;

		void markInheritanceDirty()
		{
			pendingTransition = true;
			subtreeDirty_ = true;
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

		if (ss.kind == SimpleSelector::Kind::Tag)
			return node->getStyleTag() == ss.name;
		if (ss.kind == SimpleSelector::Kind::Id)
			return node->nodeId == ss.name;

		for (const auto &c : node->getStyleClasses())
			if (c == ss.name)
				return true;
		return false;
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