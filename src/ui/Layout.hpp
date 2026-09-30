#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "Theme.hpp"
#include "UIContext.hpp"
#include "UIAnimations.hpp"


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

		std::shared_ptr<Layout> cls(const std::string& className) {
			styleClasses.push_back(className);
			pendingTransition = true;
			return shared_from_this();
		}
		std::shared_ptr<Layout> id(const std::string& node_id) {
			nodeId = node_id;
			return shared_from_this();
		}
		std::shared_ptr<Layout> size(Value w, Value h) {
			inlineBase.width = w;
			inlineBase.height = h;
			pendingTransition = true;
			return shared_from_this();
		}
		std::shared_ptr<Layout> with(std::shared_ptr<Layout> child) {
			addChild(std::move(child));
			return shared_from_this();
		}

		// --- Albero ---
		void addChild(std::shared_ptr<Layout> child) {
			if (!child) return;
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
		Vec2 measure(float parent_w, float parent_h);
		void arrange(Rect space);
		virtual void update(float dt, bool ancestorBlocked = false);
		virtual void draw(float parentOpacity = 1.0f);

		Rect getRect() const { return rect; }
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

		bool isInteractive{ true }, blocksRaycast{ false }, isHovered{ false }, wantsRemoval{ false }, pendingTransition{ true };

		std::vector<std::string> styleClasses;
		Style inlineBase, inlineHover, inlinePressed;
		ComputedStyle currentStyle, targetStyle, transitionStartStyle;
		UIState currentState{ UIState::Idle };
		float transitionTimer{ 1.0f };

		TextureHandle bgTexture; NineSlice bgPatchInfo;
		EffectHandle customEffect; bool hasShader{ false };

		std::unordered_map<std::string, AnimState> activeAnimations;
		std::vector<ActiveCssAnimation> activeCssAnimations;

		std::weak_ptr<Layout> parent;

		virtual Vec2 computeIntrinsicSize(float availW, float availH) { return {0,0}; }

		// renderSelf riceve il render-style (currentStyle + overlay animazioni CSS).
		virtual void renderSelf(float globalOpacity, const ComputedStyle& style) {
			auto r = UIContext::get().renderer;
			if (!r) return;
			Color bg = style.background.withAlpha(globalOpacity);
			if (bg.a > 0) {
				float maxRadius = std::min(rect.width, rect.height) * 0.5f;
				float rPx = std::clamp(style.radius.resolve(maxRadius * 2.0f), 0.0f, maxRadius);
				if (rPx > 0.0f && maxRadius > 0.0f) r->fillRoundedRect(rect, rPx, bg);
				else if (rect.width > 0 && rect.height > 0) r->fillRect(rect, bg);
			}
		}

		virtual void onPreUpdate(float dt) {}
		virtual void onPostUpdate(float dt) {}

	private:
		ComputedStyle resolveTargetStyle(UIState state);
		void syncCssAnimations();
	};

	template <typename Derived>
	class TLayout : public Layout {
	public:
		TLayout(LayoutType t = LayoutType::Stack) : Layout(t) {}

		std::shared_ptr<Derived> cls(const std::string& name) {
			this->styleClasses.push_back(name);
			this->pendingTransition = true;
			return self();
		}
		std::shared_ptr<Derived> id(const std::string& node_id) {
			this->nodeId = node_id;
			return self();
		}
		std::shared_ptr<Derived> size(Value w, Value h) {
			this->inlineBase.width = w;
			this->inlineBase.height = h;
			this->pendingTransition = true;
			return self();
		}
		std::shared_ptr<Derived> with(std::shared_ptr<Layout> c) {
			this->addChild(std::move(c));
			return self();
		}
		std::shared_ptr<Derived> self() {
			return std::static_pointer_cast<Derived>(this->shared_from_this());
		}
	};
}