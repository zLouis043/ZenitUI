#pragma once

/**
 * \file RaylibBackend.hpp
 * \brief Reference Raylib implementation of `IPlatform`, `IRenderer`, and `IAssetProvider`.
 *
 * Links against Raylib via the `ZenitUI::RaylibBackend` CMake target.
 * Supports shaders (`Feature::Effects`), nested render targets
 * (`Feature::NestedTargets`), sub-rect target blits (`Feature::TargetSubRect`),
 * per-glyph `measureCodepoint`, and configurable `CodepointSet` font-atlas
 * baking.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "UIContext.hpp"
#include "Utf8.hpp"
#include <raylib.h>
#include <unordered_map>
#include <string>
#include <functional>
#include <string_view>

namespace ZenitUI
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	namespace detail
	{
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
	}

	/**
	 * \class RaylibPlatform
	 * \ingroup grp_backends
	 * \brief Raylib implementation of `IPlatform` (window metrics, pointer, keyboard, clipboard).
	 */
	class RaylibPlatform : public IPlatform
	{
	public:
		Vec2 viewportSize() override;
		PointerState pointer() override;
		double time() override;
		ModifierState modifiers() override;
		InputEvents pollInputEvents() override;
		float dpiScale() override;
		EdgeInsets safeArea() override;
		void setClipboardText(const std::string &text) override;
		std::string getClipboardText() override;
	};

	/**
	 * \class RaylibRenderer
	 * \ingroup grp_backends
	 * \brief Raylib implementation of `IRenderer` with transform/clip/target/shader stacks.
	 */
	class RaylibRenderer : public IRenderer
	{
	public:
		/** \brief Construct the renderer. Must be used after Raylib's `InitWindow`. */
		RaylibRenderer();
		/** \brief Destroy the renderer. */
		~RaylibRenderer();

		/// \name IRenderer overrides
		/// @{
		void fillRect(Rect r, Color c) override;
		void fillRoundedRect(Rect r, float radiusPx, Color c) override;
		void fillCircle(Vec2 center, float radius, Color c) override;

		void strokeRect(Rect r, float thickness, Color c) override;
		void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) override;

		void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) override;
		void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) override;

		void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) override;
		Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) override;
		Vec2 measureCodepoint(FontHandle f, uint32_t codepoint, float size, float spacing) override;

		void pushTransform(const Transform2D &t) override;
		void popTransform() override;
		void pushEffect(EffectHandle e) override;
		void popEffect() override;
		void pushClip(Rect r) override;
		void popClip() override;

		Rect getClipRect() const override;

		TargetHandle createTarget(int w, int h) override;
		void destroyTarget(TargetHandle t) override;
		void pushTarget(TargetHandle t) override;
		void popTarget() override;
		void drawTarget(TargetHandle t, Rect dst, Color tint) override;
		void drawTarget(TargetHandle t, Rect src, Rect dst, Color tint) override;

		/** \brief Reports support for `Effects`, `NestedTargets`, and `TargetSubRect`. */
		bool supports(Feature f) const override
		{
			return f == Feature::Effects ||
				   f == Feature::NestedTargets ||
				   f == Feature::TargetSubRect;
		}

		void setEffectFloat(EffectHandle e, const char *name, float value) override;
		void setEffectVec2(EffectHandle e, const char *name, Vec2 value) override;
		void setEffectVec3(EffectHandle e, const char *name, Vec3 value) override;
		void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) override;
		void setEffectVec4(EffectHandle e, const char *name, Color value) override;
		bool inTarget() const override;
		void clearTarget(TargetHandle t, Color c) override;

		void beginFrame() override;
		void endFrame() override;
		void setDpiScale(float scale) override;
		/// @}

		/// \name Native Raylib resource registration
		/// @{
		/** \brief Register an existing Raylib `::Texture2D` and return its handle. */
		TextureHandle registerTexture(const ::Texture2D &t);
		/** \brief Register an existing Raylib `::Font` and return its handle. */
		FontHandle registerFont(const ::Font &f);
		/** \brief Register an existing Raylib `::Shader` and return its handle. */
		EffectHandle registerEffect(const ::Shader &s);

		/** \brief Unload and unregister a previously registered font handle. */
		void unregisterFont(FontHandle h);
		/** \brief Unload and unregister a previously registered texture handle. */
		void unregisterTexture(TextureHandle h);
		/** \brief Unload and unregister a previously registered shader handle. */
		void unregisterEffect(EffectHandle h);
		/// @}

		/** \brief Transform a local-space clip rectangle into screen space using the current transform stack. */
		Rect transformClipToScreen(const Rect &local) const;

	private:
		void applyScissor(const Rect &clip);

		struct TargetState
		{
			TargetHandle target{};
			bool clipActive;
			Rect currentClip;
			std::vector<Rect> clipStack;
			std::vector<Transform2D> transformStack;
			int targetW{0};
			int targetH{0};
		};
		std::vector<TargetState> targetStates_;

		std::vector<Rect> clipStack;
		std::vector<Transform2D> transformStack;
		std::vector<EffectHandle> effectStack_;
		bool clipActive{false};
		Rect currentClip{0, 0, 0, 0};
		float dpiScale_{1.0f};
		bool insideTarget_{false};
		TargetHandle currentTarget_{};
		int currentTargetW_{0};
		int currentTargetH_{0};
	};

	/**
	 * \class RaylibAssetProvider
	 * \ingroup grp_backends
	 * \brief Raylib implementation of `IAssetProvider` for fonts, textures, and shaders.
	 *
	 * Uses `CodepointSet::defaultLatin()` by default when baking TTF/OTF font
	 * atlases via `LoadFontEx`. Customize `defaultCodepoints()` before calling
	 * `loadFont`, or pass a per-font `CodepointSet` overload.
	 */
	class RaylibAssetProvider : public IAssetProvider
	{
	public:
		using IAssetProvider::getFont;
		using IAssetProvider::loadFont;
		using IAssetProvider::loadFontFromMemory;
		using IAssetProvider::unloadFont;

		/** \brief Construct an asset provider bound to \p r for GPU resource registration. */
		explicit RaylibAssetProvider(RaylibRenderer &r) : renderer(r) {}

		/** \brief Replace the default codepoint set used by subsequent font loads. */
		void setDefaultCodepoints(CodepointSet set) { defaultCodepoints_ = std::move(set); }

		/** \brief Mutable access to the default codepoint set (e.g. to merge extra ranges or symbols). */
		CodepointSet &defaultCodepoints() { return defaultCodepoints_; }

		/** \brief Read-only access to the default codepoint set. */
		const CodepointSet &defaultCodepoints() const { return defaultCodepoints_; }

		/** \brief Load a font from disk using `defaultCodepoints()`. */
		bool loadFont(std::string name, const char *path, int baseSize = 96);

		/** \brief Load a font from disk using an explicit \p codepoints set. */
		bool loadFont(std::string name, const char *path, int baseSize, const CodepointSet &codepoints);

		/** \brief Load a 2D texture from disk and register it under \p name. */
		bool loadTexture(std::string name, const char *path);

		/** \brief Load a fragment (and optional vertex) shader from disk under \p name. */
		bool loadEffect(std::string name, const char *fsPath, const char *vsPath = nullptr);

		/** \brief Load a font from disk (`IAssetProvider` override, default size 96 px). */
		FontHandle loadFont(std::string_view name, std::string_view path) override;

		/** \brief Load a font from an in-memory buffer (`IAssetProvider` override, default size 96 px). */
		FontHandle loadFontFromMemory(std::string_view name,
									  const void *data, size_t size) override;

		/** \brief Load a font from an in-memory buffer with an explicit \p baseSize and \p codepoints set. */
		FontHandle loadFontFromMemory(std::string_view name,
									  const void *data, size_t size,
									  int baseSize, const CodepointSet &codepoints);

		/** \brief Unload a previously loaded font by name. */
		void unloadFont(std::string_view name) override;

		/** \brief Look up a loaded font by name (zero-allocation transparent lookup). */
		FontHandle getFont(std::string_view name) override
		{
			auto it = fonts.find(name);
			return it != fonts.end() ? it->second : FontHandle{};
		}

		/** \brief Look up a loaded texture by name. */
		TextureHandle getTexture(std::string_view name) override
		{
			auto it = textures.find(name);
			return it != textures.end() ? it->second : TextureHandle{};
		}

		/** \brief Look up a loaded shader effect by name. */
		EffectHandle getEffect(std::string_view name) override
		{
			auto it = effects.find(name);
			return it != effects.end() ? it->second : EffectHandle{};
		}

	private:
		RaylibRenderer &renderer;
		std::unordered_map<std::string, FontHandle, detail::TransparentStringHash, std::equal_to<>> fonts;
		std::unordered_map<std::string, TextureHandle, detail::TransparentStringHash, std::equal_to<>> textures;
		std::unordered_map<std::string, EffectHandle, detail::TransparentStringHash, std::equal_to<>> effects;
		CodepointSet defaultCodepoints_{CodepointSet::defaultLatin()};
	};

	/** @} */
}