#pragma once

// @backend_name     Mock (Headless)
// @backend_target   ZenitUI::MockBackend
// @backend_role     Deterministic headless backend & test fixture (no GPU/window)
// @backend_features Configurable (Effects, NestedTargets, TargetSubRect)
// @backend_classes  ZenitUI::Test::MockPlatform ZenitUI::Test::MockRenderer ZenitUI::Test::MockAssetProvider ZenitUI::Test::Env

/**
 * \file MockBackend.hpp
 * \brief Headless mock backend (`MockRenderer`, `MockPlatform`, `MockAssetProvider`)
 *        and RAII test harness (`ZenitUI::Test::Env`).
 *
 * Serves a dual purpose:
 *  1. **Unit-test harness:** drives ZenitUI's full frame cycle (`updateTree`,
 *     `measure`, `arrange`, `draw`) deterministically without opening a window
 *     or GPU context, recording draw calls, clip rects, transforms, and shader
 *     uniforms for assertions.
 *  2. **Minimal backend skeleton:** demonstrates the smallest contract an
 *     `IPlatform` / `IRenderer` / `IAssetProvider` triplet must satisfy.
 *     Notice in particular:
 *       - `IRenderer::supports(Feature)` lets a minimal backend opt out of
 *         shaders and off-screen targets simply by returning `false`.
 *       - `IPlatform::pointer()` and `pollInputEvents()` must clear edge-triggered
 *         fields (`pressed`, `released`, `wheelY`, `keys`, `chars`) once polled.
 *       - `IRenderer::pushClip()` / `popClip()` must maintain an intersected
 *         clip stack so `getClipRect()` remains accurate for culling.
 */

