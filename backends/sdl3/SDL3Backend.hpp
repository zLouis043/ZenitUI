#pragma once

// @backend_name     SDL3 + OpenGL 3.3
// @backend_target   ZenitUI::SDL3Backend
// @backend_role     Desktop GPU backend with HiDPI (SDL3 + batched GL 3.3 Core)
// @backend_features Effects, NestedTargets, TargetSubRect
// @backend_classes  ZenitUI::SDL3Backend ZenitUI::SDL3Platform ZenitUI::SDL3Renderer ZenitUI::SDL3AssetProvider

/**
 * \file SDL3Backend.hpp
 * \brief Reference SDL3 + OpenGL 3.3 Core implementation of `IPlatform`,
 *        `IRenderer`, `IAssetProvider`, and `IBackend`.
 *
 * Links against SDL3 and stb (fetched via CMake `FetchContent`) through the
 * `ZenitUI::SDL3Backend` CMake target. Provides full feature parity with
 * `RaylibBackend`:
 *
 *  - `Feature::Effects` — GLSL 330 fragment shaders compatible with the `.fs` files in `assets/shaders/`.
 *  - `Feature::NestedTargets` — off-screen color-only RGBA FBOs.
 *  - `Feature::TargetSubRect` — sub-rectangle blits for `TargetPool`.
 *  - HiDPI scaling via `SDL_GetWindowPixelDensity`.
 *  - Zero extra event-pumping boilerplate on `IPlatform` (`SDL3Platform` hooks
 *    into SDL3's event queue transparently via `SDL_AddEventWatch`).
 */

#include "Backend.hpp"
#include "UIContext.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct SDL_Window;
struct SDL_GLContextState;
using SDL_GLContext = SDL_GLContextState *;

