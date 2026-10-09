#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "UIContext.hpp"
#include <raylib.h>
#include <unordered_map>
#include <string>

namespace ZenitUI
{

	class RaylibPlatform : public IPlatform
	{
	public:
		Vec2 viewportSize() override;
		PointerState pointer() override;
		double time() override;
		bool shiftHeld() override;
		InputEvents pollInputEvents() override;
		float dpiScale() override;
		EdgeInsets safeArea() override;
	};

	class RaylibRenderer : public IRenderer
	{
	public:
		RaylibRenderer();
		~RaylibRenderer();

		void fillRect(Rect r, Color c) override;
		void fillRoundedRect(Rect r, float radiusPx, Color c) override;
		void fillCircle(Vec2 center, float radius, Color c) override;

		void strokeRect(Rect r, float thickness, Color c) override;
		void strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) override;

		void drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) override;
		void drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) override;

		void drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) override;
		Vec2 measureText(FontHandle f, std::string_view s, float size, float spacing) override;

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

		bool supports(Feature f) const override { return f == Feature::Effects; }

		TextureHandle registerTexture(const ::Texture2D &t);
		FontHandle registerFont(const ::Font &f);
		EffectHandle registerEffect(const ::Shader &s);

		void setEffectFloat(EffectHandle e, const char *name, float value) override;
		void setEffectVec2(EffectHandle e, const char *name, Vec2 value) override;
		void setEffectVec3(EffectHandle e, const char *name, Vec3 value) override;
		void setEffectVec4f(EffectHandle e, const char *name, Vec4 value) override;
		void setEffectVec4(EffectHandle e, const char *name, Color value) override;
		bool inTarget() const override;
		void clearTarget(TargetHandle t, Color c) override;

		Rect transformClipToScreen(const Rect &local) const;

		void beginFrame() override;
		void endFrame() override;
		void setDpiScale(float scale) override;

	private:
		std::vector<Rect> clipStack;
		std::vector<Transform2D> transformStack;
		std::vector<EffectHandle> effectStack_;
		bool clipActive{false};
		Rect currentClip{0, 0, 0, 0};
		float dpiScale_{1.0f};
		bool insideTarget_{false};
	};

	// RaylibBackend.hpp
	class RaylibAssetProvider : public IAssetProvider
	{
	public:
		explicit RaylibAssetProvider(RaylibRenderer &r) : renderer(r) {}

		bool loadFont(std::string name, const char *path, int baseSize = 96)
		{
			::Font f = LoadFontEx(path, baseSize, nullptr, 0);
			if (f.texture.id == 0)
				return false;
			SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
			fonts[std::move(name)] = renderer.registerFont(f);
			return true;
		}

		bool loadTexture(std::string name, const char *path)
		{
			::Texture2D t = LoadTexture(path);
			if (t.id == 0)
				return false;
			textures[std::move(name)] = renderer.registerTexture(t);
			return true;
		}

		bool loadEffect(std::string name, const char *fsPath, const char *vsPath = nullptr)
		{
			::Shader s = LoadShader(vsPath, fsPath);
			if (s.id == 0)
				return false;
			effects[std::move(name)] = renderer.registerEffect(s);
			return true;
		}

		FontHandle getFont(std::string_view name) override
		{
			auto it = fonts.find(std::string(name));
			return it != fonts.end() ? it->second : FontHandle{};
		}
		TextureHandle getTexture(std::string_view name) override
		{
			auto it = textures.find(std::string(name));
			return it != textures.end() ? it->second : TextureHandle{};
		}
		EffectHandle getEffect(std::string_view name) override
		{
			auto it = effects.find(std::string(name));
			return it != effects.end() ? it->second : EffectHandle{};
		}

	private:
		RaylibRenderer &renderer;
		std::unordered_map<std::string, FontHandle> fonts;
		std::unordered_map<std::string, TextureHandle> textures;
		std::unordered_map<std::string, EffectHandle> effects;
	};
}