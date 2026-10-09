#include "FilterRegistry.hpp"
#include "Layout.hpp"
#include "UIContext.hpp"
#include "StyleAttr.hpp"

namespace ZenitUI
{

    // =========================================================================
    //  Registry
    // =========================================================================

    FilterRegistry &FilterRegistry::get()
    {
        static FilterRegistry r;
        return r;
    }

    void FilterRegistry::add(FilterDef def)
    {
        defs_[def.name] = std::move(def);
    }

    void FilterRegistry::addCustom(std::string name, FilterCustomFn fn)
    {
        FilterDef def;
        def.name = std::move(name);
        def.pattern = FilterPattern::Custom;
        def.custom = std::move(fn);
        defs_[def.name] = std::move(def);
    }

    const FilterDef *FilterRegistry::find(const std::string &name) const
    {
        auto it = defs_.find(name);
        return it == defs_.end() ? nullptr : &it->second;
    }

    // =========================================================================
    //  Parametri: applica gli uniform al shader attivo
    // =========================================================================

    static float parseLengthArg(const std::string &s)
    {
        std::string t = s;
        if (t.size() > 2 && t.substr(t.size() - 2) == "px")
            t = t.substr(0, t.size() - 2);
        try
        {
            return std::stof(t);
        }
        catch (...)
        {
            return 0.0f;
        }
    }

    static void setParams(IRenderer *r, EffectHandle fx,
                          const FilterDef &def, const FilterRef &ref)
    {
        for (const auto &p : def.params)
        {
            const int n = (int)ref.args.size();

            switch (p.type)
            {
            case FilterParamType::Float:
            {
                float v = p.fdef;
                if (p.argIndex < n)
                    v = parseLengthArg(ref.args[p.argIndex]);
                r->setEffectFloat(fx, p.uniform.c_str(), v);
                break;
            }
            case FilterParamType::Vec2:
            {
                float x = p.vdef.x, y = p.vdef.y;
                if (p.argIndex + 1 < n)
                {
                    x = parseLengthArg(ref.args[p.argIndex]);
                    y = parseLengthArg(ref.args[p.argIndex + 1]);
                }
                r->setEffectVec2(fx, p.uniform.c_str(), {x, y});
                break;
            }
            case FilterParamType::Vec3:
            {
                float a = 0, b = 0, c = 0;
                if (p.argIndex + 2 < n)
                {
                    a = parseLengthArg(ref.args[p.argIndex]);
                    b = parseLengthArg(ref.args[p.argIndex + 1]);
                    c = parseLengthArg(ref.args[p.argIndex + 2]);
                }
                r->setEffectVec3(fx, p.uniform.c_str(), {a, b, c});
                break;
            }
            case FilterParamType::Vec4:
            {
                float a = 0, b = 0, c = 0, d = 0;
                if (p.argIndex + 3 < n)
                {
                    a = parseLengthArg(ref.args[p.argIndex]);
                    b = parseLengthArg(ref.args[p.argIndex + 1]);
                    c = parseLengthArg(ref.args[p.argIndex + 2]);
                    d = parseLengthArg(ref.args[p.argIndex + 3]);
                }
                r->setEffectVec4f(fx, p.uniform.c_str(), {a, b, c, d});
                break;
            }
            case FilterParamType::Color:
            {
                Color c = p.cdef;
                if (p.argIndex < n)
                    if (auto col = parseColorToken(ref.args[p.argIndex]))
                        c = *col;
                r->setEffectVec4(fx, p.uniform.c_str(), c);
                break;
            }
            }
        }
    }

    // =========================================================================
    //  Applicazione per pattern
    // =========================================================================

    static void applySinglePass(const FilterContext &c, EffectHandle fx)
    {
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
        c.renderer->drawTarget(c.src, c.region, Colors::White.withAlpha(c.opacity));
        c.renderer->popEffect();
    }

    static void applySeparable(const FilterContext &c, EffectHandle fx,
                               const FilterDef & /*def*/)
    {
        // Pass H: src → scratch
        c.renderer->pushTarget(c.scratch);
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
        c.renderer->setEffectVec2(fx, "direction", {1.0f, 0.0f});
        c.renderer->drawTarget(c.src, c.dst, Colors::White);
        c.renderer->popEffect();
        c.renderer->popTarget();

        // Pass V: scratch → framebuffer
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
        c.renderer->setEffectVec2(fx, "direction", {0.0f, 1.0f});
        c.renderer->drawTarget(c.scratch, c.region, Colors::White.withAlpha(c.opacity));
        c.renderer->popEffect();
    }

    static void applySilhouette(const FilterContext &c, EffectHandle fx,
                                const FilterDef &def)
    {
        // Gli argomenti dx/dy e shadowColor vengono settati da setParams.
        // Cerchiamo dx/dy per spostare il draw.
        float dx = 2.0f, dy = 2.0f;
        for (const auto &p : def.params)
        {
            if (p.uniform == "dx" && p.argIndex < (int)c.ref->args.size())
                dx = parseLengthArg(c.ref->args[p.argIndex]);
            if (p.uniform == "dy" && p.argIndex < (int)c.ref->args.size())
                dy = parseLengthArg(c.ref->args[p.argIndex]);
        }
        Rect shadowDst = {c.region.x + dx, c.region.y + dy,
                          c.region.width, c.region.height};

        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", {c.dst.width, c.dst.height});
        c.renderer->drawTarget(c.src, shadowDst, Colors::White.withAlpha(c.opacity));
        c.renderer->popEffect();

        c.renderer->drawTarget(c.src, c.region, Colors::White.withAlpha(c.opacity));
    }

    void applyFilter(const FilterDef &def, const FilterContext &ctx)
    {
        if (def.pattern == FilterPattern::Custom)
        {
            if (def.custom)
                def.custom(ctx);
            return;
        }

        auto *assets = UIContext::get().assets;
        if (!assets)
            return;
        EffectHandle fx = assets->getEffect(def.shader);
        if (!fx.valid())
        {
            // Shader mancante: fallback (disegna senza filtro).
            ctx.renderer->drawTarget(ctx.src, ctx.region,
                                     Colors::White.withAlpha(ctx.opacity));
            return;
        }

        setParams(ctx.renderer, fx, def, *ctx.ref);

        switch (def.pattern)
        {
        case FilterPattern::SinglePass:
            applySinglePass(ctx, fx);
            break;
        case FilterPattern::Separable:
            applySeparable(ctx, fx, def);
            break;
        case FilterPattern::Silhouette:
            applySilhouette(ctx, fx, def);
            break;
        case FilterPattern::Custom:
            break;
        }
    }

    // =========================================================================
    //  Built-in
    // =========================================================================

    void registerBuiltinFilters()
    {
        auto &reg = FilterRegistry::get();
        if (reg.find("blur"))
            return; // già registrati

        reg.add({.name = "blur",
                 .shader = "blur",
                 .pattern = FilterPattern::Separable,
                 .params = {
                     FilterParam{"radius", FilterParamType::Float, 0, 4.0f}}});

        reg.add({.name = "drop-shadow",
                 .shader = "dropShadow",
                 .pattern = FilterPattern::Silhouette,
                 .params = {
                     FilterParam{"dx", FilterParamType::Float, 0, 2.0f},
                     FilterParam{"dy", FilterParamType::Float, 1, 2.0f},
                     FilterParam{"shadowColor", FilterParamType::Color, 2, Color{0, 0, 0, 180}}}});
    }

} // namespace ZenitUI