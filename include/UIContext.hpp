#pragma once

/**
 * \file UIContext.hpp
 * \brief Per-application state: renderer, platform, pointer, focus, portals.
 *
 * `UIContext` aggregates every piece
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
#include "Geometry2D.hpp"
#include "FilterRegistry.hpp"
#include "Theme.hpp"
#include "TextEngine.hpp"

#include <span>

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
		Effects,	   ///< Supports fragment shaders (`pushEffect`, `popEffect`, `setEffect*`).
		NestedTargets, ///< Supports off-screen render targets (`createTarget`, `pushTarget`, `popTarget`, `drawTarget`).
		TargetSubRect  ///< Supports sub-rectangle blitting via `drawTarget(t, src, dst, tint)` for pooled targets.
	};

	/**
	 * \enum ShaderLanguage
	 * \brief Shading language consumed by an `IRenderer` that supports `Feature::Effects`.
	 */
	enum class ShaderLanguage
	{
		None,	   ///< Backend does not execute custom shaders.
		GLSL330,   ///< OpenGL 3.3 Core GLSL (`.fs` / `.vs`, Raylib & SDL3 GL convention).
		GLSL300ES, ///< OpenGL ES 3.0 / WebGL 2 GLSL.
		SPIRV,	   ///< Precompiled Vulkan / SDL_GPU SPIR-V binaries (`.frag.spv`).
		HLSL,	   ///< Direct3D HLSL (`.hlsl`).
		MSL,	   ///< Apple Metal Shading Language (`.metal`).
		WGSL	   ///< WebGPU Shading Language (`.wgsl`).
	};

	/**
	 * \enum BlendMode
	 * \brief Color-blending mode used by `IRenderer` draw calls.
	 */
	enum class BlendMode
	{
		Alpha,         ///< Standard straight-alpha compositing (default).
		Premultiplied, ///< Premultiplied-alpha compositing for multi-draw cached FBO layers.
		Replace        ///< Overwrite destination RGBA directly without blending (single-quad filter passes).
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

		/// \name Low-Level Indexed Geometry
		/// @{

		/**
		 * \brief Draw an indexed triangle list in logical screen coordinates.
		 *
		 * This is the single mandatory geometry primitive for backends. When
		 * `tex.valid()` is `false`, backends should sample a 1x1 opaque white
		 * texture so solid-colored vertices render unmodified.
		 *
		 * Triangles are emitted counter-clockwise in NDC (Y-down screen space)
		 * by all `Geometry2D` builders.
		 */
		virtual void drawGeometry(std::span<const Vertex2D> vertices,
								  std::span<const uint32_t> indices,
								  TextureHandle tex = {}) = 0;
		/// @}

		/// \name Shape Primitives
		/// @{

		/** \brief Fill an axis-aligned rectangle `r` with solid color `c`. */
		virtual void fillRect(Rect r, Color c)
		{
			const auto m = Geometry2D::buildQuad(r, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}

		/** \brief Fill a rectangle `r` with corner radius `radiusPx` (clamped to half the shorter side). */
		virtual void fillRoundedRect(Rect r, float radiusPx, Color c)
		{
			const auto m = Geometry2D::buildRoundedRectFill(r, radiusPx, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}

		/** \brief Fill a circle centered at `center` with the given logical `radius`. */
		virtual void fillCircle(Vec2 center, float radius, Color c)
		{
			const auto m = Geometry2D::buildCircle(center, radius, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}

		/**
		 * \brief Stroke a rectangular border strictly **inside** `r` (CSS border-box).
		 *
		 * Drawing inward keeps the border flush against `fillRect(r, ...)` without
		 * anti-aliasing seams or layout overflow.
		 */
		virtual void strokeRect(Rect r, float thickness, Color c)
		{
			const auto m = Geometry2D::buildRectStroke(r, thickness, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}

		/**
		 * \brief Stroke a rounded border strictly **inside** `r` (CSS border-box).
		 *
		 * Outer radius is `radiusPx`; inner radius shrinks by `thickness`
		 * (clamped at 0) so the border stays flush against `fillRoundedRect(r, ...)`.
		 */
		virtual void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c)
		{
			const auto m = Geometry2D::buildRoundedRectStroke(r, radiusPx, thickness, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}

		/**
		 * \brief Draw a rectangular ring with per-edge widths in a single indexed draw call.
		 */
		virtual void fillBorder(Rect r, EdgeInsets sides, Color c)
		{
			const auto m = Geometry2D::buildBorder(r, sides, c);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan());
		}
		/// @}

		/// \name Textures & Nine-Slice
		/// @{

		/** \brief Draw sub-rectangle `src` (texture pixels) of texture `t` into logical rect `dst`, multiplied by `tint`. */
		virtual void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint)
		{
			const auto m = Geometry2D::buildTexturedQuad(t, src, dst, tint);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan(), t);
		}

		/** \brief Draw texture `t` stretched into `dst` using the 9-slice insets `s` (fixed corners, stretched edges/center). */
		virtual void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint)
		{
			const auto m = Geometry2D::buildNineSlice(t, s, dst, tint);
			if (!m.empty())
				drawGeometry(m.vertexSpan(), m.indexSpan(), t);
		}

		/**
		 * \brief Allocate a dynamic RGBA8 2D texture of \p w x \p h pixels.
		 *
		 * When \p rgbaPixels is `nullptr`, the texture is initialized to transparent black.
		 * Used by `TextEngine` to manage dynamic glyph atlas pages.
		 */
		virtual TextureHandle createTexture(int /*w*/, int /*h*/, const uint8_t * /*rgbaPixels*/ = nullptr)
		{
			return {};
		}

		/**
		 * \brief Upload tightly-packed RGBA8 pixels into sub-rectangle \p region of texture \p t.
		 *
		 * Implementations must flush any pending draw batch referencing \p t before mutating texels.
		 */
		virtual void updateTexture(TextureHandle /*t*/, Rect /*region*/, const uint8_t * /*rgbaPixels*/) {}

		/** \brief Release a dynamic texture previously created by `createTexture`. */
		virtual void destroyTexture(TextureHandle /*t*/) {}

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
		 * \brief Switch the active color-blending mode.
		 *
		 * Implementations must flush any pending geometry batch before changing
		 * GPU blend state. Backends that do not support `Feature::Effects` may
		 * keep the default no-op.
		 */
		virtual void setBlendMode(BlendMode /*mode*/) {}

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

		/** \brief Return the shading language expected by `IAssetProvider::loadEffect` on this renderer. */
		virtual ShaderLanguage shaderLanguage() const { return ShaderLanguage::None; }

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

		/** \brief Optional hint setting the default rasterization pixel size used by `loadFont`. */
		virtual void setDefaultFontSize(int /*baseSize*/) {}

		/** \brief Optional hook to include an extra Unicode codepoint range `[first, last]` in subsequent font bakes. */
		virtual void addDefaultCodepointRange(uint32_t /*first*/, uint32_t /*last*/) {}

		/** \brief Optional runtime hook to load a 2D texture from disk under `name`. */
		virtual TextureHandle loadTexture(std::string_view /*name*/, std::string_view /*path*/)
		{
			return {};
		}

		/** \brief Optional runtime hook to unload a texture by logical name. */
		virtual void unloadTexture(std::string_view /*name*/) {}

		/** \brief Optional runtime hook to load a fragment (and optional vertex) shader from disk under `name`. */
		virtual EffectHandle loadEffect(std::string_view /*name*/,
										std::string_view /*fsPath*/,
										std::string_view /*vsPath*/ = {})
		{
			return {};
		}

		/**
		 * \brief Load a shader effect by stem \p fileStem under \p baseDir, resolving
		 *        the file extension from \p lang (e.g. `baseDir/blur.fs` for `GLSL330`,
		 *        `baseDir/spv/blur.frag.spv` for `SPIRV`, `baseDir/wgsl/blur.wgsl` for `WGSL`).
		 */
		EffectHandle loadEffectAsset(std::string_view name,
									 std::string_view baseDir,
									 std::string_view fileStem,
									 ShaderLanguage lang)
		{
			std::string dir(baseDir);
			if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
				dir.push_back('/');

			std::string path;
			switch (lang)
			{
			case ShaderLanguage::GLSL330:
				path = dir + std::string(fileStem) + ".fs";
				break;
			case ShaderLanguage::GLSL300ES:
				path = dir + "es300/" + std::string(fileStem) + ".fs";
				break;
			case ShaderLanguage::SPIRV:
				path = dir + "spv/" + std::string(fileStem) + ".frag.spv";
				break;
			case ShaderLanguage::HLSL:
				path = dir + "hlsl/" + std::string(fileStem) + ".hlsl";
				break;
			case ShaderLanguage::MSL:
				path = dir + "msl/" + std::string(fileStem) + ".metal";
				break;
			case ShaderLanguage::WGSL:
				path = dir + "wgsl/" + std::string(fileStem) + ".wgsl";
				break;
			case ShaderLanguage::None:
			default:
				return {};
			}
			return loadEffect(name, path);
		}

		/** \brief Optional runtime hook to unload a shader effect by logical name. */
		virtual void unloadEffect(std::string_view /*name*/) {}
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
			generation_++;
		}

		size_t liveCount() const { return entries_.size(); }

		/** \brief Monotonic generation counter incremented whenever `clear()` destroys all targets. */
		uint64_t generation() const { return generation_; }

		/** \brief Current frame counter advanced once per `trim()` call. */
		uint64_t frameIndex() const { return frameIndex_; }

	private:
		struct Entry
		{
			TargetHandle handle{};
			bool inUse{false};
			uint64_t lastUsedFrame{0};
		};
		std::vector<Entry> entries_;
		uint64_t frameIndex_{0};
		uint64_t generation_{1};
	};

	/**
	 * \struct UIContext
	 * \brief Per-application singleton: backends, frame state, and routing.
	 */
	struct UIContext
	{

		UIContext() = default;
		~UIContext();

		UIContext(const UIContext &) = delete;
		UIContext &operator=(const UIContext &) = delete;
		UIContext(UIContext &&) = delete;
		UIContext &operator=(UIContext &&) = delete;

		// ---- Backends ----
		IRenderer *renderer{nullptr};
		IPlatform *platform{nullptr};
		IAssetProvider *assets{nullptr};

		/**
		 * \brief Attach renderer, platform, and asset backends to this context.
		 * \param ownerSlot Optional pointer to an `IBackend::boundCtx_` slot kept in sync on detach.
		 */
		void attachBackend(IRenderer *r, IPlatform *p, IAssetProvider *a,
						   UIContext **ownerSlot = nullptr)
		{
			detachBackend();
			renderer = r;
			platform = p;
			assets = a;
			backendOwnerSlot_ = ownerSlot;
			if (backendOwnerSlot_)
				*backendOwnerSlot_ = this;
		}

		/**
		 * \brief Flush pooled render targets while the renderer is still valid and clear backend pointers.
		 */
		void detachBackend()
		{
			if (renderer)
			{
				textEngine.clearAtlas(*renderer);
				targetPool.clear(*renderer, &renderStats);
			}
			renderer = nullptr;
			platform = nullptr;
			assets = nullptr;
			if (backendOwnerSlot_)
			{
				*backendOwnerSlot_ = nullptr;
				backendOwnerSlot_ = nullptr;
			}
		}

		// ---- Per-frame state ----
		PointerState pointer;
		float dt{0.0f};
		double time{0.0};
		float dpiScale{1.0f};
		EdgeInsets safeArea;
		ModifierState modifiers;
		bool wheelConsumedThisFrame{false};
		std::array<bool, (size_t)MouseButton::Count> buttonConsumed{};
		Vec2 viewport{1280.0f, 720.0f}; ///< Current logical viewport dimensions.

		/** \brief Mark \p b as consumed; subsequent handlers in the same frame will skip it. */
		void consumeClick(MouseButton b) { buttonConsumed[(size_t)b] = true; }
		/** \brief `true` if \p b was already consumed this frame. */
		bool wasConsumed(MouseButton b) const { return buttonConsumed[(size_t)b]; }

		InputEvents inputEvents;
		std::weak_ptr<Layout> focusedNode;
		std::weak_ptr<Layout> pointerCapture;

		FilterRegistry filters; ///< Per-context post-processing filter definitions (preloaded with builtins).
		Theme theme; ///< Per-context stylesheet (rules, keyframes, and `:root` style).
		TextEngine textEngine; ///< Per-context FreeType/HarfBuzz shaper, glyph atlas, and layout cache.

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

		// ---- Screen-space culling clip stack across off-screen targets ----
		std::vector<Rect> cullingClipStack;

		/** \brief Active screen-space clip rect used by `Layout::draw()` for off-screen culling. */
		Rect getCullingClip() const
		{
			if (!cullingClipStack.empty())
				return cullingClipStack.front();
			return renderer ? renderer->getClipRect()
							: Rect{0.0f, 0.0f, viewport.x, viewport.y};
		}

		/** \brief Push an off-screen target while preserving the root screen-space culling clip. */
		void pushLayerTarget(TargetHandle t)
		{
			if (!renderer)
				return;
			cullingClipStack.push_back(getCullingClip());
			renderer->pushTarget(t);
			renderStats.pushTargetCalls++;
		}

		/** \brief Pop an off-screen target pushed via `pushLayerTarget`. */
		void popLayerTarget()
		{
			if (!renderer)
				return;
			renderer->popTarget();
			if (!cullingClipStack.empty())
				cullingClipStack.pop_back();
		}
		
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
			cullingClipStack.clear();
			renderStats.beginFrame();
			activePortals = std::move(framePortals);
			framePortals.clear();
			if (platform)
			{
				Vec2 newVp = platform->viewportSize();
				EdgeInsets newSafe = platform->safeArea();
				if (newVp.x != viewport.x || newVp.y != viewport.y ||
					newSafe != safeArea)
				{
					viewportGeneration++;
				}
				viewport = newVp;
				safeArea = newSafe;
				pointer = platform->pointer();
				modifiers = platform->modifiers();
				inputEvents = platform->pollInputEvents();
				dpiScale = platform->dpiScale();
				textEngine.setDpiScale(dpiScale);
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

		/** \brief Attach a root `Layout` subtree to this context and set it as the primary root. */
		void mount(Layout &root);
		void mount(const std::shared_ptr<Layout> &root)
		{
			if (root)
				mount(*root);
		}

		/** \brief Detach a previously mounted root `Layout` subtree from this context. */
		void unmount(Layout &root);
		void unmount(const std::shared_ptr<Layout> &root)
		{
			if (root)
				unmount(*root);
		}

		/** \brief Detach the currently mounted primary root, if any. */
		void unmount();

		/** \brief Return the primary root set by `mount()`, or `nullptr` if none. */
		Layout *mountedRoot() const { return primaryRoot_; }

		/** \brief Detach every root currently attached to this context. */
		void detachAllRoots();

		/** \brief Run one full update/measure/arrange frame on the mounted primary root. */
		void runFrame(float dt);

		/** \brief Render the mounted primary root (`beginFrame`, `draw`, `endFrame`). */
		void renderFrame();

		/** \brief Run one full update/measure/arrange frame on \p root using this context. */
		void runFrame(Layout &root, float dt);

		/** \brief Render \p root (`beginFrame`, `draw`, `endFrame`) using this context. */
		void renderFrame(Layout &root);

		/** \brief Clear focus. */
		void releaseFocus() { focusedNode.reset(); }
		/** \brief `true` if \p n is the current focused node. */
		bool hasFocus(const Layout *n) const
		{
			auto sp = focusedNode.lock();
			return sp && sp.get() == n;
		}
	private:
		friend class Layout;

		void registerRoot(Layout *root)
		{
			if (!root)
				return;
			if (std::find(attachedRoots_.begin(), attachedRoots_.end(), root) == attachedRoots_.end())
				attachedRoots_.push_back(root);
		}

		void unregisterRoot(Layout *root)
		{
			if (primaryRoot_ == root)
				primaryRoot_ = nullptr;
			auto it = std::find(attachedRoots_.begin(), attachedRoots_.end(), root);
			if (it != attachedRoots_.end())
				attachedRoots_.erase(it);
		}

		std::vector<Layout *> attachedRoots_;
		Layout *primaryRoot_{nullptr};
		UIContext **backendOwnerSlot_{nullptr};
	};

	/** @} */
}