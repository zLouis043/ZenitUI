#include "TestFramework.hpp"
#include "StyleParser.hpp"
#include "Theme.hpp"

// Aggiunti per i test dei filter params (fix 1.4)
#include "Mocks.hpp"
#include "FilterRegistry.hpp"
#include "UIContext.hpp"

using namespace ZenitUI;
using namespace ZenitUI::ZMarkup;

// ============================================================
//  Parser di `effect:`
// ============================================================

TEST(Parser_effect, simple_name)
{
    Theme t;
    loadStyleString(".btn { effect: hueShift; }", t);
    CHECK(t.rules.size() == 1);
    const auto &e = t.rules[0].style.effect;
    CHECK(e.is_set);
    CHECK(e.value == "hueShift");
}

TEST(Parser_effect, empty_not_set)
{
    Theme t;
    loadStyleString(".btn { color: red; }", t);
    CHECK(!t.rules[0].style.effect.is_set);
}

TEST(Parser_effect, coexists_with_filter)
{
    Theme t;
    loadStyleString(".btn { effect: hueShift; filter: blur(2px); }", t);
    CHECK(t.rules[0].style.effect.is_set);
    CHECK(t.rules[0].style.effect.value == "hueShift");
    CHECK(t.rules[0].style.filters.is_set);
    CHECK(t.rules[0].style.filters.value.size() == 1);
}

// ============================================================
//  Filter params: Vec3 / Vec4 non scalano a [0, 255]  (fix 1.4)
// ============================================================

using namespace ZenitUI::Test;

TEST(Filter_vec4, values_are_not_scaled_to_255)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"color", FilterParamType::Vec4, 0, 0.0f}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"0.5", "0.25", "0.75", "1.0"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.src = {};
    ctx.scratch = {};
    ctx.region = {0, 0, 100, 100};
    ctx.dst = {0, 0, 100, 100};
    ctx.opacity = 1.0f;

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedVec4f.size() == 1);
    const auto &cap = env.renderer.capturedVec4f[0];
    CHECK(cap.name == "color");
    CHECK_NEAR(cap.value.x, 0.5f, 1e-6);
    CHECK_NEAR(cap.value.y, 0.25f, 1e-6);
    CHECK_NEAR(cap.value.z, 0.75f, 1e-6);
    CHECK_NEAR(cap.value.w, 1.0f, 1e-6);
}

TEST(Filter_vec3, values_are_not_scaled_to_255)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"dir", FilterParamType::Vec3, 0, 0.0f}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"0.1", "0.2", "0.3"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};
    ctx.opacity = 1.0f;

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedVec3.size() == 1);
    const auto &cap = env.renderer.capturedVec3[0];
    CHECK(cap.name == "dir");
    CHECK_NEAR(cap.value.x, 0.1f, 1e-6);
    CHECK_NEAR(cap.value.y, 0.2f, 1e-6);
    CHECK_NEAR(cap.value.z, 0.3f, 1e-6);
}

TEST(Filter_float, value_parsed_and_px_stripped)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"radius", FilterParamType::Float, 0, 4.0f}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"12px"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedFloat.size() == 1);
    CHECK(env.renderer.capturedFloat[0].name == "radius");
    CHECK_NEAR(env.renderer.capturedFloat[0].value, 12.0f, 1e-6);
}

TEST(Filter_float, default_used_when_no_arg)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"radius", FilterParamType::Float, 0, 4.0f}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {}; // nessun argomento

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedFloat.size() == 1);
    CHECK_NEAR(env.renderer.capturedFloat[0].value, 4.0f, 1e-6);
}

TEST(Filter_vec2, consumes_two_args)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"offset", FilterParamType::Vec2, 0, 0.0f}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"2px", "8px"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};

    applyFilter(def, ctx);

    // Nota: applySinglePass chiama anche setEffectVec2("texSize", ...)
    // automaticamente. Cerchiamo specificamente l'uniform "offset".
    bool found = false;
    for (const auto &c : env.renderer.capturedVec2)
    {
        if (c.name == "offset")
        {
            found = true;
            CHECK_NEAR(c.value.x, 2.0f, 1e-6);
            CHECK_NEAR(c.value.y, 8.0f, 1e-6);
            break;
        }
    }
    CHECK(found);
}

TEST(Filter_color, hex_parsed)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"tint", FilterParamType::Color, 0, Colors::White}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"#FF8000FF"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedColor.size() == 1);
    CHECK(env.renderer.capturedColor[0].name == "tint");
    CHECK(env.renderer.capturedColor[0].value.r == 0xFF);
    CHECK(env.renderer.capturedColor[0].value.g == 0x80);
    CHECK(env.renderer.capturedColor[0].value.b == 0x00);
    CHECK(env.renderer.capturedColor[0].value.a == 0xFF);
}

TEST(Filter_color, named_color_parsed)
{
    Env env;
    env.assets.registerEffect("test");

    FilterDef def;
    def.name = "test";
    def.shader = "test";
    def.pattern = FilterPattern::SinglePass;
    def.params = {FilterParam{"tint", FilterParamType::Color, 0, Colors::White}};

    FilterRef ref;
    ref.name = "test";
    ref.args = {"red"};

    FilterContext ctx;
    ctx.renderer = &env.renderer;
    ctx.ref = &ref;
    ctx.dst = {0, 0, 100, 100};
    ctx.region = {0, 0, 100, 100};

    applyFilter(def, ctx);

    CHECK(env.renderer.capturedColor.size() == 1);
    CHECK(env.renderer.capturedColor[0].value == Colors::Red);
}