#include "ZenitUI.hpp"
#include "Utf8.hpp"
#include "Backend.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ZenitUI::Test
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	// =========================================================================
	// 1. Headless Renderer (IRenderer)
	// =========================================================================

	/**
	 * \class MockRenderer
	 * \ingroup grp_backends
	 * \brief Recording `IRenderer` that tracks draw counts, text metrics, clip
	 *        stacks, render targets, and shader uniforms without a GPU.
	 */
	class MockRenderer : public IRenderer
	{
	public:
		/** \brief Recorded shader uniform assignment captured by `setEffect*`. */
		template <typename T>
		struct CapturedUniform
		{
			std::string name;
			T value;
		};

		using CapturedFloat = CapturedUniform<float>;
		using CapturedVec2 = CapturedUniform<Vec2>;
		using CapturedVec3 = CapturedUniform<Vec3>;
		using CapturedVec4 = CapturedUniform<Vec4>;
		using CapturedColor = CapturedUniform<Color>;

		/// \name Configuration & Feature Flags
		/// @{
		float charWidth{8.0f}; ///< Fixed monospace advance used by `measureText`.
		bool supportsEffects{false};
		bool supportsNestedTargets{false};
		bool supportsTargetSubRect{false};
		/// @}

		/// \name Recorded Inspection State
		/// @{
		int fillRectCount{0};
		int drawTextCount{0};
		int pushClipCount{0};
		int popClipCount{0};
		int pushEffectCount{0};
		int createTargetCount{0};
		int destroyTargetCount{0};
		int pushTargetCount{0};
		int liveTargets{0};
		int activeTargetDepth{0};
		int lastCreatedTargetW{0};
		int lastCreatedTargetH{0};

		EffectHandle lastPushedEffect{};
		Rect lastDrawTargetSrc{};
		Rect lastDrawTargetDst{};

		std::vector<std::string> drawnTexts;
		std::vector<Rect> clipStack;
		Rect currentClip{0, 0, 1e9f, 1e9f};
		std::vector<Transform2D> transformHistory;

		std::vector<CapturedFloat> capturedFloat;
		std::vector<CapturedVec2> capturedVec2;
		std::vector<CapturedVec3> capturedVec3;
		std::vector<CapturedVec4> capturedVec4f;
		std::vector<CapturedColor> capturedColor;
		/// @}

		/** \brief Clear all counters, histories, and feature flags to defaults. */
		void reset()
		{
			fillRectCount = 0;
			drawTextCount = 0;
			pushClipCount = 0;
			popClipCount = 0;
			pushEffectCount = 0;
			createTargetCount = 0;
			destroyTargetCount = 0;
			pushTargetCount = 0;
			liveTargets = 0;
			activeTargetDepth = 0;
			lastCreatedTargetW = 0;
			lastCreatedTargetH = 0;

			lastPushedEffect = {};
			lastDrawTargetSrc = {};
			lastDrawTargetDst = {};

			supportsEffects = false;
			supportsNestedTargets = false;
			supportsTargetSubRect = false;

			drawnTexts.clear();
			clipStack.clear();
			currentClip = {0, 0, 1e9f, 1e9f};
			transformHistory.clear();

			capturedFloat.clear();
			capturedVec2.clear();
			capturedVec3.clear();
			capturedVec4f.clear();
			capturedColor.clear();
		}

		/// \name Frame Lifecycle & Capabilities
		/// @{
		void beginFrame() override {}
		void endFrame() override {}
		void setDpiScale(float) override {}

		bool supports(Feature f) const override
		{
			switch (f)
			{
			case Feature::Effects:
				return supportsEffects;
			case Feature::NestedTargets:
				return supportsNestedTargets;
			case Feature::TargetSubRect:
				return supportsTargetSubRect;
			default:
				return false;
			}
		}
		/// @}

		/// \name 2D Primitives & Textures
		/// @{
		void fillRect(Rect, Color) override { ++fillRectCount; }
		void fillRoundedRect(Rect, float, Color) override {}
		void fillCircle(Vec2, float, Color) override {}
		void strokeRect(Rect, float, Color) override {}
		void strokeRoundedRect(Rect, float, float, Color) override {}
		void drawTexture(TextureHandle, Rect, Rect, Color) override {}
		void drawNineSlice(TextureHandle, NineSlice, Rect, Color) override {}
		/// @}

		/// \name Text Rendering & Metrics
		/// @{
		void drawText(FontHandle, std::string_view s, Vec2, float, float, Color) override
		{
			++drawTextCount;
			drawnTexts.emplace_back(s);
		}

		Vec2 measureText(FontHandle, std::string_view s, float size, float spacing) override
		{
			const size_t count = Utf8::codepointCount(s);
			float w = charWidth * static_cast<float>(count);
			if (count > 0)
				w += spacing * static_cast<float>(count - 1);
			return {w, size};
		}
		/// @}

		/// \name Transform, Clip & Effect Stacks
		/// @{
		void pushTransform(const Transform2D &t) override { transformHistory.push_back(t); }
		void popTransform() override {}

		void pushEffect(EffectHandle e) override
		{
			++pushEffectCount;
			lastPushedEffect = e;
		}
		void popEffect() override {}

		void pushClip(Rect r) override
		{
			++pushClipCount;
			clipStack.push_back(currentClip);
			const float x1 = std::max(currentClip.x, r.x);
			const float y1 = std::max(currentClip.y, r.y);
			const float x2 = std::min(currentClip.x + currentClip.width, r.x + r.width);
			const float y2 = std::min(currentClip.y + currentClip.height, r.y + r.height);
			currentClip = {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
		}

		void popClip() override
		{
			++popClipCount;
			if (!clipStack.empty())
			{
				currentClip = clipStack.back();
				clipStack.pop_back();
			}
		}

		Rect getClipRect() const override { return currentClip; }
		/// @}

		/// \name Off-Screen Render Targets
		/// @{
		TargetHandle createTarget(int w, int h) override
		{
			++createTargetCount;
			++liveTargets;
			lastCreatedTargetW = w;
			lastCreatedTargetH = h;
			return TargetHandle{nextTargetId_++, w, h};
		}

		void destroyTarget(TargetHandle t) override
		{
			if (t.valid())
			{
				++destroyTargetCount;
				if (liveTargets > 0)
					--liveTargets;
			}
		}

		void pushTarget(TargetHandle) override
		{
			++pushTargetCount;
			++activeTargetDepth;
		}

		void popTarget() override
		{
			if (activeTargetDepth > 0)
				--activeTargetDepth;
		}

		void drawTarget(TargetHandle t, Rect dst, Color) override
		{
			lastDrawTargetSrc = {0.0f, 0.0f, static_cast<float>(t.width), static_cast<float>(t.height)};
			lastDrawTargetDst = dst;
		}

		void drawTarget(TargetHandle t, Rect src, Rect dst, Color) override
		{
			if (supportsTargetSubRect)
			{
				lastDrawTargetSrc = src;
				lastDrawTargetDst = dst;
			}
			else
			{
				drawTarget(t, dst, Colors::White);
			}
		}

		bool inTarget() const override { return activeTargetDepth > 0; }
		void clearTarget(TargetHandle, Color) override {}
		/// @}

		/// \name Shader Uniforms
		/// @{
		void setEffectFloat(EffectHandle, const char *name, float value) override
		{
			capturedFloat.push_back({name, value});
		}
		void setEffectVec2(EffectHandle, const char *name, Vec2 value) override
		{
			capturedVec2.push_back({name, value});
		}
		void setEffectVec3(EffectHandle, const char *name, Vec3 value) override
		{
			capturedVec3.push_back({name, value});
		}
		void setEffectVec4f(EffectHandle, const char *name, Vec4 value) override
		{
			capturedVec4f.push_back({name, value});
		}
		void setEffectVec4(EffectHandle, const char *name, Color value) override
		{
			capturedColor.push_back({name, value});
		}
		/// @}

	private:
		uint32_t nextTargetId_{1000};
	};

	// =========================================================================
	// 2. Headless Platform (IPlatform)
	// =========================================================================

	/**
	 * \class MockPlatform
	 * \ingroup grp_backends
	 * \brief Programmable `IPlatform` for injecting deterministic pointer,
	 *        keyboard, viewport, and clipboard state in tests.
	 */
	class MockPlatform : public IPlatform
	{
	public:
		Vec2 viewport{800.0f, 600.0f};
		double currentTime{0.0};
		ModifierState mods{};
		PointerState nextPointer{};
		InputEvents nextEvents{};
		EdgeInsets safe{};
		std::string clipboardData;

		/** \brief Restore viewport, clock, and queued input events to defaults. */
		void reset()
		{
			viewport = {800.0f, 600.0f};
			currentTime = 0.0;
			mods = {};
			nextPointer = {};
			nextEvents = {};
			safe = {};
		}

		/// \name IPlatform Overrides
		/// @{
		Vec2 viewportSize() override { return viewport; }
		double time() override { return currentTime; }
		float dpiScale() override { return 1.0f; }
		EdgeInsets safeArea() override { return safe; }
		ModifierState modifiers() override { return mods; }

		PointerState pointer() override
		{
			PointerState polled = nextPointer;
			// Edge-triggered states last for a single frame only.
			nextPointer.wheelY = 0.0f;
			for (auto &b : nextPointer.buttons)
			{
				b.pressed = false;
				b.released = false;
			}
			return polled;
		}

		InputEvents pollInputEvents() override
		{
			InputEvents polled = std::move(nextEvents);
			nextEvents = {};
			return polled;
		}

		void setClipboardText(const std::string &text) override { clipboardData = text; }
		std::string getClipboardText() override { return clipboardData; }
		/// @}

		/// \name Test Input Injection Helpers
		/// @{
		void moveMouse(Vec2 pos)
		{
			nextPointer.pos = pos;
			for (auto &b : nextPointer.buttons)
				b = {};
		}

		void pressLeft(Vec2 pos) { setButton(MouseButton::Left, pos, true, true, false); }
		void holdLeft(Vec2 pos) { setButton(MouseButton::Left, pos, true, false, false); }
		void releaseLeft(Vec2 pos) { setButton(MouseButton::Left, pos, false, false, true); }

		void pressRight(Vec2 pos) { setButton(MouseButton::Right, pos, true, true, false); }
		void releaseRight(Vec2 pos) { setButton(MouseButton::Right, pos, false, false, true); }

		void pressMiddle(Vec2 pos) { setButton(MouseButton::Middle, pos, true, true, false); }
		void holdMiddle(Vec2 pos) { setButton(MouseButton::Middle, pos, true, false, false); }
		void releaseMiddle(Vec2 pos) { setButton(MouseButton::Middle, pos, false, false, true); }

		void scrollWheel(float delta) { nextPointer.wheelY = delta; }
		void pressKey(int key) { nextEvents.keys.push_back(key); }
		void holdKey(int key) { nextEvents.held.push_back(key); }
		void typeChar(int codepoint) { nextEvents.chars.push_back(codepoint); }
		/// @}

	private:
		void setButton(MouseButton btn, Vec2 pos, bool down, bool pressed, bool released)
		{
			nextPointer.pos = pos;
			nextPointer[btn] = {down, pressed, released};
		}
	};

	// =========================================================================
	// 3. Headless Asset Provider (IAssetProvider)
	// =========================================================================

	/**
	 * \class MockAssetProvider
	 * \ingroup grp_backends
	 * \brief In-memory `IAssetProvider` that mints synthetic font and shader
	 *        handles without touching the filesystem or GPU.
	 */
	class MockAssetProvider : public IAssetProvider
	{
	public:
		using IAssetProvider::getFont;

		/// \name Test Registration Helpers
		/// @{
		void registerEffect(std::string name)
		{
			effects_[std::move(name)] = EffectHandle{nextEffectId_++};
		}

		void registerFont(const std::string &name, FontHandle h)
		{
			fonts_[name] = h;
		}
		/// @}

		/// \name IAssetProvider Overrides
		/// @{

		EffectHandle getEffect(std::string_view name) override
		{
			auto it = effects_.find(std::string(name));
			return it != effects_.end() ? it->second : EffectHandle{};
		}

		FontHandle getFont(std::string_view name) override
		{
			auto it = fonts_.find(std::string(name));
			return it != fonts_.end() ? it->second : FontHandle{};
		}

		FontHandle loadFont(std::string_view name, std::string_view /*path*/) override
		{
			if (name.empty())
				return {};
			const FontHandle h{nextFontId_++};
			fonts_[std::string(name)] = h;
			return h;
		}

		FontHandle loadFontFromMemory(std::string_view name,
									  const void * /*data*/, size_t /*size*/) override
		{
			return loadFont(name, "");
		}

		void unloadFont(std::string_view name) override
		{
			fonts_.erase(std::string(name));
		}

		void setDefaultFontSize(int baseSize) override { defaultFontSize_ = baseSize; }
		void addDefaultCodepointRange(uint32_t, uint32_t) override {}

		TextureHandle loadTexture(std::string_view name, std::string_view /*path*/) override
		{
			if (name.empty())
				return {};
			const TextureHandle h{nextTextureId_++, 64, 64};
			textures_[std::string(name)] = h;
			return h;
		}

		void unloadTexture(std::string_view name) override
		{
			textures_.erase(std::string(name));
		}

		TextureHandle getTexture(std::string_view name) override
		{
			auto it = textures_.find(std::string(name));
			return it != textures_.end() ? it->second : TextureHandle{};
		}

		EffectHandle loadEffect(std::string_view name,
								std::string_view /*fsPath*/,
								std::string_view /*vsPath*/ = {}) override
		{
			if (name.empty())
				return {};
			const EffectHandle h{nextEffectId_++};
			effects_[std::string(name)] = h;
			return h;
		}

		void unloadEffect(std::string_view name) override
		{
			effects_.erase(std::string(name));
		}

		/// @}

	private:
		std::unordered_map<std::string, EffectHandle> effects_;
		std::unordered_map<std::string, FontHandle> fonts_;
		std::unordered_map<std::string, TextureHandle> textures_;
		int defaultFontSize_{96};
		uint32_t nextEffectId_{100};
		uint32_t nextFontId_{500};
		uint32_t nextTextureId_{300};
	};

	// =========================================================================
	// 4. RAII Headless Test Environment
	// =========================================================================

	/**
	 * \struct Env
	 * \ingroup grp_backends
	 * \brief Scoped test fixture that binds `MockRenderer`, `MockPlatform`,
	 *        and `MockAssetProvider` into `UIContext` and restores clean state
	 *        on destruction.
	 */
	struct Env
	{
		MockRenderer renderer;
		MockPlatform platform;
		MockAssetProvider assets;

		Env()
		{
			auto &ctx = UIContext::get();
			ctx.renderer = &renderer;
			ctx.platform = &platform;
			ctx.assets = &assets;
			reset();
		}

		~Env()
		{
			auto &ctx = UIContext::get();
			ctx.targetPool.clear(renderer, &ctx.renderStats);
			ctx.renderer = nullptr;
			ctx.platform = nullptr;
			ctx.assets = nullptr;
			Metrics::viewport = {1920.0f, 1080.0f};
		}

		/** \brief Reset global `UIContext`, `Theme`, and mock backend state. */
		void reset()
		{
			Theme::get().clear();

			auto &ctx = UIContext::get();
			ctx.focusedNode.reset();
			ctx.pointerCapture.reset();
			ctx.topmostConsumer = nullptr;
			ctx.pressTarget = nullptr;
			ctx.releaseTarget = nullptr;
			ctx.buttonConsumed.fill(false);
			ctx.wheelConsumedThisFrame = false;
			ctx.pointer = {};
			ctx.inputEvents = {};
			ctx.dt = 0.0f;
			ctx.time = 0.0;
			ctx.activePortals.clear();
			ctx.framePortals.clear();
			ctx.cullingClipStack.clear();
			ctx.targetPool.clear(renderer, &ctx.renderStats);
			ctx.renderStats.resetAll();

			Metrics::viewport = {1920.0f, 1080.0f};

			platform.reset();
			renderer.reset();
		}

		/** \brief Advance \p root through one complete update/layout/draw frame. */
		void frame(std::shared_ptr<Layout> root, float dt = 1.0f / 60.0f)
		{
			auto &ctx = UIContext::get();
			ctx.beginFrame(dt);
			root->updateTree(dt);
			root->measure(platform.viewport.x, platform.viewport.y);
			root->arrange({0.0f, 0.0f, platform.viewport.x, platform.viewport.y});
			root->draw();
			ctx.endFrame();
		}

		/** \brief Simulate a complete left-button press and release at \p pos. */
		void click(std::shared_ptr<Layout> root, Vec2 pos)
		{
			platform.pressLeft(pos);
			frame(root);
			platform.releaseLeft(pos);
			frame(root);
		}
	};

	/**
	 * \class MockBackend
	 * \ingroup grp_backends
	 * \brief Headless `IBackend` host wrapping `MockPlatform`, `MockRenderer`, and `MockAssetProvider`.
	 */
	class MockBackend : public IBackend
	{
	public:
		MockPlatform mockPlatform;
		MockRenderer mockRenderer;
		MockAssetProvider mockAssets;
		float fixedDt{1.0f / 60.0f};
		int maxFrames{1};

		bool init(const WindowConfig &config) override
		{
			mockPlatform.viewport = {static_cast<float>(config.width), static_cast<float>(config.height)};
			framesPolled_ = 0;
			running_ = true;
			bindToContext();
			return true;
		}

		void shutdown() override
		{
			auto &ctx = UIContext::get();
			ctx.targetPool.clear(mockRenderer, &ctx.renderStats);
			unbindFromContext();
			running_ = false;
		}

		bool pollFrame() override
		{
			if (!running_ || (maxFrames >= 0 && framesPolled_ >= maxFrames))
				return false;
			++framesPolled_;
			return true;
		}

		float deltaTime() const override { return fixedDt; }
		void beginDraw(Color) override {}
		void endDraw() override {}
		std::string_view name() const override { return "Mock (Headless)"; }

		IPlatform &platform() override { return mockPlatform; }
		IRenderer &renderer() override { return mockRenderer; }
		IAssetProvider &assets() override { return mockAssets; }

	private:
		int framesPolled_{0};
		bool running_{false};
	};

	/** @} */
} // namespace ZenitUI::Test