#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "RaylibBackend.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <string>
#include <unordered_map>

namespace ZenitUI
{

	static ::Color rl(const Color &c) { return {c.r, c.g, c.b, c.a}; }
	static ::Vector2 rl(const Vec2 &v) { return {v.x, v.y}; }
	static ::Rectangle rl(const Rect &r) { return {r.x, r.y, r.width, r.height}; }

	Vec2 RaylibPlatform::viewportSize()
	{
		float s = dpiScale();
		return {(float)GetScreenWidth() / s, (float)GetScreenHeight() / s};
	}
	PointerState RaylibPlatform::pointer()
	{
		auto mp = GetMousePosition();
		float dS = dpiScale();
		PointerState s;
		s.pos = {mp.x / dS, mp.y / dS};
		s.wheelY = GetMouseWheelMove();

		// Canonical per-button state.
		auto &left = s[MouseButton::Left];
		auto &right = s[MouseButton::Right];
		auto &mid = s[MouseButton::Middle];

		left.down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
		left.pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		left.released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

		right.down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
		right.pressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
		right.released = IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);

		mid.down = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
		mid.pressed = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
		mid.released = IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE);

		return s;
	}
	double RaylibPlatform::time() { return GetTime(); }

	ModifierState RaylibPlatform::modifiers()
	{
		ModifierState m;
		m.shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
		m.ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
		m.alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
		m.super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
		return m;
	}

	InputEvents RaylibPlatform::pollInputEvents()
	{
		InputEvents ev;

		static const struct
		{
			int rl;
			int mapped;
		} kMap[] = {
			{KEY_BACKSPACE, Key::Backspace},
			{KEY_DELETE, Key::Delete},
			{KEY_ENTER, Key::Enter},
			{KEY_ESCAPE, Key::Escape},
			{KEY_LEFT, Key::Left},
			{KEY_RIGHT, Key::Right},
			{KEY_UP, Key::Up},
			{KEY_DOWN, Key::Down},
			{KEY_HOME, Key::Home},
			{KEY_END, Key::End},
			{KEY_TAB, Key::Tab},
			{KEY_SPACE, Key::Space},
			{KEY_A, Key::A},
			{KEY_C, Key::C},
			{KEY_V, Key::V},
			{KEY_X, Key::X},
			{KEY_Y, Key::Y},
			{KEY_Z, Key::Z},
		};

		// Caratteri (rising edge)
		int c;
		while ((c = GetCharPressed()) > 0)
			ev.chars.push_back(c);

		// Tasti speciali appena premuti (rising edge)
		int k;
		while ((k = GetKeyPressed()) > 0)
		{
			for (auto &m : kMap)
				if (m.rl == k)
				{
					ev.keys.push_back(m.mapped);
					break;
				}
		}

		// Tasti correntemente premuti (level)
		for (auto &m : kMap)
			if (IsKeyDown(m.rl))
				ev.held.push_back(m.mapped);

		return ev;
	}

	float RaylibPlatform::dpiScale()
	{
		// Desktop: 1.0. Su Android/iOS con FLAG_WINDOW_HIGHDPI, Raylib espone
		// già il framebuffer scalato; qui si ritorna 1.0 e il backend grafico
		// (OpenGL ES) applica lo scale nativamente. Su Windows/retina, se in
		// futuro serve, si può leggere da GetWindowScaleDPI() (Raylib 5.1+).
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(__APPLE__)
		return 1.0f; // già gestito dal backend grafico
#else
		return 1.0f; // desktop standard
#endif
	}

	EdgeInsets RaylibPlatform::safeArea()
	{
		// Desktop: nessun notch/bordo arrotondato. Mobile: qui si leggerebbe
		// dalla API nativa (Android: DisplayCutout + WindowInsets;
		// iOS: safeAreaInsets della UIWindow). Per ora zero.
		return {};
	}

	void RaylibPlatform::setClipboardText(const std::string &text)
	{
		SetClipboardText(text.c_str());
	}

	std::string RaylibPlatform::getClipboardText()
	{
		const char *t = GetClipboardText();
		return t ? std::string(t) : "";
	}

	struct RaylibResources
	{
		std::unordered_map<uint32_t, ::RenderTexture2D> targets;
		std::unordered_map<uint32_t, ::Texture2D> textures;
		std::unordered_map<uint32_t, ::Font> fonts;
		std::unordered_map<uint32_t, ::Shader> shaders;
		uint32_t nextId{1};
	};
	static RaylibResources res;

	RaylibRenderer::RaylibRenderer() {}
	RaylibRenderer::~RaylibRenderer() {}

	void RaylibRenderer::fillRect(Rect r, Color c) { DrawRectangleRec(rl(r), rl(c)); }
	void RaylibRenderer::fillRoundedRect(Rect r, float radiusPx, Color c)
	{
		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR > 0)
			DrawRectangleRounded(rl(r), std::clamp(radiusPx / maxR, 0.0f, 1.0f), 8, rl(c));
	}
	void RaylibRenderer::fillCircle(Vec2 center, float radius, Color c) { DrawCircleV(rl(center), radius, rl(c)); }

	void RaylibRenderer::strokeRect(Rect r, float thickness, Color c)
	{
		if (thickness <= 0.0f || r.width <= 0 || r.height <= 0)
			return;
		DrawRectangleLinesEx(rl(r), thickness, rl(c));
	}

	void RaylibRenderer::strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c)
	{
		if (thickness <= 0.0f || r.width <= 0 || r.height <= 0)
			return;
		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return;
		float roundness = std::clamp(radiusPx / maxR, 0.0f, 1.0f);
		DrawRectangleRoundedLinesEx(rl(r), roundness, 8, thickness, rl(c));
	}

	void RaylibRenderer::drawTexture(TextureHandle t, Rect src, Rect dst, Color tint)
	{
		if (res.textures.count(t.id))
			DrawTexturePro(res.textures[t.id], rl(src), rl(dst), {0, 0}, 0.0f, rl(tint));
	}

	void RaylibRenderer::drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint)
	{
		if (res.textures.count(t.id))
		{
			::NPatchInfo np = {{0, 0, (float)t.width, (float)t.height}, s.left, s.top, s.right, s.bottom, NPATCH_NINE_PATCH};
			DrawTextureNPatch(res.textures[t.id], np, rl(dst), {0, 0}, 0.0f, rl(tint));
		}
	}

	void RaylibRenderer::drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c)
	{
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		std::string str(s);
		DrawTextEx(font, str.c_str(), rl(pos), size, spacing, rl(c));
	}

	Vec2 RaylibRenderer::measureText(FontHandle f, std::string_view s, float size, float spacing)
	{
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		std::string str(s);
		auto v = MeasureTextEx(font, str.c_str(), size * dpiScale_, spacing * dpiScale_);
		return {v.x / dpiScale_, v.y / dpiScale_};
	}

	void RaylibRenderer::pushEffect(EffectHandle e)
	{
		if (!res.shaders.count(e.id))
			return;
		effectStack_.push_back(e);
		BeginShaderMode(res.shaders[e.id]);
	}

	void RaylibRenderer::popEffect()
	{
		if (effectStack_.empty())
			return;
		effectStack_.pop_back();
		EndShaderMode();
		// Riattiva il precedente se ce n'è uno nello stack.
		if (!effectStack_.empty())
		{
			const auto &prev = effectStack_.back();
			if (res.shaders.count(prev.id))
				BeginShaderMode(res.shaders[prev.id]);
		}
	}

	bool RaylibRenderer::inTarget() const { return insideTarget_; }

	void RaylibRenderer::clearTarget(TargetHandle t, Color c)
	{
		if (!res.targets.count(t.id))
			return;
		BeginTextureMode(res.targets[t.id]);
		ClearBackground(::Color{c.r, c.g, c.b, c.a});
		EndTextureMode();
	}

	static Rect intersectRect(const Rect &a, const Rect &b)
	{
		float x1 = std::max(a.x, b.x);
		float y1 = std::max(a.y, b.y);
		float x2 = std::min(a.x + a.width, b.x + b.width);
		float y2 = std::min(a.y + a.height, b.y + b.height);
		float w = std::max(0.0f, x2 - x1);
		float h = std::max(0.0f, y2 - y1);
		return {x1, y1, w, h};
	}

	Rect RaylibRenderer::transformClipToScreen(const Rect &local) const
	{
		// Se non ci sono transform attivi, il rect è già in coordinate schermo.
		if (transformStack.empty())
			return local;

		// 4 angoli
		Vec2 corners[4] = {
			{local.x, local.y},
			{local.x + local.width, local.y},
			{local.x, local.y + local.height},
			{local.x + local.width, local.y + local.height}};

		// Applica TUTTI i transform attivi, in ordine di stack (dal più esterno).
		for (auto it = transformStack.rbegin(); it != transformStack.rend(); ++it)
		{
			for (auto &c : corners)
				c = applyTransform(*it, c);
		}

		// Bounding box dei 4 angoli trasformati
		float minX = corners[0].x, maxX = corners[0].x;
		float minY = corners[0].y, maxY = corners[0].y;
		for (int i = 1; i < 4; ++i)
		{
			minX = std::min(minX, corners[i].x);
			maxX = std::max(maxX, corners[i].x);
			minY = std::min(minY, corners[i].y);
			maxY = std::max(maxY, corners[i].y);
		}
		return {minX, minY, maxX - minX, maxY - minY};
	}

	void RaylibRenderer::setDpiScale(float scale)
	{
		dpiScale_ = (scale > 0.01f) ? scale : 1.0f;
	}

	void RaylibRenderer::beginFrame()
	{
		if (dpiScale_ != 1.0f)
		{
			rlPushMatrix();
			rlScalef(dpiScale_, dpiScale_, 1.0f);
		}
	}

	void RaylibRenderer::endFrame()
	{
		if (dpiScale_ != 1.0f)
			rlPopMatrix();
	}

	void RaylibRenderer::pushTransform(const Transform2D &t)
	{
		transformStack.push_back(t); // NUOVO

		rlPushMatrix();
		rlTranslatef(t.pivot.x + t.translate.x, t.pivot.y + t.translate.y, 0);
		rlRotatef(t.rotationDeg, 0, 0, 1);
		rlScalef(t.scale, t.scale, 1);
		rlTranslatef(-t.pivot.x, -t.pivot.y, 0);
	}
	void RaylibRenderer::popTransform()
	{
		rlPopMatrix();
		if (!transformStack.empty())
			transformStack.pop_back(); // NUOVO
	}

	void RaylibRenderer::applyScissor(const Rect &clip)
	{
		float scale = insideTarget_ ? 1.0f : dpiScale_;
		int x = (int)std::floor(clip.x * scale);
		int y = (int)std::floor(clip.y * scale);
		int w = (int)std::ceil(clip.width * scale);
		int h = (int)std::ceil(clip.height * scale);

		if (!insideTarget_)
		{
			if (w <= 0 || h <= 0)
				BeginScissorMode(0, 0, 0, 0);
			else
				BeginScissorMode(x, y, w, h);
			return;
		}

		// Dentro una RenderTexture2D, BeginScissorMode di Raylib sbaglierebbe
		// usando GetScreenHeight(). Usiamo rlgl direttamente con l'altezza del target.
		rlDrawRenderBatchActive();
		rlEnableScissorTest();
		if (w <= 0 || h <= 0)
		{
			rlScissor(0, 0, 0, 0);
			return;
		}
		rlScissor(x, currentTargetH_ - (y + h), w, h);
	}

	void RaylibRenderer::pushClip(Rect r)
	{
		Rect screenRect = transformClipToScreen(r);

		if (clipActive)
		{
			clipStack.push_back(currentClip);
			currentClip = intersectRect(currentClip, screenRect);
		}
		else
		{
			Rect bounds = insideTarget_
							  ? Rect{0.0f, 0.0f, (float)currentTargetW_, (float)currentTargetH_}
							  : Rect{0.0f, 0.0f, (float)GetScreenWidth() / dpiScale_, (float)GetScreenHeight() / dpiScale_};
			clipStack.push_back(bounds);
			currentClip = intersectRect(bounds, screenRect);
			clipActive = true;
		}

		applyScissor(currentClip);
	}

	void RaylibRenderer::popClip()
	{
		if (clipStack.empty())
			return;
		currentClip = clipStack.back();
		clipStack.pop_back();

		if (clipStack.empty())
		{
			clipActive = false;
			EndScissorMode();
			return;
		}

		applyScissor(currentClip);
	}

	void RaylibRenderer::pushTarget(TargetHandle t)
	{
		if (!res.targets.count(t.id))
			return;

		if (clipActive)
			EndScissorMode();

		rlDrawRenderBatchActive();

		// Svuotiamo le nostre trasformazioni dallo stack OpenGL prima che Raylib chiuda l'FBO!
		for (size_t i = 0; i < transformStack.size(); ++i)
			rlPopMatrix();

		if (insideTarget_)
			EndTextureMode();

		targetStates_.push_back({currentTarget_, clipActive, currentClip, clipStack, transformStack, currentTargetW_, currentTargetH_});

		clipActive = false;
		clipStack.clear();
		currentClip = {0.0f, 0.0f, (float)t.width, (float)t.height};

		transformStack.clear(); // Resettiamo per il nuovo target!

		currentTarget_ = t;
		currentTargetW_ = t.width;
		currentTargetH_ = t.height;
		insideTarget_ = true;

		BeginTextureMode(res.targets[t.id]);
		ClearBackground(::BLANK);
	}

	void RaylibRenderer::popTarget()
	{
		if (clipActive)
			EndScissorMode();

		rlDrawRenderBatchActive();

		// Svuotiamo eventuali trasformazioni rimaste appese in questo target
		for (size_t i = 0; i < transformStack.size(); ++i)
			rlPopMatrix();

		EndTextureMode(); // Ora Raylib chiude l'FBO in totale sicurezza

		if (!targetStates_.empty())
		{
			auto s = targetStates_.back();
			targetStates_.pop_back();
			currentTarget_ = s.target;
			clipActive = s.clipActive;
			currentClip = s.currentClip;
			clipStack = s.clipStack;
			transformStack = s.transformStack; // Ripristiniamo la memoria delle trasformazioni!
			currentTargetW_ = s.targetW;
			currentTargetH_ = s.targetH;
		}
		else
		{
			currentTarget_ = {};
			clipActive = false;
			clipStack.clear();
			currentClip = {0.0f, 0.0f, (float)GetScreenWidth() / dpiScale_, (float)GetScreenHeight() / dpiScale_};
			transformStack.clear();
			currentTargetW_ = 0;
			currentTargetH_ = 0;
		}

		insideTarget_ = currentTarget_.valid();

		if (insideTarget_ && res.targets.count(currentTarget_.id))
		{
			BeginTextureMode(res.targets[currentTarget_.id]);
		}

		// RIAZZERIAMO le trasformazioni per il target (o schermo) ripristinato
		for (const auto &tr : transformStack)
		{
			rlPushMatrix();
			rlTranslatef(tr.pivot.x + tr.translate.x, tr.pivot.y + tr.translate.y, 0);
			rlRotatef(tr.rotationDeg, 0, 0, 1);
			rlScalef(tr.scale, tr.scale, 1);
			rlTranslatef(-tr.pivot.x, -tr.pivot.y, 0);
		}

		if (clipActive)
			applyScissor(currentClip);
	}

	TargetHandle RaylibRenderer::createTarget(int w, int h)
	{
		if (insideTarget_)
		{
			rlDrawRenderBatchActive();
			for (size_t i = 0; i < transformStack.size(); ++i)
				rlPopMatrix();
			EndTextureMode();
		}

		uint32_t id = res.nextId++;
		res.targets[id] = LoadRenderTexture(w, h);

		if (insideTarget_ && res.targets.count(currentTarget_.id))
		{
			BeginTextureMode(res.targets[currentTarget_.id]);

			for (const auto &tr : transformStack)
			{
				rlPushMatrix();
				rlTranslatef(tr.pivot.x + tr.translate.x, tr.pivot.y + tr.translate.y, 0);
				rlRotatef(tr.rotationDeg, 0, 0, 1);
				rlScalef(tr.scale, tr.scale, 1);
				rlTranslatef(-tr.pivot.x, -tr.pivot.y, 0);
			}

			if (clipActive)
				applyScissor(currentClip);
		}
		return {id, w, h};
	}

	Rect RaylibRenderer::getClipRect() const
	{
		// Layout::draw() usa getClipRect() per il frustum culling in coordinate schermo.
		// Se siamo dentro un target, restituiamo il clip dello schermo salvato alla radice.
		if (insideTarget_ && !targetStates_.empty())
		{
			const auto &root = targetStates_.front();
			return root.clipActive
					   ? root.currentClip
					   : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
		}
		return clipActive ? currentClip : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
	}

	void RaylibRenderer::destroyTarget(TargetHandle t)
	{
		if (res.targets.count(t.id))
		{
			UnloadRenderTexture(res.targets[t.id]);
			res.targets.erase(t.id);
		}
	}

	void RaylibRenderer::drawTarget(TargetHandle t, Rect dst, Color tint)
	{
		if (res.targets.count(t.id))
		{
			::Texture2D tex = res.targets[t.id].texture;
			::Rectangle src = {0.0f, 0.0f, (float)tex.width, -(float)tex.height};
			DrawTexturePro(tex, src, rl(dst), {0, 0}, 0.0f, rl(tint));
		}
	}

	TextureHandle RaylibRenderer::registerTexture(const ::Texture2D &t)
	{
		uint32_t id = res.nextId++;
		res.textures[id] = t;
		return {id, t.width, t.height};
	}

	FontHandle RaylibRenderer::registerFont(const ::Font &f)
	{
		uint32_t id = res.nextId++;
		res.fonts[id] = f;
		return {id};
	}

	EffectHandle RaylibRenderer::registerEffect(const ::Shader &s)
	{
		uint32_t id = res.nextId++;
		res.shaders[id] = s;
		return {id};
	}

	static int shaderLoc(EffectHandle e, const char *name)
	{
		auto it = res.shaders.find(e.id);
		if (it == res.shaders.end())
			return -1;
		return GetShaderLocation(it->second, name);
	}

	void RaylibRenderer::setEffectFloat(EffectHandle e, const char *name, float value)
	{
		int loc = shaderLoc(e, name);
		if (loc < 0)
			return;
		SetShaderValue(res.shaders[e.id], loc, &value, SHADER_UNIFORM_FLOAT);
	}

	void RaylibRenderer::setEffectVec2(EffectHandle e, const char *name, Vec2 value)
	{
		int loc = shaderLoc(e, name);
		if (loc < 0)
			return;
		float arr[2] = {value.x, value.y};
		SetShaderValue(res.shaders[e.id], loc, arr, SHADER_UNIFORM_VEC2);
	}

	void RaylibRenderer::setEffectVec3(EffectHandle e, const char *name, Vec3 value)
	{
		int loc = shaderLoc(e, name);
		if (loc < 0)
			return;
		float arr[3] = {value.x, value.y, value.z};
		SetShaderValue(res.shaders[e.id], loc, arr, SHADER_UNIFORM_VEC3);
	}

	void RaylibRenderer::setEffectVec4f(EffectHandle e, const char *name, Vec4 value)
	{
		int loc = shaderLoc(e, name);
		if (loc < 0)
			return;
		float arr[4] = {value.x, value.y, value.z, value.w};
		SetShaderValue(res.shaders[e.id], loc, arr, SHADER_UNIFORM_VEC4);
	}

	void RaylibRenderer::setEffectVec4(EffectHandle e, const char *name, Color value)
	{
		int loc = shaderLoc(e, name);
		if (loc < 0)
			return;
		float arr[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
		SetShaderValue(res.shaders[e.id], loc, arr, SHADER_UNIFORM_VEC4);
	}
}