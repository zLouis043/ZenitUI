#pragma once

#include "Common.hpp"

#include "CoreTypes.hpp"

namespace ZenitUI {

	enum class Feature { Effects, NestedTargets };

	class IRenderer {
	public:
		virtual ~IRenderer() = default;

		virtual void fillRect(Rect r, Color c) = 0;
		virtual void fillRoundedRect(Rect r, float radiusPx, Color c) = 0;
		virtual void fillCircle(Vec2 center, float radius, Color c) = 0;
		
		virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) = 0;
		virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) = 0;
		
		virtual void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) = 0;
		virtual Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) = 0;

		virtual void pushTransform(const Transform2D& t) = 0;
		virtual void popTransform() = 0;
		virtual void pushEffect(EffectHandle e) = 0;
		virtual void popEffect() = 0;

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
	};

	struct UIContext {
		IRenderer* renderer{ nullptr };
		IPlatform* platform{ nullptr };

		PointerState pointer;
		float dt{ 0.0f };
		std::mt19937 rng;

		void beginFrame(float delta_time) {
			dt = delta_time;
			if (platform) {
				Metrics::viewport = platform->viewportSize();
				pointer = platform->pointer();
			}
		}

		static UIContext& get() {
			static UIContext instance;
			return instance;
		}
	};
}