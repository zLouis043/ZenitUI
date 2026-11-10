#pragma once

/**
 * \file UIContext.hpp
 * \brief Per-application state: renderer, platform, pointer, focus, portals.
 *
 * `UIContext` is a singleton (`UIContext::get()`) that aggregates every piece
 * of mutable per-frame state the framework needs but does not want to thread
 * through every call. It is the boundary between the platform layer (window,
 * input, clock) and the framework (layout, style, render).
 *
 * Two interface families live here:
 *
 *  - `IRenderer` — abstract drawing backend. Concrete implementations target
 *    a specific GPU API or a software rasterizer; the framework never talks
 *    to a specific backend directly.
 *
 *  - `IPlatform` / `IAssetProvider` — abstract input and asset backends.
 *
 * `UIContext` itself holds:
 *
 *  - The current frame's pointer / modifier / input snapshot.
 *  - Focus and pointer-capture ownership.
 *  - Hit-test and click-routing state.
 *  - The per-frame portal registry.
 *  - One-shot "was consumed" flags for buttons and the wheel.
 */

#include "Common.hpp"

#include "CoreTypes.hpp"

namespace ZenitUI
{

	class Layout;

	/**
	 * \enum Feature
	 * \brief Optional renderer capabilities.
	 */
	enum class Feature
	{
		Effects,
		NestedTargets
	};

	/**
	 * \class IRenderer
	 * \brief Abstract 2D rendering backend.
	 */
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		// ---- Shape primitives ----
		virtual void fillRect(Rect r, Color c) = 0;
		virtual void fillRoundedRect(Rect r, float radiusPx, Color c) = 0;
		virtual void fillCircle(Vec2 center, float radius, Color c) = 0;