namespace ZenitUI
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	namespace detail
	{
		struct SdlTransparentStringHash
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
	 * \class SDL3Platform
	 * \ingroup grp_backends
	 * \brief Reference `IPlatform` backed by SDL3 window, input, and clipboard APIs.
	 *
	 * Registers an internal `SDL_AddEventWatch` listener on construction so
	 * edge-triggered mouse, wheel, keyboard, and UTF-8 text events are captured
	 * automatically whenever `SDL_PollEvent` runs — keeping the `IPlatform`
	 * interface strictly identical to polling backends like `RaylibPlatform`.
	 */
	class SDL3Platform : public IPlatform
	{
	public:
		/** \brief Construct the platform adapter bound to \p window (must be created after `SDL_Init`). */
		explicit SDL3Platform(SDL_Window *window);
		~SDL3Platform() override;

		SDL3Platform(const SDL3Platform &) = delete;
		SDL3Platform &operator=(const SDL3Platform &) = delete;

		/// \name IPlatform Overrides
		/// @{
		Vec2 viewportSize() override;
		PointerState pointer() override;
		double time() override;
		ModifierState modifiers() override;
		InputEvents pollInputEvents() override;
		float dpiScale() override;
		EdgeInsets safeArea() override;
		void setClipboardText(const std::string &text) override;
		std::string getClipboardText() override;
		/// @}

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	// =========================================================================
	// 2. Renderer Adapter (IRenderer)
	// =========================================================================

	/**
	 * \class SDL3Renderer
	 * \ingroup grp_backends
	 * \brief Reference `IRenderer` backed by a batched OpenGL 3.3 Core 2D pipeline.
	 *
	 * Uses the same GLSL 330 fragment-shader interface (`fragTexCoord`,
	 * `fragColor`, `texture0`, `colDiffuse`, `finalColor`) as `RaylibRenderer`
	 * so shader assets in `assets/shaders/` run unmodified on both backends.
	 *
	 * \note Must be constructed with an active OpenGL 3.3 Core context on \p window
	 *       and destroyed before `SDL_GL_DestroyContext`.
	 */
	class SDL3Renderer : public IRenderer
	{
	public:
		/** \brief Initialize OpenGL 3.3 function pointers, default shader, and vertex batch. */
		explicit SDL3Renderer(SDL_Window *window);
		~SDL3Renderer() override;

		SDL3Renderer(const SDL3Renderer &) = delete;
		SDL3Renderer &operator=(const SDL3Renderer &) = delete;

		/** \brief Returns `true` if OpenGL 3.3 entry points and core shaders initialized cleanly. */
		bool isReady() const;

		/// \name Frame Lifecycle & Capabilities
		/// @{
		void beginFrame() override;
		void endFrame() override;
		void setDpiScale(float scale) override;

		bool supports(Feature f) const override
		{
			return f == Feature::Effects ||
				   f == Feature::NestedTargets ||
				   f == Feature::TargetSubRect;
		}

		ShaderLanguage shaderLanguage() const override { return ShaderLanguage::GLSL330; }

		/// @}

		/// \name 2D Vector Primitives
		/// @{
		void drawGeometry(std::span<const Vertex2D> vertices,
						  std::span<const uint32_t> indices,
						  TextureHandle tex = {}) override;
		TextureHandle createTexture(int w, int h, const uint8_t *rgbaPixels = nullptr) override;
		void updateTexture(TextureHandle t, Rect region, const uint8_t *rgbaPixels) override;
		void destroyTexture(TextureHandle h) override;
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
		void setBlendMode(BlendMode mode) override;
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

		/// \name Resource Creation Helpers (used by `SDL3AssetProvider`)
		/// @{
		TextureHandle createTextureRGBA(int width, int height, const unsigned char *rgbaPixels);
		EffectHandle createEffectFromSource(const char *fsSource, const char *vsSource = nullptr);
		void destroyEffect(EffectHandle h);
		/// @}

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	// =========================================================================
	// 3. Asset Provider Adapter (IAssetProvider)
	// =========================================================================

	/**
	 * \class SDL3AssetProvider
	 * \ingroup grp_backends
	 * \brief Reference `IAssetProvider` loading textures (`stb_image`) and
	 *        GLSL 330 shaders into `SDL3Renderer`.
	 */
	class SDL3AssetProvider : public IAssetProvider
	{
	public:
		explicit SDL3AssetProvider(SDL3Renderer &renderer) : renderer_(renderer) {}
		~SDL3AssetProvider() override;

		SDL3AssetProvider(const SDL3AssetProvider &) = delete;
		SDL3AssetProvider &operator=(const SDL3AssetProvider &) = delete;

		/// \name IAssetProvider Overrides
		/// @{
		TextureHandle loadTexture(std::string_view name, std::string_view path) override;
		void unloadTexture(std::string_view name) override;

		EffectHandle loadEffect(std::string_view name,
								std::string_view fsPath,
								std::string_view vsPath = {}) override;
		void unloadEffect(std::string_view name) override;

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
		/// @}

	private:
		template <typename Handle>
		using HandleMap = std::unordered_map<std::string, Handle, detail::SdlTransparentStringHash, std::equal_to<>>;

		SDL3Renderer &renderer_;
		HandleMap<TextureHandle> textures_;
		HandleMap<EffectHandle> effects_;
	};

	// =========================================================================
	// 4. Unified Host Adapter (IBackend)
	// =========================================================================

	/**
	 * \class SDL3Backend
	 * \ingroup grp_backends
	 * \brief Unified `IBackend` host managing an SDL3 window, OpenGL 3.3 Core
	 *        context, and `SDL3Platform` / `SDL3Renderer` / `SDL3AssetProvider`.
	 */
	class SDL3Backend : public IBackend
	{
	public:
		SDL3Backend() = default;
		~SDL3Backend() override { shutdown(); }

		SDL3Backend(const SDL3Backend &) = delete;
		SDL3Backend &operator=(const SDL3Backend &) = delete;

		bool init(const WindowConfig &config) override;
		void shutdown() override;
		bool pollFrame() override;
		float deltaTime() const override { return deltaTime_; }
		void beginDraw(Color clearColor) override;
		void endDraw() override;
		std::string_view name() const override { return "SDL3 + OpenGL 3.3"; }

		IPlatform &platform() override { return *platform_; }
		IRenderer &renderer() override { return *renderer_; }
		IAssetProvider &assets() override { return *assets_; }

	private:
		SDL_Window *window_{nullptr};
		SDL_GLContext glContext_{nullptr};
		std::unique_ptr<SDL3Platform> platform_;
		std::unique_ptr<SDL3Renderer> renderer_;
		std::unique_ptr<SDL3AssetProvider> assets_;
		uint64_t lastTicksNs_{0};
		float deltaTime_{1.0f / 60.0f};
		bool running_{false};
		bool initialized_{false};
	};

	/** @} */
} // namespace ZenitUI