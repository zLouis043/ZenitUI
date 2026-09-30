#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "RaylibBackend.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <string>
#include <unordered_map>

namespace ZenitUI {

	static ::Color rl(const Color& c) { return { c.r, c.g, c.b, c.a }; }
	static ::Vector2 rl(const Vec2& v) { return { v.x, v.y }; }
	static ::Rectangle rl(const Rect& r) { return { r.x, r.y, r.width, r.height }; }

	Vec2 RaylibPlatform::viewportSize() { return { (float)GetScreenWidth(), (float)GetScreenHeight() }; }
	PointerState RaylibPlatform::pointer() {
		auto mp = GetMousePosition();
		return { {mp.x, mp.y}, IsMouseButtonDown(MOUSE_BUTTON_LEFT), IsMouseButtonPressed(MOUSE_BUTTON_LEFT), IsMouseButtonReleased(MOUSE_BUTTON_LEFT) };
	}
	double RaylibPlatform::time() { return GetTime(); }

	struct RaylibResources {
		std::unordered_map<uint32_t, ::RenderTexture2D> targets;
		std::unordered_map<uint32_t, ::Texture2D> textures;
		std::unordered_map<uint32_t, ::Font> fonts;
		std::unordered_map<uint32_t, ::Shader> shaders;
		uint32_t nextId{ 1 };
	};
	static RaylibResources res;

	RaylibRenderer::RaylibRenderer() {}
	RaylibRenderer::~RaylibRenderer() {}

	void RaylibRenderer::fillRect(Rect r, Color c) { DrawRectangleRec(rl(r), rl(c)); }
	void RaylibRenderer::fillRoundedRect(Rect r, float radiusPx, Color c) {
		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR > 0) DrawRectangleRounded(rl(r), std::clamp(radiusPx / maxR, 0.0f, 1.0f), 8, rl(c));
	}
	void RaylibRenderer::fillCircle(Vec2 center, float radius, Color c) { DrawCircleV(rl(center), radius, rl(c)); }

	void RaylibRenderer::drawTexture(TextureHandle t, Rect src, Rect dst, Color tint) {
		if (res.textures.count(t.id)) DrawTexturePro(res.textures[t.id], rl(src), rl(dst), {0,0}, 0.0f, rl(tint));
	}
	
	void RaylibRenderer::drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint) {
		if (res.textures.count(t.id)) {
			::NPatchInfo np = { {0,0,(float)t.width,(float)t.height}, s.left, s.top, s.right, s.bottom, NPATCH_NINE_PATCH };
			DrawTextureNPatch(res.textures[t.id], np, rl(dst), {0,0}, 0.0f, rl(tint));
		}
	}

	void RaylibRenderer::drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c) {
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		std::string str(s);
		DrawTextEx(font, str.c_str(), rl(pos), size, spacing, rl(c));
	}

	Vec2 RaylibRenderer::measureText(FontHandle f, std::string_view s, float size, float spacing) {
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		std::string str(s);
		auto v = MeasureTextEx(font, str.c_str(), size, spacing);
		return { v.x, v.y };
	}

	void RaylibRenderer::pushTransform(const Transform2D& t) {
		rlPushMatrix();
		rlTranslatef(t.pivot.x + t.translate.x, t.pivot.y + t.translate.y, 0);
		rlRotatef(t.rotationDeg, 0, 0, 1);
		rlScalef(t.scale, t.scale, 1);
		rlTranslatef(-t.pivot.x, -t.pivot.y, 0);
	}
	void RaylibRenderer::popTransform() { rlPopMatrix(); }

	void RaylibRenderer::pushEffect(EffectHandle e) {
		if (res.shaders.count(e.id)) BeginShaderMode(res.shaders[e.id]);
	}
	void RaylibRenderer::popEffect() { EndShaderMode(); }

	TargetHandle RaylibRenderer::createTarget(int w, int h) {
		uint32_t id = res.nextId++;
		res.targets[id] = LoadRenderTexture(w, h);
		return { id, w, h };
	}
	void RaylibRenderer::destroyTarget(TargetHandle t) {
		if (res.targets.count(t.id)) {
			UnloadRenderTexture(res.targets[t.id]);
			res.targets.erase(t.id);
		}
	}
	void RaylibRenderer::pushTarget(TargetHandle t) {
		if (res.targets.count(t.id)) { BeginTextureMode(res.targets[t.id]); ClearBackground(::BLANK); }
	}
	void RaylibRenderer::popTarget() { EndTextureMode(); }
	
	void RaylibRenderer::drawTarget(TargetHandle t, Rect dst, Color tint) {
		if (res.targets.count(t.id)) {
			::Texture2D tex = res.targets[t.id].texture;
			::Rectangle src = { 0.0f, 0.0f, (float)tex.width, -(float)tex.height }; 
			DrawTexturePro(tex, src, rl(dst), {0,0}, 0.0f, rl(tint));
		}
	}

	TextureHandle RaylibRenderer::registerTexture(unsigned int raylibTexId, int w, int h) {
		uint32_t id = res.nextId++;
		res.textures[id] = { raylibTexId, w, h, 1, 7 }; 
		return { id, w, h };
	}

	FontHandle RaylibRenderer::registerFont(unsigned int raylibFontId) {
		uint32_t id = res.nextId++;
		res.fonts[id] = {}; 
		return { id };
	}

	EffectHandle RaylibRenderer::registerEffect(unsigned int raylibShaderId) {
		uint32_t id = res.nextId++;
		res.shaders[id] = {}; 
		return { id };
	}
}