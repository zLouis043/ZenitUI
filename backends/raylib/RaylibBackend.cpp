#include "RaylibBackend.hpp"
#include "Logger.hpp"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace ZenitUI
{
	// =========================================================================
	// Internal Raylib <-> ZenitUI Conversion & Geometry Helpers
	// =========================================================================
	namespace
	{
		constexpr int kRoundedCornerSegments = 12;

		::Color toRl(const Color &c) { return {c.r, c.g, c.b, c.a}; }
		::Vector2 toRl(const Vec2 &v) { return {v.x, v.y}; }
		::Rectangle toRl(const Rect &r) { return {r.x, r.y, r.width, r.height}; }

		Rect intersectRect(const Rect &a, const Rect &b)
		{
			const float x1 = std::max(a.x, b.x);
			const float y1 = std::max(a.y, b.y);
			const float x2 = std::min(a.x + a.width, b.x + b.width);
			const float y2 = std::min(a.y + a.height, b.y + b.height);
			return {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
		}

		// Push a 2D pivot-based transform onto rlgl's active matrix stack.
		void pushRlTransform(const Transform2D &t)
		{
			rlPushMatrix();
			rlTranslatef(t.pivot.x + t.translate.x, t.pivot.y + t.translate.y, 0.0f);
			rlRotatef(t.rotationDeg, 0.0f, 0.0f, 1.0f);
			rlScalef(t.scale, t.scale, 1.0f);
			rlTranslatef(-t.pivot.x, -t.pivot.y, 0.0f);
		}

		ButtonState sampleMouseButton(int rlButton)
		{
			return {
				IsMouseButtonDown(rlButton),
				IsMouseButtonPressed(rlButton),
				IsMouseButtonReleased(rlButton)};
		}

		// Off-screen layers only need RGBA color (no depth/stencil buffer).
		// Falls back to Raylib's LoadRenderTexture if the driver rejects a
		// color-only FBO.
		::RenderTexture2D loadColorOnlyRenderTexture(int w, int h)
		{
			::RenderTexture2D target = {0};
#if defined(RAYLIB_VERSION_MAJOR) && ((RAYLIB_VERSION_MAJOR > 5) || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5))
			target.id = rlLoadFramebuffer();
#else
			target.id = rlLoadFramebuffer(w, h);
#endif
			if (target.id == 0)
				return LoadRenderTexture(w, h);

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
	} // namespace

	// =========================================================================
	// 1. RaylibPlatform (IPlatform)
	// =========================================================================

	Vec2 RaylibPlatform::viewportSize()
	{
		const float s = dpiScale();
		return {static_cast<float>(GetScreenWidth()) / s,
				static_cast<float>(GetScreenHeight()) / s};
	}

	double RaylibPlatform::time()
	{
		return GetTime();
	}

	float RaylibPlatform::dpiScale()
	{
		// Raylib desktop reports screen coordinates in logical pixels by default.
		// Backends targeting HiDPI displays where framebuffer != logical size
		// should return the physical-to-logical ratio here (e.g. GetWindowScaleDPI().x).
		return 1.0f;
	}

	EdgeInsets RaylibPlatform::safeArea()
	{
		return {};
	}

	PointerState RaylibPlatform::pointer()
	{
		const auto mp = GetMousePosition();
		const float s = dpiScale();

		PointerState state;
		state.pos = {mp.x / s, mp.y / s};
		state.wheelY = GetMouseWheelMove();
		state[MouseButton::Left] = sampleMouseButton(MOUSE_BUTTON_LEFT);
		state[MouseButton::Right] = sampleMouseButton(MOUSE_BUTTON_RIGHT);
		state[MouseButton::Middle] = sampleMouseButton(MOUSE_BUTTON_MIDDLE);
		return state;
	}

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
		struct KeyBinding
		{
			int rlKey;
			int zenitKey;
		};
		static constexpr KeyBinding kKeyMap[] = {
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

		InputEvents ev;

		for (int ch = GetCharPressed(); ch > 0; ch = GetCharPressed())
			ev.chars.push_back(ch);

		for (int key = GetKeyPressed(); key > 0; key = GetKeyPressed())
		{
			for (const auto &entry : kKeyMap)
			{
				if (entry.rlKey == key)
				{
					ev.keys.push_back(entry.zenitKey);
					break;
				}
			}
		}

		for (const auto &entry : kKeyMap)
		{
			if (IsKeyDown(entry.rlKey))
				ev.held.push_back(entry.zenitKey);
		}

		return ev;
	}

	void RaylibPlatform::setClipboardText(const std::string &text)
	{
		SetClipboardText(text.c_str());
	}

	std::string RaylibPlatform::getClipboardText()
	{
		const char *text = GetClipboardText();
		return text ? std::string(text) : std::string{};
	}

	// =========================================================================
	// 2. RaylibRenderer (IRenderer)
	// =========================================================================

	struct RaylibRenderer::Resources
	{
		using UniformCache = std::unordered_map<
			std::string, int, detail::TransparentStringHash, std::equal_to<>>;

		std::unordered_map<uint32_t, ::RenderTexture2D> targets;
		std::unordered_map<uint32_t, ::Texture2D> textures;
		std::unordered_map<uint32_t, ::Font> fonts;
		std::unordered_map<uint32_t, ::Shader> shaders;
		std::unordered_map<uint32_t, UniformCache> shaderUniforms;
		uint32_t nextId{1};

		::Font resolveFont(FontHandle h) const
		{
			auto it = fonts.find(h.id);
			return it != fonts.end() ? it->second : GetFontDefault();
		}

		int uniformLocation(EffectHandle e, const char *name)
		{
			auto it = shaders.find(e.id);
			if (it == shaders.end())
				return -1;

			auto &cache = shaderUniforms[e.id];
			if (auto lit = cache.find(std::string_view(name)); lit != cache.end())
				return lit->second;

			const int loc = GetShaderLocation(it->second, name);
			cache.emplace(name, loc);
			return loc;
		}
	};

	RaylibRenderer::RaylibRenderer() : res_(std::make_unique<Resources>()) {}
	RaylibRenderer::~RaylibRenderer() = default;

	// ---- Frame lifecycle & DPI ----

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

	// ---- 2D vector primitives ----

	void RaylibRenderer::fillRect(Rect r, Color c)
	{
		DrawRectangleRec(toRl(r), toRl(c));
	}

	void RaylibRenderer::fillRoundedRect(Rect r, float radiusPx, Color c)
	{
		const float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return;
		const float roundness = std::clamp(radiusPx / maxR, 0.0f, 1.0f);
		DrawRectangleRounded(toRl(r), roundness, kRoundedCornerSegments, toRl(c));
	}

	void RaylibRenderer::fillCircle(Vec2 center, float radius, Color c)
	{
		DrawCircleV(toRl(center), radius, toRl(c));
	}

	void RaylibRenderer::strokeRect(Rect r, float thickness, Color c)
	{
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f)
			return;
		DrawRectangleLinesEx(toRl(r), thickness, toRl(c));
	}

	void RaylibRenderer::strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c)
	{
		// ZenitUI follows CSS border-box semantics: borders are drawn strictly
		// *inside* `r` so they stay flush against `fillRoundedRect(r, ...)`
		// without anti-aliasing seams. Raylib's `DrawRectangleRoundedLinesEx`
		// expands outward and leaves a ~1px gap under MSAA, so we emit an
		// inward-extruded rounded ring directly via rlgl.
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f || c.a == 0)
			return;

		const float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return;

		const float t = std::min(thickness, maxR);
		if (t >= maxR - 0.01f)
		{
			fillRoundedRect(r, radiusPx, c);
			return;
		}

		const float outerR = std::clamp(radiusPx, 0.0f, maxR);
		const float innerR = std::max(0.0f, outerR - t);

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);

		// Corners in clockwise screen order: top-right, bottom-right, bottom-left, top-left.
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

		constexpr float kStartAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};
		constexpr int kRingPts = 4 * (segs + 1);

		Vec2 outerPts[kRingPts];
		Vec2 innerPts[kRingPts];

		int idx = 0;
		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				const float a = kStartAngles[corner] + kStep * static_cast<float>(s);
				const float ca = std::cos(a);
				const float sa = std::sin(a);
				outerPts[idx] = {outerCenters[corner].x + ca * outerR,
								 outerCenters[corner].y + sa * outerR};
				innerPts[idx] = {innerCenters[corner].x + ca * innerR,
								 innerCenters[corner].y + sa * innerR};
				++idx;
			}
		}

		// Emit counter-clockwise triangles in screen space (Y-down) for OpenGL back-face culling.
		rlCheckRenderBatchLimit(kRingPts * 6);
		rlSetTexture(0);
		rlBegin(RL_TRIANGLES);
		rlColor4ub(c.r, c.g, c.b, c.a);

		for (int i = 0; i < kRingPts; ++i)
		{
			const int next = (i + 1) % kRingPts;
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

	// ---- Textures & nine-slice ----

	void RaylibRenderer::drawTexture(TextureHandle t, Rect src, Rect dst, Color tint)
	{
		if (auto it = res_->textures.find(t.id); it != res_->textures.end())
			DrawTexturePro(it->second, toRl(src), toRl(dst), {0.0f, 0.0f}, 0.0f, toRl(tint));
	}

	void RaylibRenderer::drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint)
	{
		if (auto it = res_->textures.find(t.id); it != res_->textures.end())
		{
			const ::NPatchInfo np = {
				{0.0f, 0.0f, static_cast<float>(t.width), static_cast<float>(t.height)},
				s.left, s.top, s.right, s.bottom,
				NPATCH_NINE_PATCH};
			DrawTextureNPatch(it->second, np, toRl(dst), {0.0f, 0.0f}, 0.0f, toRl(tint));
		}
	}

	// ---- Text rendering & metrics ----

	void RaylibRenderer::drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c)
	{
		const ::Font font = res_->resolveFont(f);
		// Raylib's C API requires a null-terminated string; snap to whole pixels for crisp glyphs.
		const std::string str(s);
		const ::Vector2 snappedPos = {std::round(pos.x), std::round(pos.y)};
		DrawTextEx(font, str.c_str(), snappedPos, size, spacing, toRl(c));
	}

	Vec2 RaylibRenderer::measureText(FontHandle f, std::string_view s, float size, float spacing)
	{
		const ::Font font = res_->resolveFont(f);
		const std::string str(s);
		const ::Vector2 v = MeasureTextEx(font, str.c_str(), size * dpiScale_, spacing * dpiScale_);
		return {v.x / dpiScale_, v.y / dpiScale_};
	}

	Vec2 RaylibRenderer::measureCodepoint(FontHandle f, uint32_t codepoint, float size, float /*spacing*/)
	{
		const ::Font font = res_->resolveFont(f);
		if (font.baseSize <= 0)
			return {0.0f, size};
		const float scale = size / static_cast<float>(font.baseSize);
		const int idx = GetGlyphIndex(font, static_cast<int>(codepoint));
		const float adv = (font.glyphs[idx].advanceX == 0)
							  ? (font.recs[idx].width * scale)
							  : (static_cast<float>(font.glyphs[idx].advanceX) * scale);
		return {adv, size};
	}

	// ---- Transform, clip & shader stacks ----

	void RaylibRenderer::pushTransform(const Transform2D &t)
	{
		transformStack_.push_back(t);
		pushRlTransform(t);
	}

	void RaylibRenderer::popTransform()
	{
		rlPopMatrix();
		if (!transformStack_.empty())
			transformStack_.pop_back();
	}

	Rect RaylibRenderer::transformClipToScreen(const Rect &local) const
	{
		if (transformStack_.empty())
			return local;

		Vec2 corners[4] = {
			{local.x, local.y},
			{local.x + local.width, local.y},
			{local.x, local.y + local.height},
			{local.x + local.width, local.y + local.height}};

		for (auto it = transformStack_.rbegin(); it != transformStack_.rend(); ++it)
		{
			for (auto &corner : corners)
				corner = applyTransform(*it, corner);
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

	void RaylibRenderer::applyScissor(const Rect &clip)
	{
		const float scale = insideTarget_ ? 1.0f : dpiScale_;
		const int x = static_cast<int>(std::floor(clip.x * scale));
		const int y = static_cast<int>(std::floor(clip.y * scale));
		const int w = static_cast<int>(std::ceil(clip.width * scale));
		const int h = static_cast<int>(std::ceil(clip.height * scale));

		if (!insideTarget_)
		{
			if (w <= 0 || h <= 0)
				BeginScissorMode(0, 0, 0, 0);
			else
				BeginScissorMode(x, y, w, h);
			return;
		}

		// Inside an off-screen FBO, OpenGL's scissor origin is bottom-left.
		rlDrawRenderBatchActive();
		rlEnableScissorTest();
		if (w <= 0 || h <= 0)
			rlScissor(0, 0, 0, 0);
		else
			rlScissor(x, currentTargetH_ - (y + h), w, h);
	}

	void RaylibRenderer::pushClip(Rect r)
	{
		const Rect screenRect = transformClipToScreen(r);

		if (clipActive_)
		{
			clipStack_.push_back(currentClip_);
			currentClip_ = intersectRect(currentClip_, screenRect);
		}
		else
		{
			const Rect bounds = insideTarget_
									? Rect{0.0f, 0.0f, static_cast<float>(currentTargetW_), static_cast<float>(currentTargetH_)}
									: Rect{0.0f, 0.0f, static_cast<float>(GetScreenWidth()) / dpiScale_, static_cast<float>(GetScreenHeight()) / dpiScale_};
			clipStack_.push_back(bounds);
			currentClip_ = intersectRect(bounds, screenRect);
			clipActive_ = true;
		}

		applyScissor(currentClip_);
	}

	void RaylibRenderer::popClip()
	{
		if (clipStack_.empty())
			return;

		currentClip_ = clipStack_.back();
		clipStack_.pop_back();

		if (clipStack_.empty())
		{
			clipActive_ = false;
			EndScissorMode();
			return;
		}

		applyScissor(currentClip_);
	}

	Rect RaylibRenderer::getClipRect() const
	{
		// When drawing inside a layer target, report the root screen clip so
		// culling checks against screen-space node bounds stay accurate.
		if (insideTarget_ && !targetStates_.empty())
		{
			const auto &root = targetStates_.front();
			return root.clipActive
					   ? root.currentClip
					   : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
		}
		return clipActive_
				   ? currentClip_
				   : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
	}

	void RaylibRenderer::pushEffect(EffectHandle e)
	{
		auto it = res_->shaders.find(e.id);
		if (it == res_->shaders.end())
			return;
		effectStack_.push_back(e);
		BeginShaderMode(it->second);
	}

	void RaylibRenderer::popEffect()
	{
		if (effectStack_.empty())
			return;
		effectStack_.pop_back();
		EndShaderMode();

		if (!effectStack_.empty())
		{
			if (auto it = res_->shaders.find(effectStack_.back().id); it != res_->shaders.end())
				BeginShaderMode(it->second);
		}
	}

	// ---- Shader uniforms ----

	void RaylibRenderer::setEffectFloat(EffectHandle e, const char *name, float value)
	{
		const int loc = res_->uniformLocation(e, name);
		if (loc >= 0)
			SetShaderValue(res_->shaders[e.id], loc, &value, SHADER_UNIFORM_FLOAT);
	}

	void RaylibRenderer::setEffectVec2(EffectHandle e, const char *name, Vec2 value)
	{
		const int loc = res_->uniformLocation(e, name);
		if (loc >= 0)
		{
			const float arr[2] = {value.x, value.y};
			SetShaderValue(res_->shaders[e.id], loc, arr, SHADER_UNIFORM_VEC2);
		}
	}

	void RaylibRenderer::setEffectVec3(EffectHandle e, const char *name, Vec3 value)
	{
		const int loc = res_->uniformLocation(e, name);
		if (loc >= 0)
		{
			const float arr[3] = {value.x, value.y, value.z};
			SetShaderValue(res_->shaders[e.id], loc, arr, SHADER_UNIFORM_VEC3);
		}
	}

	void RaylibRenderer::setEffectVec4f(EffectHandle e, const char *name, Vec4 value)
	{
		const int loc = res_->uniformLocation(e, name);
		if (loc >= 0)
		{
			const float arr[4] = {value.x, value.y, value.z, value.w};
			SetShaderValue(res_->shaders[e.id], loc, arr, SHADER_UNIFORM_VEC4);
		}
	}

	void RaylibRenderer::setEffectVec4(EffectHandle e, const char *name, Color value)
	{
		const int loc = res_->uniformLocation(e, name);
		if (loc >= 0)
		{
			const float arr[4] = {
				value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
			SetShaderValue(res_->shaders[e.id], loc, arr, SHADER_UNIFORM_VEC4);
		}
	}

	// ---- Off-screen render targets ----

	void RaylibRenderer::suspendRenderState()
	{
		if (clipActive_)
			EndScissorMode();
		rlDrawRenderBatchActive();
		for (size_t i = 0; i < transformStack_.size(); ++i)
			rlPopMatrix();
	}

	void RaylibRenderer::restoreRenderState()
	{
		for (const auto &tr : transformStack_)
			pushRlTransform(tr);
		if (clipActive_)
			applyScissor(currentClip_);
	}

	TargetHandle RaylibRenderer::createTarget(int w, int h)
	{
		// Creating an FBO binds it internally; suspend any active target first.
		if (insideTarget_)
		{
			suspendRenderState();
			EndTextureMode();
		}

		const uint32_t id = res_->nextId++;
		res_->targets[id] = loadColorOnlyRenderTexture(w, h);

		if (insideTarget_)
		{
			if (auto it = res_->targets.find(currentTarget_.id); it != res_->targets.end())
				BeginTextureMode(it->second);
			restoreRenderState();
		}

		return {id, w, h};
	}

	void RaylibRenderer::destroyTarget(TargetHandle t)
	{
		if (auto it = res_->targets.find(t.id); it != res_->targets.end())
		{
			UnloadRenderTexture(it->second);
			res_->targets.erase(it);
		}
	}

	void RaylibRenderer::pushTarget(TargetHandle t)
	{
		auto it = res_->targets.find(t.id);
		if (it == res_->targets.end())
			return;

		suspendRenderState();
		if (insideTarget_)
			EndTextureMode();

		targetStates_.push_back({
			currentTarget_,
			clipActive_,
			currentClip_,
			std::move(clipStack_),
			std::move(transformStack_),
			currentTargetW_,
			currentTargetH_});

		clipActive_ = false;
		clipStack_.clear();
		currentClip_ = {0.0f, 0.0f, static_cast<float>(t.width), static_cast<float>(t.height)};
		transformStack_.clear();

		currentTarget_ = t;
		currentTargetW_ = t.width;
		currentTargetH_ = t.height;
		insideTarget_ = true;

		BeginTextureMode(it->second);
		ClearBackground(::BLANK);
	}

	void RaylibRenderer::popTarget()
	{
		suspendRenderState();
		EndTextureMode();

		if (!targetStates_.empty())
		{
			auto prev = std::move(targetStates_.back());
			targetStates_.pop_back();
			currentTarget_ = prev.target;
			clipActive_ = prev.clipActive;
			currentClip_ = prev.currentClip;
			clipStack_ = std::move(prev.clipStack);
			transformStack_ = std::move(prev.transformStack);
			currentTargetW_ = prev.targetW;
			currentTargetH_ = prev.targetH;
		}
		else
		{
			currentTarget_ = {};
			clipActive_ = false;
			clipStack_.clear();
			currentClip_ = {0.0f, 0.0f,
							static_cast<float>(GetScreenWidth()) / dpiScale_,
							static_cast<float>(GetScreenHeight()) / dpiScale_};
			transformStack_.clear();
			currentTargetW_ = 0;
			currentTargetH_ = 0;
		}

		insideTarget_ = currentTarget_.valid();

		if (insideTarget_)
		{
			if (auto it = res_->targets.find(currentTarget_.id); it != res_->targets.end())
				BeginTextureMode(it->second);
		}

		restoreRenderState();
	}

	void RaylibRenderer::drawTarget(TargetHandle t, Rect dst, Color tint)
	{
		auto it = res_->targets.find(t.id);
		if (it == res_->targets.end())
			return;
		// OpenGL FBO textures are stored bottom-up, hence the negative source height.
		const ::Texture2D tex = it->second.texture;
		const ::Rectangle src = {0.0f, 0.0f, static_cast<float>(tex.width), -static_cast<float>(tex.height)};
		DrawTexturePro(tex, src, toRl(dst), {0.0f, 0.0f}, 0.0f, toRl(tint));
	}

	void RaylibRenderer::drawTarget(TargetHandle t, Rect src, Rect dst, Color tint)
	{
		auto it = res_->targets.find(t.id);
		if (it == res_->targets.end())
			return;
		const ::Texture2D tex = it->second.texture;
		const ::Rectangle rlSrc = {
			src.x,
			static_cast<float>(tex.height) - src.y - src.height,
			src.width,
			-src.height};
		DrawTexturePro(tex, rlSrc, toRl(dst), {0.0f, 0.0f}, 0.0f, toRl(tint));
	}

	bool RaylibRenderer::inTarget() const
	{
		return insideTarget_;
	}

	void RaylibRenderer::clearTarget(TargetHandle t, Color c)
	{
		if (auto it = res_->targets.find(t.id); it != res_->targets.end())
		{
			BeginTextureMode(it->second);
			ClearBackground(toRl(c));
			EndTextureMode();
		}
	}

	// ---- Native Raylib resource registration ----

	TextureHandle RaylibRenderer::registerTexture(const ::Texture2D &t)
	{
		const uint32_t id = res_->nextId++;
		res_->textures[id] = t;
		return {id, t.width, t.height};
	}

	FontHandle RaylibRenderer::registerFont(const ::Font &f)
	{
		const uint32_t id = res_->nextId++;
		res_->fonts[id] = f;
		return {id};
	}

	EffectHandle RaylibRenderer::registerEffect(const ::Shader &s)
	{
		const uint32_t id = res_->nextId++;
		res_->shaders[id] = s;
		return {id};
	}

	void RaylibRenderer::unregisterFont(FontHandle h)
	{
		if (auto it = res_->fonts.find(h.id); it != res_->fonts.end())
		{
			UnloadFont(it->second);
			res_->fonts.erase(it);
		}
	}

	void RaylibRenderer::unregisterTexture(TextureHandle h)
	{
		if (auto it = res_->textures.find(h.id); it != res_->textures.end())
		{
			UnloadTexture(it->second);
			res_->textures.erase(it);
		}
	}

	void RaylibRenderer::unregisterEffect(EffectHandle h)
	{
		if (auto it = res_->shaders.find(h.id); it != res_->shaders.end())
		{
			UnloadShader(it->second);
			res_->shaders.erase(it);
			res_->shaderUniforms.erase(h.id);
		}
	}

	// =========================================================================
	// 3. RaylibAssetProvider (IAssetProvider)
	// =========================================================================

	FontHandle RaylibAssetProvider::bakeAndRegisterFont(const std::string &name,
														const char *filePath,
														int baseSize,
														const CodepointSet &codepoints)
	{
		auto cps = codepoints.build();
		int *cpData = cps.empty() ? nullptr : cps.data();
		const int cpCount = static_cast<int>(cps.size());

		::Font f = LoadFontEx(filePath, baseSize, cpData, cpCount);
		if (f.texture.id == 0)
			return {};

		SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
		const FontHandle h = renderer_.registerFont(f);
		fonts_[name] = h;
		return h;
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
		if (fonts_.find(name) != fonts_.end())
		{
			logWarn("Assets", "", 0, 0, "font '" + name + "' already loaded");
			return false;
		}

		if (bakeAndRegisterFont(name, path, baseSize, codepoints).id == 0)
		{
			logError("Assets", "", 0, 0,
					 "failed to load font '" + name + "' from '" + path + "'");
			return false;
		}
		return true;
	}

	FontHandle RaylibAssetProvider::loadFont(std::string_view name, std::string_view path)
	{
		const std::string key(name);
		if (!loadFont(key, std::string(path).c_str(), defaultFontSize_, defaultCodepoints_))
			return {};
		return fonts_[key];
	}

	FontHandle RaylibAssetProvider::loadFontFromMemory(std::string_view name,
													   const void *data, size_t size)
	{
		return loadFontFromMemory(name, data, size, defaultFontSize_, defaultCodepoints_);
	}

	FontHandle RaylibAssetProvider::loadFontFromMemory(std::string_view name,
													   const void *data, size_t size,
													   int baseSize,
													   const CodepointSet &codepoints)
	{
		if (name.empty() || !data || size == 0)
			return {};

		const std::string key(name);
		if (auto it = fonts_.find(key); it != fonts_.end())
		{
			logWarn("Assets", "", 0, 0, "font '" + key + "' already loaded");
			return it->second;
		}

		// Write to a temporary file so LoadFontEx works identically across Raylib versions.
		const std::filesystem::path tmp =
			std::filesystem::temp_directory_path() / ("zenit_font_" + key + ".ttf");
		{
			std::ofstream out(tmp, std::ios::binary);
			if (!out)
			{
				logError("Assets", "", 0, 0,
						 "failed to open temp file for font '" + key + "'");
				return {};
			}
			out.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
		}

		const FontHandle h = bakeAndRegisterFont(key, tmp.string().c_str(), baseSize, codepoints);
		std::error_code ec;
		std::filesystem::remove(tmp, ec);

		if (h.id == 0)
			logError("Assets", "", 0, 0, "failed to load font '" + key + "' from memory");
		return h;
	}

	void RaylibAssetProvider::unloadFont(std::string_view name)
	{
		if (auto it = fonts_.find(name); it != fonts_.end())
		{
			renderer_.unregisterFont(it->second);
			fonts_.erase(it);
		}
	}

	bool RaylibAssetProvider::loadTexture(std::string name, const char *path)
	{
		if (name.empty() || !path || textures_.find(name) != textures_.end())
			return false;
		const ::Texture2D t = LoadTexture(path);
		if (t.id == 0)
			return false;
		textures_[std::move(name)] = renderer_.registerTexture(t);
		return true;
	}

	bool RaylibAssetProvider::loadEffect(std::string name, const char *fsPath, const char *vsPath)
	{
		if (name.empty() || !fsPath || effects_.find(name) != effects_.end())
			return false;
		const ::Shader s = LoadShader(vsPath, fsPath);
		if (s.id == 0)
			return false;
		effects_[std::move(name)] = renderer_.registerEffect(s);
		return true;
	}

	TextureHandle RaylibAssetProvider::loadTexture(std::string_view name, std::string_view path)
	{
		const std::string key(name);
		if (!loadTexture(key, std::string(path).c_str()))
			return getTexture(name);
		return textures_[key];
	}

	void RaylibAssetProvider::unloadTexture(std::string_view name)
	{
		if (auto it = textures_.find(name); it != textures_.end())
		{
			renderer_.unregisterTexture(it->second);
			textures_.erase(it);
		}
	}

	EffectHandle RaylibAssetProvider::loadEffect(std::string_view name,
												 std::string_view fsPath,
												 std::string_view vsPath)
	{
		const std::string key(name);
		const std::string fsStr(fsPath);
		const std::string vsStr(vsPath);
		const char *vsPtr = vsStr.empty() ? nullptr : vsStr.c_str();
		if (!loadEffect(key, fsStr.c_str(), vsPtr))
			return getEffect(name);
		return effects_[key];
	}

	void RaylibAssetProvider::unloadEffect(std::string_view name)
	{
		if (auto it = effects_.find(name); it != effects_.end())
		{
			renderer_.unregisterEffect(it->second);
			effects_.erase(it);
		}
	}

	// =========================================================================
	// 4. RaylibBackend (IBackend)
	// =========================================================================

	bool RaylibBackend::init(const WindowConfig &config)
	{
		if (initialized_)
			return true;

		unsigned int flags = 0;
		if (config.resizable)
			flags |= FLAG_WINDOW_RESIZABLE;
		if (config.msaa)
			flags |= FLAG_MSAA_4X_HINT;
		if (config.vsync)
			flags |= FLAG_VSYNC_HINT;

		SetConfigFlags(flags);
		InitWindow(config.width, config.height, config.title.c_str());
		if (!IsWindowReady())
			return false;

		if (config.minWidth > 0 && config.minHeight > 0)
			SetWindowMinSize(config.minWidth, config.minHeight);
		if (config.targetFps > 0)
			SetTargetFPS(config.targetFps);

		platform_ = std::make_unique<RaylibPlatform>();
		renderer_ = std::make_unique<RaylibRenderer>();
		assets_ = std::make_unique<RaylibAssetProvider>(*renderer_);

		bindToContext();
		initialized_ = true;
		return true;
	}

	void RaylibBackend::shutdown()
	{
		if (!initialized_)
			return;

		auto &ctx = UIContext::get();
		if (renderer_)
			ctx.targetPool.clear(*renderer_, &ctx.renderStats);

		unbindFromContext();
		assets_.reset();
		renderer_.reset();
		platform_.reset();
		CloseWindow();
		initialized_ = false;
	}

	bool RaylibBackend::pollFrame()
	{
		return initialized_ && !WindowShouldClose();
	}

	float RaylibBackend::deltaTime() const
	{
		return GetFrameTime();
	}

	void RaylibBackend::beginDraw(Color clearColor)
	{
		BeginDrawing();
		ClearBackground(toRl(clearColor));
	}

	void RaylibBackend::endDraw()
	{
		EndDrawing();
	}

} // namespace ZenitUI