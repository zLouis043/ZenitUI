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
		PointerState s;
		s.pos      = { mp.x, mp.y };
		s.down     = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
		s.pressed  = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		s.released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
		s.rightDown     = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
		s.rightPressed  = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
		s.rightReleased = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
		s.wheelY   = GetMouseWheelMove();
		return s;
	}
	double RaylibPlatform::time() { return GetTime(); }

	bool RaylibPlatform::shiftHeld() {  
		return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	}

	InputEvents RaylibPlatform::pollInputEvents() {
		InputEvents ev;

		static const struct { int rl; int mapped; } kMap[] = {
			{ KEY_BACKSPACE, Key::Backspace },
			{ KEY_DELETE,    Key::Delete    },
			{ KEY_ENTER,     Key::Enter     },
			{ KEY_ESCAPE,    Key::Escape    },
			{ KEY_LEFT,      Key::Left      },
			{ KEY_RIGHT,     Key::Right     },
			{ KEY_HOME,      Key::Home      },
			{ KEY_END,       Key::End       },
			{ KEY_TAB,       Key::Tab       },
			{ KEY_SPACE,     Key::Space     },
		};

		// Caratteri (rising edge)
		int c;
		while ((c = GetCharPressed()) > 0) ev.chars.push_back(c);

		// Tasti speciali appena premuti (rising edge)
		int k;
		while ((k = GetKeyPressed()) > 0) {
			for (auto& m : kMap)
				if (m.rl == k) { ev.keys.push_back(m.mapped); break; }
		}

		// Tasti correntemente premuti (level)
		for (auto& m : kMap)
			if (IsKeyDown(m.rl)) ev.held.push_back(m.mapped);

		return ev;
	}

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

		void RaylibRenderer::strokeRect(Rect r, float thickness, Color c) {
		if (thickness <= 0.0f || r.width <= 0 || r.height <= 0) return;
		DrawRectangleLinesEx(rl(r), thickness, rl(c));
	}

	void RaylibRenderer::strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c) {
		if (thickness <= 0.0f || r.width <= 0 || r.height <= 0) return;
		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f) return;
		float roundness = std::clamp(radiusPx / maxR, 0.0f, 1.0f);
		DrawRectangleRoundedLinesEx(rl(r), roundness, 8, thickness, rl(c));
	}

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

	void RaylibRenderer::pushEffect(EffectHandle e) {
		if (res.shaders.count(e.id)) BeginShaderMode(res.shaders[e.id]);
	}
	void RaylibRenderer::popEffect() { EndShaderMode(); }

	static Rect intersectRect(const Rect& a, const Rect& b) {
		float x1 = std::max(a.x, b.x);
		float y1 = std::max(a.y, b.y);
		float x2 = std::min(a.x + a.width,  b.x + b.width);
		float y2 = std::min(a.y + a.height, b.y + b.height);
		float w = std::max(0.0f, x2 - x1);
		float h = std::max(0.0f, y2 - y1);
		return { x1, y1, w, h };
	}

	Rect RaylibRenderer::transformClipToScreen(const Rect& local) const {
		// Se non ci sono transform attivi, il rect è già in coordinate schermo.
		if (transformStack.empty()) return local;

		// 4 angoli
		Vec2 corners[4] = {
			{ local.x,                    local.y },
			{ local.x + local.width,      local.y },
			{ local.x,                    local.y + local.height },
			{ local.x + local.width,      local.y + local.height }
		};

		// Applica TUTTI i transform attivi, in ordine di stack (dal più esterno).
		for (auto it = transformStack.rbegin(); it != transformStack.rend(); ++it) {
			for (auto& c : corners) c = applyTransform(*it, c);
		}

		// Bounding box dei 4 angoli trasformati
		float minX = corners[0].x, maxX = corners[0].x;
		float minY = corners[0].y, maxY = corners[0].y;
		for (int i = 1; i < 4; ++i) {
			minX = std::min(minX, corners[i].x);
			maxX = std::max(maxX, corners[i].x);
			minY = std::min(minY, corners[i].y);
			maxY = std::max(maxY, corners[i].y);
		}
		return { minX, minY, maxX - minX, maxY - minY };
	}

	void RaylibRenderer::pushTransform(const Transform2D& t) {
		transformStack.push_back(t);          // NUOVO

		rlPushMatrix();
		rlTranslatef(t.pivot.x + t.translate.x, t.pivot.y + t.translate.y, 0);
		rlRotatef(t.rotationDeg, 0, 0, 1);
		rlScalef(t.scale, t.scale, 1);
		rlTranslatef(-t.pivot.x, -t.pivot.y, 0);
	}
	void RaylibRenderer::popTransform() {
		rlPopMatrix();
		if (!transformStack.empty()) transformStack.pop_back();   // NUOVO
	}

	void RaylibRenderer::pushClip(Rect r) {
		// Converti il rect in coordinate schermo con i transform correnti
		Rect screenRect = transformClipToScreen(r);

		if (clipActive) {
			clipStack.push_back(currentClip);
			currentClip = intersectRect(currentClip, screenRect);
		} else {
			clipStack.push_back({ 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() });
			currentClip = screenRect;
			clipActive = true;
		}

		int x = (int)std::floor(currentClip.x);
		int y = (int)std::floor(currentClip.y);
		int w = (int)std::ceil(currentClip.width);
		int h = (int)std::ceil(currentClip.height);
		if (w <= 0 || h <= 0) { BeginScissorMode(0, 0, 0, 0); return; }
		BeginScissorMode(x, y, w, h);
	}

	void RaylibRenderer::popClip() {
		if (clipStack.empty()) return;
		currentClip = clipStack.back();
		clipStack.pop_back();

		if (clipStack.empty()) {
			clipActive = false;
			EndScissorMode();
			return;
		}

		int x = (int)std::floor(currentClip.x);
		int y = (int)std::floor(currentClip.y);
		int w = (int)std::ceil(currentClip.width);
		int h = (int)std::ceil(currentClip.height);
		BeginScissorMode(x, y, w, h);
	}

    Rect RaylibRenderer::getClipRect() const
    {
        return clipActive ? currentClip : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
    }

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

	TextureHandle RaylibRenderer::registerTexture(const ::Texture2D& t) {
		uint32_t id = res.nextId++;
		res.textures[id] = t;
		return { id, t.width, t.height };
	}

	FontHandle RaylibRenderer::registerFont(const ::Font& f) {
		uint32_t id = res.nextId++;
		res.fonts[id] = f;
		return { id };
	}

	EffectHandle RaylibRenderer::registerEffect(const ::Shader& s) {
		uint32_t id = res.nextId++;
		res.shaders[id] = s;
		return { id };
	}
}