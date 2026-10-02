#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "Theme.hpp"
#include "UIContext.hpp"
#include "UIAnimations.hpp"
#include "Logger.hpp"


namespace ZenitUI {

	enum class LayoutType { Stack, Vertical, Horizontal };
	enum class UIState { Idle, Hover, Pressed, Disabled };

	

	class Layout : public std::enable_shared_from_this<Layout> {
	public:
		Layout(LayoutType type = LayoutType::Stack) : type(type) {}
		virtual ~Layout() = default;

		template <typename T = Layout> std::shared_ptr<T> as() {
			return std::static_pointer_cast<T>(shared_from_this());
		}

		void addClass(const std::string& className) {
			styleClasses.push_back(className);
			pendingTransition = true;
		}
		void setId(const std::string& node_id) {
			nodeId = node_id;
		}

		void setStyleTag(const std::string& t) { styleTag = t; }
		const std::string& getStyleTag() const { return styleTag; }
		const std::vector<std::string>& getStyleClasses() const { return styleClasses; }

		void setSize(Value w, Value h) {
			inlineBase.width = w;
			inlineBase.height = h;
			pendingTransition = true;
		}

		// --- API fluent (ritornano shared_from_this, NON usare nel costruttore) ---
		std::shared_ptr<Layout> cls(const std::string& className) {
			addClass(className);
			return shared_from_this();
		}
		std::shared_ptr<Layout> id(const std::string& node_id) {
			setId(node_id);
			return shared_from_this();
		}
		std::shared_ptr<Layout> size(Value w, Value h) {
			setSize(w, h);
			return shared_from_this();
		}
		std::shared_ptr<Layout> with(std::shared_ptr<Layout> child) {
			addChild(std::move(child));
			return shared_from_this();
		}

