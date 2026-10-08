#pragma once

#include "UIContext.hpp"
#include "Layout.hpp"
#include "Theme.hpp"
#include "StyleParser.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace ZenitUI::Test
{

    // ============================================================
    //  MockRenderer
    // ============================================================

    class MockRenderer : public IRenderer
    {
    public:
        // Contatori per le assert (non tutti usati, ma a portata)
        int fillRectCount{0};
        int drawTextCount{0};
        int pushClipCount{0};
        int popClipCount{0};

        // Dimensione fissa per il testo: 8px per char + spacing
        float charWidth{8.0f};

        // Clip stack
        std::vector<Rect> clipStack;
        Rect currentClip{0, 0, 1e9f, 1e9f};

        void fillRect(Rect, Color) override { fillRectCount++; }
        void fillRoundedRect(Rect, float, Color) override {}
        void fillCircle(Vec2, float, Color) override {}

        void strokeRect(Rect, float, Color) override {}
        void strokeRoundedRect(Rect, float, float, Color) override {}

        void drawTexture(TextureHandle, Rect, Rect, Color) override {}
        void drawNineSlice(TextureHandle, NineSlice, Rect, Color) override {}

        void drawText(FontHandle, std::string_view, Vec2, float, float, Color) override
        {
            drawTextCount++;
        }
        Vec2 measureText(FontHandle, std::string_view s, float size, float spacing) override
        {
            float w = charWidth * (float)s.size();
            if (!s.empty())
                w += spacing * (float)(s.size() - 1);
            return {w, size};
        }

        void pushTransform(const Transform2D &) override {}
        void popTransform() override {}
        void pushEffect(EffectHandle) override {}
        void popEffect() override {}

        void pushClip(Rect r) override
        {
            pushClipCount++;
            clipStack.push_back(currentClip);
            // Intersezione AABB con il clip corrente
            float x1 = std::max(currentClip.x, r.x);
            float y1 = std::max(currentClip.y, r.y);
            float x2 = std::min(currentClip.x + currentClip.width, r.x + r.width);
            float y2 = std::min(currentClip.y + currentClip.height, r.y + r.height);
            currentClip = {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
        }
        void popClip() override
        {
            popClipCount++;
            if (!clipStack.empty())
            {
                currentClip = clipStack.back();
                clipStack.pop_back();
            }
        }

        Rect getClipRect() const override { return currentClip; }

        TargetHandle createTarget(int, int) override { return {}; }
        void destroyTarget(TargetHandle) override {}
        void pushTarget(TargetHandle) override {}
        void popTarget() override {}
        void drawTarget(TargetHandle, Rect, Color) override {}

        // Effetti shader
        void setEffectFloat(EffectHandle, const char *, float) override {}
        void setEffectVec2(EffectHandle, const char *, Vec2) override {}
        void setEffectVec4(EffectHandle, const char *, Color) override {}
        bool inTarget() const override { return false; }
        void clearTarget(TargetHandle, Color) override {}

        // Ciclo di frame (se hai aggiunto beginFrame/endFrame/setDpiScale a IRenderer)
        void beginFrame() override {}
        void endFrame() override {}
        void setDpiScale(float) override {}

        bool supports(Feature) const override { return false; }
    };

    // ============================================================
    //  MockPlatform
    // ============================================================

    class MockPlatform : public IPlatform
    {
    public:
        Vec2 viewport{800.0f, 600.0f};
        double currentTime{0.0};
        bool shiftDown{false};

        PointerState nextPointer;
        InputEvents nextEvents;

        Vec2 viewportSize() override { return viewport; }
        double time() override { return currentTime; }
        bool shiftHeld() override { return shiftDown; }

        PointerState pointer() override
        {
            PointerState p = nextPointer;
            // Auto-clear di pressed/released: durano un solo frame
            nextPointer.pressed = false;
            nextPointer.released = false;
            nextPointer.rightPressed = false;
            nextPointer.rightReleased = false;
            return p;
        }
        InputEvents pollInputEvents() override
        {
            InputEvents e = nextEvents;
            nextEvents = {};
            return e;
        }

        float dpiScale() override {
            return 1.0f;
        }

        // --- Scripting helpers ---
        void moveMouse(Vec2 pos)
        {
            nextPointer.pos = pos;
            nextPointer.down = false;
            nextPointer.pressed = false;
            nextPointer.released = false;
        }
        void pressLeft(Vec2 pos)
        {
            nextPointer.pos = pos;
            nextPointer.down = true;
            nextPointer.pressed = true;
            nextPointer.released = false;
        }
        void releaseLeft(Vec2 pos)
        {
            nextPointer.pos = pos;
            nextPointer.down = false;
            nextPointer.pressed = false;
            nextPointer.released = true;
        }
        void holdLeft(Vec2 pos)
        {
            // Frame successivi al press: still down, no edge
            nextPointer.pos = pos;
            nextPointer.down = true;
            nextPointer.pressed = false;
            nextPointer.released = false;
        }
        void pressRight(Vec2 pos)
        {
            nextPointer.pos = pos;
            nextPointer.rightDown = true;
            nextPointer.rightPressed = true;
            nextPointer.rightReleased = false;
        }
        void releaseRight(Vec2 pos)
        {
            nextPointer.pos = pos;
            nextPointer.rightDown = false;
            nextPointer.rightPressed = false;
            nextPointer.rightReleased = true;
        }
        void pressKey(int key)
        {
            nextEvents.keys.push_back(key);
        }
        void typeChar(int c)
        {
            nextEvents.chars.push_back(c);
        }
    };

    // ============================================================
    //  MockAssetProvider
    // ============================================================

    class MockAssetProvider : public IAssetProvider
    {
    public:
        FontHandle getFont(std::string_view) override { return {}; }
        TextureHandle getTexture(std::string_view) override { return {}; }
        EffectHandle getEffect(std::string_view) override { return {}; }
    };

    // ============================================================
    //  Fixture
    // ============================================================

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
            // Non possiamo lasciare il UIContext puntato a oggetti distrutti
            auto &ctx = UIContext::get();
            ctx.renderer = nullptr;
            ctx.platform = nullptr;
            ctx.assets = nullptr;
            Metrics::viewport = {1920.0f, 1080.0f};
        }

        void reset()
        {
            Theme::get().clear();
            auto &ctx = UIContext::get();
            ctx.focusedNode.reset();
            ctx.pointerCapture.reset();
            ctx.topmostConsumer = nullptr;
            ctx.pressTarget = nullptr;
            ctx.releaseTarget = nullptr;
            ctx.clickConsumed = false;
            ctx.rightClickConsumed = false;
            ctx.wheelConsumedThisFrame = false;
            ctx.pointer = {};
            ctx.inputEvents = {};
            ctx.dt = 0.0f;
            ctx.time = 0.0;
            ctx.activePortals.clear();
            ctx.framePortals.clear();

            platform.viewport = {800.0f, 600.0f};
            platform.currentTime = 0.0;
            platform.shiftDown = false;
            platform.nextPointer = {};
            platform.nextEvents = {};

            Metrics::viewport = {1920.0f, 1080.0f};

            renderer.fillRectCount = 0;
            renderer.drawTextCount = 0;
            renderer.pushClipCount = 0;
            renderer.popClipCount = 0;
            renderer.clipStack.clear();
            renderer.currentClip = {0, 0, 1e9f, 1e9f};
        }

        // Esegue un singolo frame di UI: beginFrame + update + measure + arrange.
        void frame(std::shared_ptr<Layout> root, float dt = 1.0f / 60.0f)
        {
            auto &ctx = UIContext::get();
            ctx.beginFrame(dt);
            root->updateTree(dt);
            root->measure(platform.viewport.x, platform.viewport.y);
            root->arrange({0, 0, platform.viewport.x, platform.viewport.y});
            root->draw();
        }

        // Helper: simula un click completo (down + up) al punto indicato.
        void click(std::shared_ptr<Layout> root, Vec2 pos)
        {
            platform.pressLeft(pos);
            frame(root);
            platform.releaseLeft(pos);
            frame(root);
        }
    };

} // namespace ZenitUI::Test