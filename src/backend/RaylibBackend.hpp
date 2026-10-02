#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "UIContext.hpp"

namespace ZenitUI {

	class RaylibPlatform : public IPlatform {
	public:
		Vec2 viewportSize() override;
		PointerState pointer() override;
		double time() override;
		bool shiftHeld() override;
		InputEvents pollInputEvents() override;
	};

	class RaylibRenderer : public IRenderer {
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

		void pushTransform(const Transform2D& t) override;
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

		TextureHandle registerTexture(unsigned int raylibTexId, int w, int h);
		FontHandle registerFont(unsigned int raylibFontId);
		EffectHandle registerEffect(unsigned int raylibShaderId);

		Rect transformClipToScreen(const Rect& local) const;
	private:
		std::vector<Rect> clipStack;
		std::vector<Transform2D> transformStack;
		bool clipActive{ false };
		Rect currentClip{ 0,0,0,0 };
	};
}