				void addChild(std::shared_ptr<Layout> child) {
			if (!child) return;

			// Rete di sicurezza: se il chiamante è un costruttore o uno stato
			// dove shared_from_this() non è ancora valido, avvisa e ignora.
			// Usa X::create() per costruire widget correttamente.
			if (weak_from_this().expired()) {
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

		// --- Stato / stile ---
		void setInteractive(bool interactive) { isInteractive = interactive; }
		void setBlocksRaycast(bool blocks) { blocksRaycast = blocks; }

		void setEnabled(bool e) {
			if (isEnabled != e) { isEnabled = e; pendingTransition = true; }
		}
		bool getEnabled() const { return isEnabled; }

		void setFocusable(bool f) { isFocusable_ = f; }
		bool isFocusable() const { return isFocusable_ && isEnabled; }
		bool getFocused() const { return isFocused; }

		void setFocusScope(bool s) { isFocusScope_ = s; }
		bool isFocusScope() const { return isFocusScope_; }

		void notifyDescendantFocused(Layout* descendant) { onDescendantFocused(descendant); }

		bool isStackingContext() const {
			return currentStyle.position != Position::Static && !currentStyle.zIndex.isAuto;
		}
		int getZIndex() const {
			return currentStyle.zIndex.isAuto ? 0 : currentStyle.zIndex.value;
		}

		void setPortal(bool p) { isPortal_ = p; pendingTransition = true; }
		bool isPortal() const { return isPortal_; }

		void setPassThrough(bool p) { passThrough_ = p; }
		bool getPassThrough() const { return passThrough_; }

		void setKeyboardActivates(bool v) { keyboardActivates_ = v; }
    	bool getKeyboardActivates() const { return keyboardActivates_; }

		void setInlineBase(const Style& s) { inlineBase.overlay(s); pendingTransition = true; }
		Style& getInlineBase() { pendingTransition = true; return inlineBase; }

		// --- Animazioni imperative ---
		void addAnimation(const std::string& name, std::shared_ptr<UIAnimation> anim) {
			activeAnimations[name] = { anim, 0.0f, 0.0f, false, false };
		}
		void playAnimation(const std::string& name, bool playReverse = false);
		bool hasActiveAnimations() const;

		// --- Animazioni CSS ---
		void addCssAnimation(const std::string& name) {
			for (auto& a : activeCssAnimations) if (a.name == name) return;
			activeCssAnimations.push_back({ name, 0.0f, false });
		}
		void stopCssAnimation(const std::string& name) {
			for (auto& a : activeCssAnimations) if (a.name == name) a.finished = true;
		}

		// --- Ciclo di vita ---
		virtual Vec2 measure(float parent_w, float parent_h);
		virtual void arrange(Rect space);
		virtual void arrangeInto(Rect space);
		virtual void update(float dt, bool ancestorBlocked = false);
		virtual void draw(float parentOpacity = 1.0f);

		Layout* hitTest(Vec2 p, bool ancestorBlocked = false);

		void updateTree(float dt);

		Rect getRect() const { return rect; }
		Vec2 getMeasuredSize() const { return measuredSize; }  
		const ComputedStyle& getStyle() const { return currentStyle; }

		std::function<void()> onHoverEnter, onHoverExit, onPress, onRelease, onClick;
		std::vector<std::shared_ptr<Layout>> children;
		std::string nodeId;

		void setBackgroundTexture(TextureHandle tex, NineSlice np = {0,0,0,0}) { bgTexture = tex; bgPatchInfo = np; }
		void setShader(EffectHandle shader) { customEffect = shader; hasShader = true; }

	protected:
		LayoutType type;
		Rect rect{ 0,0,0,0 };
		Vec2 measuredSize{ 0,0 };
		Vec2 scrollContentSize{ 0,0 };
		bool styleInitialized{ false };
		bool isInteractive{ true }, blocksRaycast{ false }, isHovered{ false },
		     wantsRemoval{ false }, pendingTransition{ true }, isEnabled{ true };
		bool isFocused{ false };
		bool isFocusable_{ false };
		bool isFocusScope_{ false };
		bool isPressed{ false }; 
		bool isPortal_{ false };
		bool passThrough_{ false };
		bool keyboardActivates_{ false };


		std::string styleTag;
		std::vector<std::string> styleClasses;
		Style inlineBase, inlineHover, inlinePressed, inlineDisabled, inlineFocus;
		ComputedStyle currentStyle, targetStyle, transitionStartStyle;
		UIState currentState{ UIState::Idle };
		float transitionTimer{ 1.0f };

		TextureHandle bgTexture; NineSlice bgPatchInfo;
		EffectHandle customEffect; bool hasShader{ false };

		std::unordered_map<std::string, AnimState> activeAnimations;
		std::vector<ActiveCssAnimation> activeCssAnimations;

		std::weak_ptr<Layout> parent;

		virtual Vec2 computeIntrinsicSize(float /*availW*/, float /*availH*/) { return {0,0}; }

		virtual Vec2 measureChild(Layout* child, float availW, float availH) {
			return child->measure(availW, availH);
		}

		Transform2D currentTransform(const ComputedStyle& style) const {
			Transform2D tr;
			tr.pivot = rect.center();
			tr.translate = {
				style.translateX.resolve(Metrics::viewport.x),
				style.translateY.resolve(Metrics::viewport.y)
			};
			tr.rotationDeg = style.rotation;
			tr.scale = style.scale;
			return tr;
		}

		// Chiamato quando un discendente ottiene il focus.
		// Ritorna true se ha gestito lo scroll internamente.
		virtual void onDescendantFocused(Layout* /*descendant*/) {}

		// Chrome: sfondo + bordo. Chiamato automaticamente prima di renderContent.
		// Override solo se il widget ha un "vestito" particolare (es. texture).
		virtual void renderChrome(float op, const ComputedStyle& style) {
			auto r = UIContext::get().renderer;
			if (!r) return;
			if (rect.width <= 0 || rect.height <= 0) return;

			float maxRadius = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolve(maxRadius * 2.0f), 0.0f, maxRadius);

			Color bg = style.background.withAlpha(op);
			if (bg.a > 0) {
				if (rPx > 0.0f && maxRadius > 0.0f) r->fillRoundedRect(rect, rPx, bg);
				else                                r->fillRect(rect, bg);
			}

			Color bc = style.borderColor.withAlpha(op);
			float bw = style.borderWidth.resolve(maxRadius * 2.0f);
			if (bc.a > 0 && bw > 0.0f) {
				if (rPx > 0.0f && maxRadius > 0.0f) r->strokeRoundedRect(rect, rPx, bw, bc);
				else                                r->strokeRect(rect, bw, bc);
			}
		}

		// Contenuto specifico del widget. Di default non disegna nulla.
		virtual void renderContent(float /*op*/, const ComputedStyle& /*style*/) {}
		virtual void onPreUpdate(float /*dt*/) {}
		virtual void onPostUpdate(float  /*dt*/) {}

	private:
		ComputedStyle resolveTargetStyle();
		void syncCssAnimations();
		void handleFocusInput();
	};

	// CRTP helper: aggiunge la fluent API tipizzata su una Base qualsiasi.
	// Uso:
	//   class Text   : public TLayout<Text>              // Base = Layout
	//   class Panel  : public TLayout<Panel>             // Base = Layout
	//   class Button : public TLayout<Button, Panel>     // Base = Panel
	template <typename Derived, typename Base = Layout>
	class TLayout : public Base {
	public:
		template <typename... Args>
		TLayout(Args&&... args) : Base(std::forward<Args>(args)...) {}

		template <typename... Args>
		static std::shared_ptr<Derived> create(Args&&... args) {
			auto p = std::make_shared<Derived>(std::forward<Args>(args)...);
			static_cast<TLayout<Derived>*>(p.get())->onBuild();
			return p;
		}


		std::shared_ptr<Derived> cls(const std::string& name) {
			this->addClass(name);
			return self();
		}
		std::shared_ptr<Derived> id(const std::string& node_id) {
			this->setId(node_id);
			return self();
		}
		std::shared_ptr<Derived> size(Value w, Value h) {
			this->setSize(w, h);
			return self();
		}
		std::shared_ptr<Derived> passThrough(bool p = true) {
			this->setPassThrough(p);
			return self();
		}
		std::shared_ptr<Derived> with(std::shared_ptr<Layout> c) {
			this->addChild(std::move(c));
			return self();
		}
		std::shared_ptr<Derived> self() {
			return std::static_pointer_cast<Derived>(this->shared_from_this());
		}
	protected:
		virtual void onBuild() {}
	};
}