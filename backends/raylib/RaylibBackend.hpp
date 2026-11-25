#pragma once

// @backend_name     Raylib
// @backend_target   ZenitUI::RaylibBackend
// @backend_role     Reference desktop GPU backend (Raylib / rlgl)
// @backend_features Effects, NestedTargets, TargetSubRect
// @backend_classes  ZenitUI::RaylibBackend ZenitUI::RaylibPlatform ZenitUI::RaylibRenderer ZenitUI::RaylibAssetProvider

/**
 * \file RaylibBackend.hpp
 * \brief Reference Raylib implementation of ZenitUI's three backend interfaces:
 *        `IPlatform`, `IRenderer`, and `IAssetProvider`.
 *
 * Use this file (together with `RaylibBackend.cpp`) as the blueprint when
 * porting ZenitUI to another graphics or windowing library (SDL, GLFW+bgfx,
 * Sokol, custom engine, etc.). Every backend consists of three independent
 * pieces wired into `UIContext`:
 *
 *  1. **Platform (`IPlatform`)** — window size, DPI scale, monotonic clock,
 *     pointer/keyboard polling, safe-area insets, and system clipboard.
 *  2. **Renderer (`IRenderer`)** — 2D primitives, textures/nine-slice, text
 *     drawing & measurement, transform/clip/shader stacks, and optional
 *     off-screen render targets for post-processing filters.
 *  3. **Asset Provider (`IAssetProvider`)** — loading, caching, name-based
 *     lookup, and unloading of fonts, textures, and shaders.
 *
 * Links against Raylib via the `ZenitUI::RaylibBackend` CMake target.
 * Advertises `Feature::Effects`, `Feature::NestedTargets`, and
 * `Feature::TargetSubRect`.
 */

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#endif

#include "UIContext.hpp"
#include "Backend.hpp"
#include "Utf8.hpp"

