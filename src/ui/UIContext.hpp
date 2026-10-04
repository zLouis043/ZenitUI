#pragma once

#include "Common.hpp"

#include "CoreTypes.hpp"

namespace ZenitUI {

	class Layout;

	enum class Feature { Effects, NestedTargets };

	class IRenderer {
	public:
		virtual ~IRenderer() = default;

		virtual void fillRect(Rect r, Color c) = 0;
		virtual void fillRoundedRect(Rect r, float radiusPx, Color c) = 0;
		virtual void fillCircle(Vec2 center, float radius, Color c) = 0;
		
		virtual void strokeRect(Rect r, float thickness, Color c) = 0;
		virtual void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) = 0;

		virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) = 0;
		virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) = 0;
		
		virtual void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) = 0;
		virtual Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) = 0;

		virtual void pushTransform(const Transform2D& t) = 0;
		virtual void popTransform() = 0;
		virtual void pushEffect(EffectHandle e) = 0;
		virtual void popEffect() = 0;
		virtual void pushClip(Rect r) = 0;
		virtual void popClip() = 0;

		virtual Rect getClipRect() const = 0;

		virtual TargetHandle createTarget(int w, int h) = 0;
		virtual void destroyTarget(TargetHandle t) = 0;
		virtual void pushTarget(TargetHandle t) = 0;
		virtual void popTarget() = 0;
		virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;

		virtual bool supports(Feature f) const = 0;
	};

	class IPlatform {
	public:
		virtual ~IPlatform() = default;
		virtual Vec2 viewportSize() = 0;
		virtual PointerState pointer() = 0;
		virtual double time() = 0;
		virtual bool shiftHeld() = 0;
		virtual InputEvents pollInputEvents() = 0;  
	};

	class IAssetProvider {
	public:
		virtual ~IAssetProvider() = default;
		virtual FontHandle    getFont(std::string_view name) = 0;
		virtual TextureHandle getTexture(std::string_view name) = 0;
		virtual EffectHandle  getEffect(std::string_view name) = 0;
	};

	struct UIContext {
		IRenderer* renderer{ nullptr };
		IPlatform* platform{ nullptr };
		IAssetProvider* assets  { nullptr }; 

		PointerState pointer;
		float dt{ 0.0f };
		bool wheelConsumedThisFrame{ false };
		bool shiftHeld{ false }; 
		bool clickConsumed{ false };
		void consumeClick() { clickConsumed = true; }
		bool rightClickConsumed{ false };
		void consumeRightClick() { rightClickConsumed = true; }

		InputEvents inputEvents;
		std::weak_ptr<Layout> focusedNode;
		std::weak_ptr<Layout> pointerCapture;

		Layout* topmostConsumer{ nullptr };  

		Layout* hoverTarget   { nullptr };  // topmost sotto il mouse (geometrico)
		Layout* pressTarget   { nullptr };  // fissato al mouse-down
		Layout* releaseTarget { nullptr };  // fissato al mouse-up

		bool isPressTarget(const Layout* n) const { return pressTarget == n; }
		bool isValidClick() const {
			return pressTarget && releaseTarget && pressTarget == releaseTarget;
		}

		std::vector<std::weak_ptr<Layout>> activePortals;
		std::vector<std::weak_ptr<Layout>> framePortals;

		std::mt19937 rng;

		void beginFrame(float delta_time) {
			dt = delta_time;
			wheelConsumedThisFrame = false; 
			clickConsumed = false;
			rightClickConsumed = false; 
			topmostConsumer = nullptr; 
			hoverTarget = nullptr;      // <-- nuovo
    		releaseTarget = nullptr;    // <-- nuovo
			activePortals = std::move(framePortals);
			framePortals.clear();
			if (platform) {
				Metrics::viewport = platform->viewportSize();
				pointer = platform->pointer();
				shiftHeld = platform->shiftHeld();
				inputEvents       = platform->pollInputEvents();
			}
		}

		void requestFocus(std::shared_ptr<Layout> n);
		void releaseFocus()                          { focusedNode.reset(); }
		bool hasFocus(const Layout* n) const {
			auto sp = focusedNode.lock();
			return sp && sp.get() == n;
		}

		static UIContext& get() {
			static UIContext instance;
			return instance;
		}
	};
}