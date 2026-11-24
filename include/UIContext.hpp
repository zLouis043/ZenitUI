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
	/** \addtogroup grp_core
	 *  @{
	 */

	class Layout;

	/**
	 * \enum Feature
	 * \brief Optional renderer capabilities queried via `IRenderer::supports`.
	 *
	 * Backends that only implement core 2D drawing can return `false` for all
	 * features; ZenitUI will skip shader and off-screen layer passes gracefully.
	 * See \ref page_custom_backends for details.
	 */
	enum class Feature
	{
		Effects,       ///< Supports fragment shaders (`pushEffect`, `popEffect`, `setEffect*`).
		NestedTargets, ///< Supports off-screen render targets (`createTarget`, `pushTarget`, `popTarget`, `drawTarget`).
		TargetSubRect  ///< Supports sub-rectangle blitting via `drawTarget(t, src, dst, tint)` for pooled targets.
	};

	/**
	 * \class IRenderer
	 * \brief Abstract 2D rendering backend.
	 *
	 * All coordinates, rectangles, radii, and font sizes are expressed in
	 * **logical pixels** (post-DPI). Backends apply `dpiScale` internally when
	 * configuring the GPU transform or physical scissor rect.
	 *
	 * See \ref page_custom_backends for the full porting guide and
	 * \ref grp_backends for reference implementations (`RaylibRenderer`, `MockRenderer`).
	 */
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		/// \name Shape Primitives
		/// @{

		/** \brief Fill an axis-aligned rectangle `r` with solid color `c`. */
		virtual void fillRect(Rect r, Color c) = 0;

		/** \brief Fill a rectangle `r` with corner radius `radiusPx` (clamped to half the shorter side). */
		virtual void fillRoundedRect(Rect r, float radiusPx, Color c) = 0;

		/** \brief Fill a circle centered at `center` with the given logical `radius`. */
		virtual void fillCircle(Vec2 center, float radius, Color c) = 0;

		/**
		 * \brief Stroke a rectangular border strictly **inside** `r` (CSS border-box).
		 *
		 * Drawing inward keeps the border flush against `fillRect(r, ...)` without
		 * anti-aliasing seams or layout overflow.
		 */
		virtual void strokeRect(Rect r, float thickness, Color c) = 0;

		/**
		 * \brief Stroke a rounded border strictly **inside** `r` (CSS border-box).
		 *
		 * Outer radius is `radiusPx`; inner radius should shrink by `thickness`
		 * (clamped at 0) so the border stays flush against `fillRoundedRect(r, ...)`.
		 */
		virtual void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) = 0;

		/**
		 * \brief Draw a rectangular ring with per-edge widths by filling the four edges.
		 *
		 * Default implementation issues up to four `fillRect` calls. Backends
		 * that can draw per-edge borders in a single draw call may override.
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
		/// @}

		/// \name Textures & Nine-Slice
		/// @{

		/** \brief Draw sub-rectangle `src` (texture pixels) of texture `t` into logical rect `dst`, multiplied by `tint`. */
		virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) = 0;

		/** \brief Draw texture `t` stretched into `dst` using the 9-slice insets `s` (fixed corners, stretched edges/center). */
		virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) = 0;
		/// @}

		/// \name Text Rendering & Metrics
		/// @{

		/**
		 * \brief Draw UTF-8 text `s` at logical top-left `pos` using font `f`, font size `size`, and letter spacing `spacing`.
		 * \note If `f.id == 0` or unknown, backends should fall back to a default built-in font.
		 */
		virtual void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) = 0;

		/** \brief Measure the logical `{width, height}` of UTF-8 text `s` rendered with `f`, `size`, and `spacing`. */
		virtual Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) = 0;

		/**
		 * \brief Measure the advance width and height of a single Unicode codepoint.
		 *
		 * Default implementation encodes `codepoint` as UTF-8 and delegates to
		 * `measureText`. Backends with direct glyph tables (e.g. `RaylibRenderer`)
		 * can override for O(1) lookup without string allocation.
		 */
		virtual Vec2 measureCodepoint(FontHandle f, uint32_t codepoint, float size, float spacing)
		{
			char buf[4];
			size_t len = 0;
			if (codepoint <= 0x7Fu)
			{
				buf[0] = static_cast<char>(codepoint);
				len = 1;
			}
			else if (codepoint <= 0x7FFu)
			{
				buf[0] = static_cast<char>(0xC0u | (codepoint >> 6));
				buf[1] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
				len = 2;
			}
			else if (codepoint <= 0xFFFFu && !(codepoint >= 0xD800u && codepoint <= 0xDFFFu))
			{
				buf[0] = static_cast<char>(0xE0u | (codepoint >> 12));
				buf[1] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
				buf[2] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
				len = 3;
			}
			else if (codepoint <= 0x10FFFFu && !(codepoint >= 0xD800u && codepoint <= 0xDFFFu))
			{
				buf[0] = static_cast<char>(0xF0u | (codepoint >> 18));
				buf[1] = static_cast<char>(0x80u | ((codepoint >> 12) & 0x3Fu));
				buf[2] = static_cast<char>(0x80u | ((codepoint >> 6) & 0x3Fu));
				buf[3] = static_cast<char>(0x80u | (codepoint & 0x3Fu));
				len = 4;
			}
			if (len == 0)
				return {0.0f, size};
			return measureText(f, std::string_view(buf, len), size, spacing);
		}
		/// @}

		/// \name State Stacks
		/// @{

		/** \brief Push a 2D pivot-based transform onto the transform stack. */
		virtual void pushTransform(const Transform2D &t) = 0;

		/** \brief Pop the most recently pushed transform from the stack. */
		virtual void popTransform() = 0;

		/** \brief Bind shader effect `e` and push it onto the effect stack (used when `Feature::Effects` is supported). */
		virtual void pushEffect(EffectHandle e) = 0;

		/** \brief Pop the active shader effect and restore the previous one (if any). */
		virtual void popEffect() = 0;

		/**
		 * \brief Push a local-space clip rectangle `r`, intersecting it with the current clip.
		 *
		 * Implementations must transform `r` through the active transform stack
		 * before intersecting so scissoring matches rotated/scaled subtrees.
		 */
		virtual void pushClip(Rect r) = 0;

		/** \brief Pop the current clip rectangle and restore the previous scissor region. */
		virtual void popClip() = 0;

		/** \brief Return the active screen-space clip rectangle in logical pixels (used for culling). */
		virtual Rect getClipRect() const = 0;
		/// @}

		/// \name Off-Screen Render Targets
		/// @{

		/** \brief Allocate an off-screen RGBA render target of `w x h` pixels (used when `Feature::NestedTargets` is supported). */
		virtual TargetHandle createTarget(int w, int h) = 0;

		/** \brief Release a render target previously allocated by `createTarget`. */
		virtual void destroyTarget(TargetHandle t) = 0;

		/**
		 * \brief Redirect subsequent draw calls to target `t` and clear it to transparent black.
		 *
		 * Must save and reset the current clip and transform stacks so rendering
		 * inside `t` starts cleanly, then restore them on `popTarget`.
		 */
		virtual void pushTarget(TargetHandle t) = 0;

		/** \brief Finish drawing into the active target and restore the previous target (or main framebuffer) and stacks. */
		virtual void popTarget() = 0;

		/** \brief Composite the entire render target `t` into logical destination rect `dst`, multiplied by `tint`. */
		virtual void drawTarget(TargetHandle t, Rect dst, Color tint) = 0;

		/**
		 * \brief Composite sub-rectangle `src` (target pixels) of `t` into logical rect `dst`.
		 *
		 * Used by `TargetPool` when `supports(Feature::TargetSubRect)` is `true`.
		 * Default falls back to full-target `drawTarget(t, dst, tint)`.
		 */
		virtual void drawTarget(TargetHandle t, Rect /*src*/, Rect dst, Color tint)
		{
			drawTarget(t, dst, tint);
		}

		/** \brief Clear render target `t` to solid color `c`. */
		virtual void clearTarget(TargetHandle t, Color c) = 0;

		/** \brief Return `true` while a `pushTarget` is active without a matching `popTarget`. */
		virtual bool inTarget() const = 0;
		/// @}

		/// \name Shader Uniforms
		/// @{

		/** \brief Set a `float` uniform named `name` on effect `e`. */
		virtual void setEffectFloat(EffectHandle e, const char *name, float value) = 0;

		/** \brief Set a `vec2` uniform named `name` on effect `e`. */
		virtual void setEffectVec2(EffectHandle e, const char *name, Vec2 value) = 0;

		/** \brief Set a `vec3` uniform named `name` on effect `e`. */
		virtual void setEffectVec3(EffectHandle e, const char *name, Vec3 value) = 0;

		/** \brief Set a `vec4` uniform named `name` on effect `e`. */
		virtual void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) = 0;

		/** \brief Set a normalized RGBA `vec4` uniform (0..1 per channel) named `name` on effect `e`. */
		virtual void setEffectVec4(EffectHandle e, const char *name, Color value) = 0;
		/// @}

		/// \name Capabilities & Frame Lifecycle
		/// @{

		/** \brief Query whether this backend implements an optional `Feature`. */
		virtual bool supports(Feature f) const = 0;

		/** \brief Called at the start of `Layout::renderFrame` before any draw calls (e.g. to push a DPI scale matrix). */
		virtual void beginFrame() = 0;

		/** \brief Called at the end of `Layout::renderFrame` after all draw calls and portals have rendered. */
		virtual void endFrame() = 0;

		/** \brief Notify the renderer of the active logical-to-physical DPI scale factor for the current frame. */
		virtual void setDpiScale(float scale) = 0;
		/// @}
	};

	/**
	 * \class IPlatform
	 * \brief Abstract windowing, timing, input, and clipboard backend.
	 *
	 * Polled once per frame by `UIContext::beginFrame()`. See
	 * \ref page_custom_backends for invariants and \ref grp_backends for
	 * reference implementations (`RaylibPlatform`, `MockPlatform`).
	 */
	class IPlatform
	{
	public:
		virtual ~IPlatform() = default;

		/** \brief Return the current window/surface dimensions in **logical pixels**. */
		virtual Vec2 viewportSize() = 0;

		/**
		 * \brief Return the current pointer position (logical pixels), scroll wheel delta, and button states.
		 *
		 * Edge-triggered fields (`ButtonState::pressed`, `ButtonState::released`,
		 * `PointerState::wheelY`) must fire for **one frame only**; `ButtonState::down`
		 * stays `true` while held.
		 */
		virtual PointerState pointer() = 0;

		/** \brief Return a monotonic clock timestamp in seconds. */
		virtual double time() = 0;

		/** \brief Return the current state of modifier keys (`shift`, `ctrl`, `alt`, `super`). */
		virtual ModifierState modifiers() = 0;

		/**
		 * \brief Drain and return keyboard events queued since the previous frame.
		 *
		 * `InputEvents::chars` and `InputEvents::keys` are edge-triggered (one frame);
		 * `InputEvents::held` lists keys currently held down for repeat handling.
		 */
		virtual InputEvents pollInputEvents() = 0;

		/** \brief Return the physical-to-logical DPI scale factor (e.g. `1.0f` on standard displays, `2.0f` on Retina). */
		virtual float dpiScale() = 0;

		/** \brief Return screen safe-area insets in logical pixels (notches, rounded corners, system bars). */
		virtual EdgeInsets safeArea() = 0;

		/** \brief Copy UTF-8 `text` to the operating system clipboard. */
		virtual void setClipboardText(const std::string &text) = 0;

		/** \brief Read UTF-8 text from the operating system clipboard (empty string if unavailable). */
		virtual std::string getClipboardText() = 0;
	};

	/**
	 * \class IAssetProvider
	 * \brief Abstract name-based lookup and runtime lifecycle for fonts, textures, and shaders.
	 *
	 * Lookup methods (`getFont`, `getTexture`, `getEffect`) are called frequently
	 * during layout and draw; implementations should avoid heap allocations
	 * (e.g. via transparent `std::string_view` hashing). Return a default-constructed
	 * invalid handle (`id == 0`) when a requested asset is not registered.
	 *
	 * See \ref page_custom_backends and \ref grp_backends (`RaylibAssetProvider`, `MockAssetProvider`).
	 */
	class IAssetProvider
	{
	public:
		virtual ~IAssetProvider() = default;

		/** \brief Look up a loaded font by logical `name`; returns `{0}` if not found. */
		virtual FontHandle getFont(std::string_view name) = 0;

		/** \brief Look up a loaded 2D texture by logical `name`; returns `{0}` if not found. */
		virtual TextureHandle getTexture(std::string_view name) = 0;

		/** \brief Look up a loaded shader effect by logical `name`; returns `{0}` if not found. */
		virtual EffectHandle getEffect(std::string_view name) = 0;

		/** \brief Walk fallback font names in `stack` and return the first valid `FontHandle`. */
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

		/** \brief Convenience overload forwarding C strings to `getFont(std::string_view)`. */
		FontHandle getFont(const char *name)
		{
			return getFont(std::string_view(name));
		}

		/**
		 * \brief Optional runtime hook to load a font from disk under the given logical name.
		 * \return Registered `FontHandle`, or `{0}` if unsupported or loading failed.
		 */
		virtual FontHandle loadFont(std::string_view /*name*/,
									std::string_view /*path*/)
		{
			return {};
		}

		/**
		 * \brief Optional runtime hook to load a font from an in-memory TTF/OTF buffer.
		 * \return Registered `FontHandle`, or `{0}` if unsupported or loading failed.
		 */
		virtual FontHandle loadFontFromMemory(std::string_view /*name*/,
											  const void * /*data*/,
											  size_t /*size*/)
		{
			return {};
		}

		/** \brief Optional runtime hook to unload and unregister a font by logical name. */
		virtual void unloadFont(std::string_view /*name*/) {}
	};

	/**
	 * \struct RenderStats
	 * \brief Per-frame and live render-target / filter counters (Proposal 9, Step 0).
	 */
	struct RenderStats
	{
		uint32_t targetsCreated{0};
		uint32_t targetsDestroyed{0};
		uint32_t liveTargets{0};
		uint32_t pushTargetCalls{0};
		uint32_t filterPasses{0};

		void beginFrame()
		{
			targetsCreated = 0;
			targetsDestroyed = 0;
			pushTargetCalls = 0;
			filterPasses = 0;
		}

		void resetAll()
		{
			beginFrame();
			liveTargets = 0;
		}
	};

	/**
	 * \class TargetPool
	 * \brief Shared, bucket-sized render-target pool owned by `UIContext` (§4.9 Step c).
	 */
	class TargetPool
	{
	public:
		static constexpr int BUCKET_SIZE = 64;
		static constexpr int MAX_DIM = 4096;

		static int bucketDim(int v)
		{
			int rounded = ((v + BUCKET_SIZE - 1) / BUCKET_SIZE) * BUCKET_SIZE;
			return rounded < MAX_DIM ? rounded : MAX_DIM;
		}

		TargetHandle acquire(IRenderer &r, int w, int h, RenderStats *stats = nullptr)
		{
			if (w <= 0 || h <= 0)
				return {};

			const bool useSubRect = r.supports(Feature::TargetSubRect);
			const int reqW = useSubRect ? bucketDim(w) : w;
			const int reqH = useSubRect ? bucketDim(h) : h;

			// Ricerca LIFO di un target libero con le dimensioni richieste/arrotondate.
			for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
			{
				if (!it->inUse && it->handle.width == reqW && it->handle.height == reqH)
				{
					it->inUse = true;
					it->lastUsedFrame = frameIndex_;
					return it->handle;
				}
			}

			TargetHandle hnd = r.createTarget(reqW, reqH);
			if (!hnd.valid())
				return {};

			entries_.push_back({hnd, true, frameIndex_});
			if (stats)
			{
				stats->targetsCreated++;
				stats->liveTargets++;
			}
			return hnd;
		}

		void release(TargetHandle t)
		{
			if (!t.valid())
				return;
			for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
			{
				if (it->handle.id == t.id)
				{
					it->inUse = false;
					it->lastUsedFrame = frameIndex_;
					return;
				}
			}
		}

		void trim(IRenderer &r, RenderStats *stats = nullptr, int graceFrames = 120)
		{
			const uint64_t grace = graceFrames > 0 ? static_cast<uint64_t>(graceFrames) : 1u;
			for (auto it = entries_.begin(); it != entries_.end();)
			{
				if (!it->inUse && (frameIndex_ - it->lastUsedFrame) >= grace)
				{
					r.destroyTarget(it->handle);
					if (stats)
					{
						stats->targetsDestroyed++;
						if (stats->liveTargets > 0)
							stats->liveTargets--;
					}
					it = entries_.erase(it);
				}
				else
				{
					++it;
				}
			}
			frameIndex_++;
		}

		void clear(IRenderer &r, RenderStats *stats = nullptr)
		{
			for (auto &e : entries_)
			{
				if (e.handle.valid())
				{
					r.destroyTarget(e.handle);
					if (stats)
					{
						stats->targetsDestroyed++;
						if (stats->liveTargets > 0)
							stats->liveTargets--;
					}
				}
			}
			entries_.clear();
			frameIndex_ = 0;
		}

		size_t liveCount() const { return entries_.size(); }

	private:
		struct Entry
		{
			TargetHandle handle{};
			bool inUse{false};
			uint64_t lastUsedFrame{0};
		};
		std::vector<Entry> entries_;
		uint64_t frameIndex_{0};
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

		// ---- Render instrumentation & target pool ----
		RenderStats renderStats;
		TargetPool targetPool;

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
			renderStats.beginFrame();
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

		/** \brief End-of-frame housekeeping (trims idle pooled render targets). */
		void endFrame()
		{
			if (renderer)
				targetPool.trim(*renderer, &renderStats);
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

	/** @} */
}