		virtual void strokeRect(Rect r, float thickness, Color c) = 0;
		virtual void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) = 0;

		/**
		 * \brief Draw a rectangular ring by filling the four edges.
		 *
		 * Default implementation issues up to four `fillRect` calls. Backends
		 * that can draw true borders (e.g. via a single shader) should
		 * override.
		 */
		virtual void fillBorder(Rect r, EdgeInsets sides, Color c)
		{
			if (sides.top > 0.0f)
				fillRect({r.x, r.y, r.width, sides.top}, c);
			if (sides.bottom > 0.0f)
				fillRect({r.x, r.y + r.height - sides.bottom, r.width, sides.bottom}, c);
			float innerH = r.height - sides.top - sides.bottom;
			if (innerH > 0.0f)
			{
				if (sides.left > 0.0f)
					fillRect({r.x, r.y + sides.top, sides.left, innerH}, c);
				if (sides.right > 0.0f)
					fillRect({r.x + r.width - sides.right, r.y + sides.top, sides.right, innerH}, c);
			}
		}

		// ---- Textures ----
		virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) = 0;
		virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) = 0;

		// ---- Text ----
		virtual void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) = 0;
		virtual Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) = 0;

		// ---- State stacks ----
		virtual void pushTransform(const Transform2D &t) = 0;
		virtual void popTransform() = 0;
		virtual void pushEffect(EffectHandle e) = 0;
		virtual void popEffect() = 0;
		virtual void pushClip(Rect r) = 0;
		virtual void popClip() = 0;

		/** \brief Current clip rectangle in logical pixels. */
		virtual Rect getClipRect() const = 0;

		// ---- Render targets ----
		virtual TargetHandle createTarget(int w, int h) = 0;
		virtual void destroyTarget(TargetHandle t) = 0;
		virtual void pushTarget(TargetHandle t) = 0;
		virtual void popTarget() = 0;
		virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;

		// ---- Effect uniform setters ----
		virtual void setEffectFloat(EffectHandle e, const char *name, float value) = 0;
		virtual void setEffectVec2(EffectHandle e, const char *name, Vec2 value) = 0;
		virtual void setEffectVec3(EffectHandle e, const char *name, Vec3 value) = 0;
		virtual void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) = 0;
		virtual void setEffectVec4(EffectHandle e, const char *name, Color value) = 0;

		/** \brief Clear a render target to \p c. */
		virtual void clearTarget(TargetHandle t, Color c) = 0;

		/** \brief `true` while a `pushTarget` is active without a matching `popTarget`. */
		virtual bool inTarget() const = 0;

		/** \brief Query whether the backend supports a `Feature`. */
		virtual bool supports(Feature f) const = 0;

		// ---- Frame lifecycle ----
		virtual void beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual void setDpiScale(float scale) = 0;
	};

	/**
	 * \class IPlatform
	 * \brief Abstract windowing and input backend.
	 */
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

	/**
	 * \class IAssetProvider
	 * \brief Abstract font / texture / effect lookup.
	 */
	class IAssetProvider
	{
	public:
		virtual ~IAssetProvider() = default;
		virtual FontHandle getFont(std::string_view name) = 0;
		virtual TextureHandle getTexture(std::string_view name) = 0;
		virtual EffectHandle getEffect(std::string_view name) = 0;

		/** \brief Try every name in \p stack; return the first valid `FontHandle`. */
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

		/** \brief Convenience overload for C strings. */
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

	/**
	 * \struct UIContext
	 * \brief Per-application singleton: backends, frame state, and routing.
	 */
	struct UIContext
	{
		// ---- Backends ----
		IRenderer *renderer{nullptr};
		IPlatform *platform{nullptr};
		IAssetProvider *assets{nullptr};

		// ---- Per-frame state ----
		PointerState pointer;
		float dt{0.0f};
		double time{0.0};
		float dpiScale{1.0f};
		EdgeInsets safeArea;
		ModifierState modifiers;
		bool wheelConsumedThisFrame{false};
		std::array<bool, (size_t)MouseButton::Count> buttonConsumed{};

		/** \brief Mark \p b as consumed; subsequent handlers in the same frame will skip it. */
		void consumeClick(MouseButton b) { buttonConsumed[(size_t)b] = true; }
		/** \brief `true` if \p b was already consumed this frame. */
		bool wasConsumed(MouseButton b) const { return buttonConsumed[(size_t)b]; }

		InputEvents inputEvents;
		std::weak_ptr<Layout> focusedNode;
		std::weak_ptr<Layout> pointerCapture;

		uint32_t viewportGeneration{0};

		/** \brief Force style-cache invalidation across the tree. */
		void invalidateStyles() { viewportGeneration++; }

		// ---- Hit-test and click routing ----
		Layout *topmostConsumer{nullptr};
		Layout *hoverTarget{nullptr};
		Layout *pressTarget{nullptr};
		Layout *releaseTarget{nullptr};

		/** \brief `true` if \p n is the current press target. */
		bool isPressTarget(const Layout *n) const { return pressTarget == n; }
		/** \brief `true` if press and release targets agree (a valid click). */
		bool isValidClick() const
		{
			return pressTarget && releaseTarget && pressTarget == releaseTarget;
		}

		// ---- Portals ----
		std::vector<std::weak_ptr<Layout>> activePortals;
		std::vector<std::weak_ptr<Layout>> framePortals;

		std::mt19937 rng;

		/**
		 * \brief Advance the frame: pull platform state and reset one-shot flags.
		 *
		 * Called once at the top of `Layout::runFrame`. On viewport or
		 * safe-area change, `viewportGeneration` is bumped so every cached
		 * style key changes and everything re-resolves.
		 *
		 * \param delta_time Seconds since the previous frame.
		 */
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

		/** \brief Set \p n as the focused node; null clears focus. */
		void requestFocus(std::shared_ptr<Layout> n);
		/** \brief Clear focus. */
		void releaseFocus() { focusedNode.reset(); }
		/** \brief `true` if \p n is the current focused node. */
		bool hasFocus(const Layout *n) const
		{
			auto sp = focusedNode.lock();
			return sp && sp.get() == n;
		}

		/** \brief Return the global singleton context. */
		static UIContext &get()
		{
			static UIContext instance;
			return instance;
		}
	};
}