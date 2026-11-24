#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "RaylibBackend.hpp"
#include "Logger.hpp"
#include <raylib.h>
#include <rlgl.h>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cmath>

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

		int c;
		while ((c = GetCharPressed()) > 0)
			ev.chars.push_back(c);

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

		for (auto &m : kMap)
			if (IsKeyDown(m.rl))
				ev.held.push_back(m.mapped);

		return ev;
	}

	float RaylibPlatform::dpiScale()
	{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(__APPLE__)
		return 1.0f;
#else
		return 1.0f;
#endif
	}

	EdgeInsets RaylibPlatform::safeArea()
	{

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
		using UniformCache = std::unordered_map<
			std::string, int, detail::TransparentStringHash, std::equal_to<>>;

		std::unordered_map<uint32_t, ::RenderTexture2D> targets;
		std::unordered_map<uint32_t, ::Texture2D> textures;
		std::unordered_map<uint32_t, ::Font> fonts;
		std::unordered_map<uint32_t, ::Shader> shaders;
		std::unordered_map<uint32_t, UniformCache> shaderUniforms;
		uint32_t nextId{1};
	};
	static RaylibResources res;

	RaylibRenderer::RaylibRenderer() {}
	RaylibRenderer::~RaylibRenderer() {}

	static constexpr int kRoundedCornerSegments = 12;

	void RaylibRenderer::fillRect(Rect r, Color c) { DrawRectangleRec(rl(r), rl(c)); }
	void RaylibRenderer::fillRoundedRect(Rect r, float radiusPx, Color c)
	{
		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR > 0.0f)
			DrawRectangleRounded(rl(r), std::clamp(radiusPx / maxR, 0.0f, 1.0f), kRoundedCornerSegments, rl(c));
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
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f || c.a == 0)
			return;

		float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return;

		float t = std::min(thickness, maxR);
		if (t >= maxR - 0.01f)
		{
			fillRoundedRect(r, radiusPx, c);
			return;
		}

		float outerR = std::clamp(radiusPx, 0.0f, maxR);
		float innerR = std::max(0.0f, outerR - t);

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);

		// 4 corners in clockwise screen order: TR, BR, BL, TL
		const Vec2 outerCenters[4] = {
			{r.x + r.width - outerR, r.y + outerR},
			{r.x + r.width - outerR, r.y + r.height - outerR},
			{r.x + outerR,           r.y + r.height - outerR},
			{r.x + outerR,           r.y + outerR}};

		const Vec2 innerCenters[4] = {
			{r.x + r.width - t - innerR, r.y + t + innerR},
			{r.x + r.width - t - innerR, r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + t + innerR}};

		const float startAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};

		constexpr int kRingPts = 4 * (segs + 1);
		Vec2 outerPts[kRingPts];
		Vec2 innerPts[kRingPts];

		int idx = 0;
		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				float a = startAngles[corner] + kStep * static_cast<float>(s);
				float ca = std::cos(a);
				float sa = std::sin(a);
				outerPts[idx] = {outerCenters[corner].x + ca * outerR,
								 outerCenters[corner].y + sa * outerR};
				innerPts[idx] = {innerCenters[corner].x + ca * innerR,
								 innerCenters[corner].y + sa * innerR};
				++idx;
			}
		}

		rlCheckRenderBatchLimit(kRingPts * 6);
		rlSetTexture(0);
		rlBegin(RL_TRIANGLES);
		rlColor4ub(c.r, c.g, c.b, c.a);

		for (int i = 0; i < kRingPts; ++i)
		{
			int next = (i + 1) % kRingPts;
			const Vec2 &o0 = outerPts[i];
			const Vec2 &o1 = outerPts[next];
			const Vec2 &i0 = innerPts[i];
			const Vec2 &i1 = innerPts[next];

			rlVertex2f(o0.x, o0.y);
			rlVertex2f(i1.x, i1.y);
			rlVertex2f(o1.x, o1.y);

			rlVertex2f(o0.x, o0.y);
			rlVertex2f(i0.x, i0.y);
			rlVertex2f(i1.x, i1.y);
		}

		rlEnd();
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
		Vector2 snappedPos = {std::round(pos.x), std::round(pos.y)};
		DrawTextEx(font, str.c_str(), snappedPos, size, spacing, rl(c));
	}

	Vec2 RaylibRenderer::measureText(FontHandle f, std::string_view s, float size, float spacing)
	{
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		std::string str(s);
		auto v = MeasureTextEx(font, str.c_str(), size * dpiScale_, spacing * dpiScale_);
		return {v.x / dpiScale_, v.y / dpiScale_};
	}

	Vec2 RaylibRenderer::measureCodepoint(FontHandle f, uint32_t codepoint, float size, float /*spacing*/)
	{
		::Font font = res.fonts.count(f.id) ? res.fonts[f.id] : GetFontDefault();
		if (font.baseSize <= 0)
			return {0.0f, size};
		float scale = size / static_cast<float>(font.baseSize);
		int idx = GetGlyphIndex(font, static_cast<int>(codepoint));
		float adv = (font.glyphs[idx].advanceX == 0)
						? (font.recs[idx].width * scale)
						: (static_cast<float>(font.glyphs[idx].advanceX) * scale);
		return {adv, size};
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

		if (transformStack.empty())
			return local;

		Vec2 corners[4] = {
			{local.x, local.y},
			{local.x + local.width, local.y},
			{local.x, local.y + local.height},
			{local.x + local.width, local.y + local.height}};

		for (auto it = transformStack.rbegin(); it != transformStack.rend(); ++it)
		{
			for (auto &c : corners)
				c = applyTransform(*it, c);
		}

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
		transformStack.push_back(t);

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
			transformStack.pop_back();
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

		for (size_t i = 0; i < transformStack.size(); ++i)
			rlPopMatrix();

		if (insideTarget_)
			EndTextureMode();

		targetStates_.push_back({currentTarget_, clipActive, currentClip,
								 std::move(clipStack), std::move(transformStack),
								 currentTargetW_, currentTargetH_});

		clipActive = false;
		clipStack.clear();
		currentClip = {0.0f, 0.0f, (float)t.width, (float)t.height};

		transformStack.clear();

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

		for (size_t i = 0; i < transformStack.size(); ++i)
			rlPopMatrix();

		EndTextureMode();

		if (!targetStates_.empty())
		{
			auto s = std::move(targetStates_.back());
			targetStates_.pop_back();
			currentTarget_ = s.target;
			clipActive = s.clipActive;
			currentClip = s.currentClip;
			clipStack = std::move(s.clipStack);
			transformStack = std::move(s.transformStack);
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

	static ::RenderTexture2D loadColorOnlyRenderTexture(int w, int h)
	{
		::RenderTexture2D target = {0};
#if defined(RAYLIB_VERSION_MAJOR) && ((RAYLIB_VERSION_MAJOR > 5) || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5))
		target.id = rlLoadFramebuffer();
#else
		target.id = rlLoadFramebuffer(w, h);
#endif
		if (target.id > 0)
		{
			rlEnableFramebuffer(target.id);
			target.texture.id = rlLoadTexture(nullptr, w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
			target.texture.width = w;
			target.texture.height = h;
			target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			target.texture.mipmaps = 1;

			rlFramebufferAttach(target.id, target.texture.id,
								RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

			if (!rlFramebufferComplete(target.id))
			{
				logWarn("RaylibRenderer", "", 0, 0,
						"color-only framebuffer incomplete, falling back to LoadRenderTexture");
				rlDisableFramebuffer();
				rlUnloadTexture(target.texture.id);
				rlUnloadFramebuffer(target.id);
				return LoadRenderTexture(w, h);
			}
			rlDisableFramebuffer();
			return target;
		}
		return LoadRenderTexture(w, h);
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
		res.targets[id] = loadColorOnlyRenderTexture(w, h);

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

	void RaylibRenderer::drawTarget(TargetHandle t, Rect src, Rect dst, Color tint)
	{
		auto it = res.targets.find(t.id);
		if (it == res.targets.end())
			return;
		::Texture2D tex = it->second.texture;
		::Rectangle rlSrc = {
			src.x,
			(float)tex.height - src.y - src.height,
			src.width,
			-src.height};
		DrawTexturePro(tex, rlSrc, rl(dst), {0, 0}, 0.0f, rl(tint));
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

	void RaylibRenderer::unregisterFont(FontHandle h)
	{
		auto it = res.fonts.find(h.id);
		if (it == res.fonts.end())
			return;
		UnloadFont(it->second);
		res.fonts.erase(it);
	}

	void RaylibRenderer::unregisterTexture(TextureHandle h)
	{
		auto it = res.textures.find(h.id);
		if (it == res.textures.end())
			return;
		UnloadTexture(it->second);
		res.textures.erase(it);
	}

	void RaylibRenderer::unregisterEffect(EffectHandle h)
	{
		auto it = res.shaders.find(h.id);
		if (it == res.shaders.end())
			return;
		UnloadShader(it->second);
		res.shaders.erase(it);
		res.shaderUniforms.erase(h.id);
	}

	static int shaderLoc(EffectHandle e, const char *name)
	{
		auto it = res.shaders.find(e.id);
		if (it == res.shaders.end())
			return -1;

		auto &cache = res.shaderUniforms[e.id];
		if (auto lit = cache.find(std::string_view(name)); lit != cache.end())
			return lit->second;

		int loc = GetShaderLocation(it->second, name);
		cache.emplace(name, loc);
		return loc;
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

	bool RaylibAssetProvider::loadFont(std::string name, const char *path, int baseSize)
	{
		return loadFont(std::move(name), path, baseSize, defaultCodepoints_);
	}

	bool RaylibAssetProvider::loadFont(std::string name, const char *path, int baseSize,
									   const CodepointSet &codepoints)
	{
		if (name.empty() || !path)
			return false;
		if (fonts.count(name))
		{
			logWarn("Assets", "", 0, 0,
					"font '" + name + "' already loaded");
			return false;
		}

		auto cps = codepoints.build();
		int *cpData = cps.empty() ? nullptr : cps.data();
		int cpCount = static_cast<int>(cps.size());

		::Font f = LoadFontEx(path, baseSize, cpData, cpCount);
		if (f.texture.id == 0)
		{
			logError("Assets", "", 0, 0,
					 "failed to load font '" + name + "' from '" + path + "'");
			return false;
		}
		SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
		fonts[name] = renderer.registerFont(f);
		return true;
	}

	FontHandle RaylibAssetProvider::loadFont(std::string_view name, std::string_view path)
	{
		if (!loadFont(std::string(name), std::string(path).c_str(), 96, defaultCodepoints_))
			return {};
		return fonts[std::string(name)];
	}

	FontHandle RaylibAssetProvider::loadFontFromMemory(std::string_view name,
													   const void *data, size_t size)
	{
		return loadFontFromMemory(name, data, size, 96, defaultCodepoints_);
	}

	FontHandle RaylibAssetProvider::loadFontFromMemory(std::string_view name,
													   const void *data, size_t size,
													   int baseSize,
													   const CodepointSet &codepoints)
	{
		if (name.empty() || !data || size == 0)
			return {};

		const std::string key(name);
		if (fonts.count(key))
		{
			logWarn("Assets", "", 0, 0,
					"font '" + key + "' already loaded");
			return fonts[key];
		}

		std::filesystem::path tmp =
			std::filesystem::temp_directory_path() / ("zenit_font_" + key + ".ttf");
		{
			std::ofstream out(tmp, std::ios::binary);
			if (!out)
			{
				logError("Assets", "", 0, 0,
						 "failed to open temp file for font '" + key + "'");
				return {};
			}
			out.write(static_cast<const char *>(data),
					  static_cast<std::streamsize>(size));
		}

		auto cps = codepoints.build();
		int *cpData = cps.empty() ? nullptr : cps.data();
		int cpCount = static_cast<int>(cps.size());

		::Font f = LoadFontEx(tmp.string().c_str(), baseSize, cpData, cpCount);
		std::error_code ec;
		std::filesystem::remove(tmp, ec);

		if (f.texture.id == 0)
		{
			logError("Assets", "", 0, 0,
					 "failed to load font '" + key + "' from memory");
			return {};
		}
		SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
		FontHandle h = renderer.registerFont(f);
		fonts[key] = h;
		return h;
	}

	void RaylibAssetProvider::unloadFont(std::string_view name)
	{
		auto it = fonts.find(name);
		if (it == fonts.end())
			return;
		renderer.unregisterFont(it->second);
		fonts.erase(it);
	}

	bool RaylibAssetProvider::loadTexture(std::string name, const char *path)
	{
		if (name.empty() || !path)
			return false;
		if (textures.count(name))
			return false;
		::Texture2D t = LoadTexture(path);
		if (t.id == 0)
			return false;
		textures[name] = renderer.registerTexture(t);
		return true;
	}

	bool RaylibAssetProvider::loadEffect(std::string name, const char *fsPath, const char *vsPath)
	{
		if (name.empty() || !fsPath)
			return false;
		if (effects.count(name))
			return false;
		::Shader s = LoadShader(vsPath, fsPath);
		if (s.id == 0)
			return false;
		effects[name] = renderer.registerEffect(s);
		return true;
	}
}