#include <raylib.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ZenitUI
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	namespace detail
	{
		/**
		 * \brief Transparent hasher enabling zero-allocation `std::string_view`
		 *        lookups in `std::unordered_map<std::string, ...>`.
		 */
		struct TransparentStringHash
		{
			using is_transparent = void;

			size_t operator()(std::string_view sv) const noexcept
			{
				return std::hash<std::string_view>{}(sv);
			}
			size_t operator()(const std::string &s) const noexcept
			{
				return std::hash<std::string_view>{}(s);
			}
			size_t operator()(const char *s) const noexcept
			{
				return std::hash<std::string_view>{}(s);
			}
		};
	} // namespace detail

	// =========================================================================
	// 1. Platform Adapter (IPlatform)
	// =========================================================================

	/**
	 * \class RaylibPlatform
	 * \ingroup grp_backends
	 * \brief Reference `IPlatform` backed by Raylib window, input, and clipboard APIs.
	 *
	 * Translates Raylib's physical screen coordinates into ZenitUI logical
	 * coordinates by dividing by `dpiScale()`, and maps Raylib key/mouse states
	 * into ZenitUI's per-frame `PointerState` and `InputEvents`.
	 */
	class RaylibPlatform : public IPlatform
	{
	public:
		/// \name Window & Timing
		/// @{
		Vec2 viewportSize() override;
		double time() override;
		float dpiScale() override;
		EdgeInsets safeArea() override;
		/// @}

		/// \name Pointer & Keyboard Input
		/// @{
		PointerState pointer() override;
		ModifierState modifiers() override;
		InputEvents pollInputEvents() override;
		/// @}

		/// \name System Clipboard
		/// @{
		void setClipboardText(const std::string &text) override;
		std::string getClipboardText() override;
		/// @}
	};

	// =========================================================================
	// 2. Renderer Adapter (IRenderer)
	// =========================================================================

	/**
	 * \class RaylibRenderer
	 * \ingroup grp_backends
	 * \brief Reference `IRenderer` backed by Raylib / `rlgl`.
	 *
	 * Owns the mapping from opaque ZenitUI handles (`TextureHandle`,
	 * `FontHandle`, `EffectHandle`, `TargetHandle`) to native Raylib GPU
	 * objects, and maintains software stacks for transforms, scissor clips,
	 * active shaders, and nested off-screen render targets.
	 *
	 * \note Must be constructed after Raylib's `InitWindow()` and destroyed
	 *       (or have its resources unloaded) before `CloseWindow()`.
	 */
	class RaylibRenderer : public IRenderer
	{
	public:
		/** \brief Construct the renderer. Call after `InitWindow()`. */
		RaylibRenderer();

		/**
		 * \brief Destroy the renderer and release its handle table.
		 * \note Does not force-unload GPU handles on destruction because
		 *       Raylib's OpenGL context may already be closed when stack
		 *       unwinding reaches the destructor.
		 */
		~RaylibRenderer() override;

		RaylibRenderer(const RaylibRenderer &) = delete;
		RaylibRenderer &operator=(const RaylibRenderer &) = delete;

		/// \name Frame Lifecycle & DPI
		/// @{
		void beginFrame() override;
		void endFrame() override;
		void setDpiScale(float scale) override;
		/// @}

		/// \name Capabilities
		/// @{
		/** \brief Reports support for `Effects`, `NestedTargets`, and `TargetSubRect`. */
		bool supports(Feature f) const override
		{
			return f == Feature::Effects ||
				   f == Feature::NestedTargets ||
				   f == Feature::TargetSubRect;
		}

		/** \brief Reports support for `GLSL330` shader language. */
		ShaderLanguage shaderLanguage() const override { return ShaderLanguage::GLSL330; }

		/// @}

		/// \name 2D Vector Primitives
		/// @{
		void fillRect(Rect r, Color c) override;
		void fillRoundedRect(Rect r, float radiusPx, Color c) override;
		void fillCircle(Vec2 center, float radius, Color c) override;
		void strokeRect(Rect r, float thickness, Color c) override;
		void drawGeometry(std::span<const Vertex2D> vertices,
                  std::span<const uint32_t> indices,
                  TextureHandle tex = {}) override;
		/// @}

		/// \name Textures & Nine-Slice
		/// @{
		void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) override;
		void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) override;
		/// @}

		/// \name Text Rendering & Metrics
		/// @{
		void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) override;
		Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) override;
		Vec2 measureCodepoint(FontHandle f, uint32_t codepoint, float size, float spacing) override;
		/// @}

		/// \name Transform, Clip & Shader Stacks
		/// @{
		void pushTransform(const Transform2D &t) override;
		void popTransform() override;
		void pushClip(Rect r) override;
		void popClip() override;
		Rect getClipRect() const override;
		void pushEffect(EffectHandle e) override;
		void popEffect() override;
		/// @}

		/// \name Shader Uniforms
		/// @{
		void setEffectFloat(EffectHandle e, const char *name, float value) override;
		void setEffectVec2(EffectHandle e, const char *name, Vec2 value) override;
		void setEffectVec3(EffectHandle e, const char *name, Vec3 value) override;
		void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) override;
		void setEffectVec4(EffectHandle e, const char *name, Color value) override;
		/// @}

		/// \name Off-Screen Render Targets
		/// @{
		TargetHandle createTarget(int w, int h) override;
		void destroyTarget(TargetHandle t) override;
		void pushTarget(TargetHandle t) override;
		void popTarget() override;
		void drawTarget(TargetHandle t, Rect dst, Color tint) override;
		void drawTarget(TargetHandle t, Rect src, Rect dst, Color tint) override;
		bool inTarget() const override;
		void clearTarget(TargetHandle t, Color c) override;
		/// @}

		/// \name Native Raylib Resource Registration
		/// @{
		/** \brief Register an existing Raylib `::Texture2D` and return its ZenitUI handle. */
		TextureHandle registerTexture(const ::Texture2D &t);
		/** \brief Register an existing Raylib `::Font` and return its ZenitUI handle. */
		FontHandle registerFont(const ::Font &f);
		/** \brief Register an existing Raylib `::Shader` and return its ZenitUI handle. */
		EffectHandle registerEffect(const ::Shader &s);

		/** \brief Unload and unregister a previously registered font handle. */
		void unregisterFont(FontHandle h);
		/** \brief Unload and unregister a previously registered texture handle. */
		void unregisterTexture(TextureHandle h);
		/** \brief Unload and unregister a previously registered shader handle. */
		void unregisterEffect(EffectHandle h);
		/// @}

		/** \brief Transform a local-space clip rect into screen space via the active transform stack. */
		Rect transformClipToScreen(const Rect &local) const;

	private:
		struct Resources;
		struct TargetState
		{
			TargetHandle target{};
			bool clipActive{false};
			Rect currentClip{0, 0, 0, 0};
			std::vector<Rect> clipStack;
			std::vector<Transform2D> transformStack;
			int targetW{0};
			int targetH{0};
		};

		void applyScissor(const Rect &clip);
		void suspendRenderState();
		void restoreRenderState();

		std::unique_ptr<Resources> res_;

		std::vector<TargetState> targetStates_;
		std::vector<Rect> clipStack_;
		std::vector<Transform2D> transformStack_;
		std::vector<EffectHandle> effectStack_;

		bool clipActive_{false};
		Rect currentClip_{0, 0, 0, 0};
		float dpiScale_{1.0f};

		bool insideTarget_{false};
		TargetHandle currentTarget_{};
		int currentTargetW_{0};
		int currentTargetH_{0};
	};

	// =========================================================================
	// 3. Asset Provider Adapter (IAssetProvider)
	// =========================================================================

	/**
	 * \class RaylibAssetProvider
	 * \ingroup grp_backends
	 * \brief Reference `IAssetProvider` for loading and caching Raylib fonts,
	 *        textures, and shaders by logical name.
	 *
	 * Uses `CodepointSet::defaultLatin()` by default when baking TTF/OTF font
	 * atlases via `LoadFontEx`. Customize `defaultCodepoints()` before loading
	 * fonts, or pass a per-font `CodepointSet` overload.
	 */
	class RaylibAssetProvider : public IAssetProvider
	{
	public:
		using IAssetProvider::getFont;
		using IAssetProvider::loadFont;
		using IAssetProvider::loadFontFromMemory;
		using IAssetProvider::unloadFont;
		using IAssetProvider::loadTexture; 
		using IAssetProvider::loadEffect; 
		using IAssetProvider::unloadTexture; 
		using IAssetProvider::unloadEffect;

		/** \brief Construct an asset provider bound to \p renderer for GPU handle registration. */
		explicit RaylibAssetProvider(RaylibRenderer &renderer) : renderer_(renderer) {}

		/// \name Font Codepoint Configuration
		/// @{
		/** \brief Replace the default codepoint set used by subsequent font loads. */
		void setDefaultCodepoints(CodepointSet set) { defaultCodepoints_ = std::move(set); }
		/** \brief Mutable access to the default codepoint set (e.g. to merge extra ranges). */
		CodepointSet &defaultCodepoints() { return defaultCodepoints_; }
		/** \brief Read-only access to the default codepoint set. */
		const CodepointSet &defaultCodepoints() const { return defaultCodepoints_; }
		/// @}

		/// \name Raylib Convenience Loaders
		/// @{
		/** \brief Load a font from disk using `defaultCodepoints()`. */
		bool loadFont(std::string name, const char *path, int baseSize = 96);
		/** \brief Load a font from disk using an explicit \p codepoints set. */
		bool loadFont(std::string name, const char *path, int baseSize, const CodepointSet &codepoints);
		/** \brief Load a 2D texture from disk and register it under \p name. */
		bool loadTexture(std::string name, const char *path);
		/** \brief Load a fragment (and optional vertex) shader from disk under \p name. */
		bool loadEffect(std::string name, const char *fsPath, const char *vsPath = nullptr);
		/** \brief Load a font from memory with an explicit \p baseSize and \p codepoints set. */
		FontHandle loadFontFromMemory(std::string_view name,
									  const void *data, size_t size,
									  int baseSize, const CodepointSet &codepoints);
		/// @}

		/// \name IAssetProvider Overrides
		/// @{
		FontHandle loadFont(std::string_view name, std::string_view path) override;
		FontHandle loadFontFromMemory(std::string_view name, const void *data, size_t size) override;
		void unloadFont(std::string_view name) override;

		FontHandle getFont(std::string_view name) override
		{
			auto it = fonts_.find(name);
			return it != fonts_.end() ? it->second : FontHandle{};
		}

		TextureHandle getTexture(std::string_view name) override
		{
			auto it = textures_.find(name);
			return it != textures_.end() ? it->second : TextureHandle{};
		}

		EffectHandle getEffect(std::string_view name) override
		{
			auto it = effects_.find(name);
			return it != effects_.end() ? it->second : EffectHandle{};
		}

		void setDefaultFontSize(int baseSize) override
	{
		if (baseSize > 0)
			defaultFontSize_ = baseSize;
	}

	void addDefaultCodepointRange(uint32_t first, uint32_t last) override
	{
		defaultCodepoints_.addRange(first, last);
	}

	TextureHandle loadTexture(std::string_view name, std::string_view path) override;
	void unloadTexture(std::string_view name) override;
	EffectHandle loadEffect(std::string_view name,
							std::string_view fsPath,
							std::string_view vsPath = {}) override;
	void unloadEffect(std::string_view name) override;

		/// @}

	private:
		template <typename Handle>
		using HandleMap = std::unordered_map<std::string, Handle, detail::TransparentStringHash, std::equal_to<>>;

		FontHandle bakeAndRegisterFont(const std::string &name,
									   const char *filePath,
									   int baseSize,
									   const CodepointSet &codepoints);

		RaylibRenderer &renderer_;
		HandleMap<FontHandle> fonts_;
		HandleMap<TextureHandle> textures_;
		HandleMap<EffectHandle> effects_;
		CodepointSet defaultCodepoints_{CodepointSet::defaultLatin()};
		int defaultFontSize_{96};
	};

	// =========================================================================
	// 4. Unified Host Adapter (IBackend)
	// =========================================================================

	/**
	 * \class RaylibBackend
	 * \ingroup grp_backends
	 * \brief Unified `IBackend` host wrapping Raylib windowing, rendering, and assets.
	 */
	class RaylibBackend : public IBackend
	{
	public:
		RaylibBackend() = default;
		~RaylibBackend() override { shutdown(); }

		RaylibBackend(const RaylibBackend &) = delete;
		RaylibBackend &operator=(const RaylibBackend &) = delete;

		bool init(const WindowConfig &config) override;
		void shutdown() override;
		bool pollFrame() override;
		float deltaTime() const override;
		void beginDraw(Color clearColor) override;
		void endDraw() override;
		std::string_view name() const override { return "Raylib"; }

		IPlatform &platform() override { return *platform_; }
		IRenderer &renderer() override { return *renderer_; }
		IAssetProvider &assets() override { return *assets_; }

	private:
		std::unique_ptr<RaylibPlatform> platform_;
		std::unique_ptr<RaylibRenderer> renderer_;
		std::unique_ptr<RaylibAssetProvider> assets_;
		bool initialized_{false};
	};

	/** @} */
} // namespace ZenitUI