#pragma once

#include "Common.hpp"

#include "CoreTypes.hpp"

namespace ZenitUI
{

	class Layout;

	enum class Feature
	{
		Effects,
		NestedTargets
	};

	class IRenderer
	{
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

		virtual void pushTransform(const Transform2D &t) = 0;
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
		virtual void setEffectFloat(EffectHandle e, const char *name, float value) = 0;
		virtual void setEffectVec2(EffectHandle e, const char *name, Vec2 value) = 0;
		virtual void setEffectVec3(EffectHandle e, const char *name, Vec3 value) = 0;
		virtual void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) = 0;
		virtual void setEffectVec4(EffectHandle e, const char *name, Color value) = 0;
		virtual void clearTarget(TargetHandle t, Color c) = 0;
		virtual bool inTarget() const = 0;

		virtual bool supports(Feature f) const = 0;

		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual void setDpiScale(float scale) = 0;
	};

	class IPlatform
	{
	public:
		virtual ~IPlatform() = default;
		virtual Vec2 viewportSize() = 0;
		virtual PointerState pointer() = 0;
		virtual double time() = 0;
		virtual ModifierState modifiers() = 0;
		virtual InputEvents pollInputEvents() = 0;
		virtual float dpiScale() = 0;
		virtual EdgeInsets safeArea() = 0;
		virtual void setClipboardText(const std::string &text) = 0;
		virtual std::string getClipboardText() = 0;
	};

	class IAssetProvider
	{
	public:
		virtual ~IAssetProvider() = default;
		virtual FontHandle getFont(std::string_view name) = 0;
		virtual TextureHandle getTexture(std::string_view name) = 0;
		virtual EffectHandle getEffect(std::string_view name) = 0;

		virtual FontHandle getFont(const FontStack &stack)
		{
			for (const auto &name : stack)
			{
				FontHandle h = getFont(std::string_view(name));
				if (h.id != 0)
					return h;
			}
			return {};
		}

		FontHandle getFont(const char *name)
		{
			return getFont(std::string_view(name));
		}

		virtual FontHandle loadFont(std::string_view /*name*/,
									std::string_view /*path*/)
		{
			return {};
		}

		virtual FontHandle loadFontFromMemory(std::string_view /*name*/,
											  const void * /*data*/,
											  size_t /*size*/)
		{
			return {};
		}

		virtual void unloadFont(std::string_view /*name*/) {}
	};

	struct UIContext
	{
		IRenderer *renderer{nullptr};
		IPlatform *platform{nullptr};
		IAssetProvider *assets{nullptr};

		PointerState pointer;
		float dt{0.0f};
		double time{0.0};
		float dpiScale{1.0f};
		EdgeInsets safeArea;
		ModifierState modifiers;
		bool wheelConsumedThisFrame{false};
		std::array<bool, (size_t)MouseButton::Count> buttonConsumed{};

		void consumeClick(MouseButton b) { buttonConsumed[(size_t)b] = true; }
		bool wasConsumed(MouseButton b) const { return buttonConsumed[(size_t)b]; }

		InputEvents inputEvents;
		std::weak_ptr<Layout> focusedNode;
		std::weak_ptr<Layout> pointerCapture;

		uint32_t viewportGeneration{0};

		void invalidateStyles() { viewportGeneration++; }

		Layout *topmostConsumer{nullptr};

		Layout *hoverTarget{nullptr};
		Layout *pressTarget{nullptr};
		Layout *releaseTarget{nullptr};

		bool isPressTarget(const Layout *n) const { return pressTarget == n; }
		bool isValidClick() const
		{
			return pressTarget && releaseTarget && pressTarget == releaseTarget;
		}

		std::vector<std::weak_ptr<Layout>> activePortals;
		std::vector<std::weak_ptr<Layout>> framePortals;

		std::mt19937 rng;

		void beginFrame(float delta_time)
		{
			dt = delta_time;
			time += delta_time;
			wheelConsumedThisFrame = false;
			buttonConsumed.fill(false);
			topmostConsumer = nullptr;
			hoverTarget = nullptr;
			releaseTarget = nullptr;
			activePortals = std::move(framePortals);
			framePortals.clear();
			if (platform)
			{
				Vec2 newVp = platform->viewportSize();
				EdgeInsets newSafe = platform->safeArea();
				if (newVp.x != Metrics::viewport.x || newVp.y != Metrics::viewport.y ||
					newSafe != safeArea)
				{
					viewportGeneration++;
				}
				Metrics::viewport = newVp;
				safeArea = newSafe;
				pointer = platform->pointer();
				modifiers = platform->modifiers();
				inputEvents = platform->pollInputEvents();
				dpiScale = platform->dpiScale();
				if (renderer)
				{
					renderer->setDpiScale(dpiScale);
				}
			}
		}

		void requestFocus(std::shared_ptr<Layout> n);
		void releaseFocus() { focusedNode.reset(); }
		bool hasFocus(const Layout *n) const
		{
			auto sp = focusedNode.lock();
			return sp && sp.get() == n;
		}

		static UIContext &get()
		{
			static UIContext instance;
			return instance;
		}
	};
}