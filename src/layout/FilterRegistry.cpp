#include "FilterRegistry.hpp"
#include "Layout.hpp"
#include "UIContext.hpp"
#include "StyleAttr.hpp"

namespace ZenitUI
{

    static Vec2 resolveTexSize(const FilterContext &c, TargetHandle t)
    {
        if (t.width > 0 && t.height > 0)
            return {(float)t.width, (float)t.height};
        if (c.texSize.x > 0.0f && c.texSize.y > 0.0f)
            return c.texSize;
        return {c.dst.width, c.dst.height};
    }

    static void drawUnmodified(const FilterContext &c)
    {
        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            UIContext::get().renderStats.pushTargetCalls++;
            c.renderer->drawTarget(c.src, c.dst, c.dst, Colors::White);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(c.src, c.dst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }
    }

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


    static void applySinglePass(const FilterContext &c, EffectHandle fx)
    {
        auto &stats = UIContext::get().renderStats;
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, c.src));

        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            stats.pushTargetCalls++;
            c.renderer->drawTarget(c.src, c.dst, c.dst, Colors::White);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(c.src, c.dst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }

        stats.filterPasses++;
        c.renderer->popEffect();
    }

    static void applySeparable(const FilterContext &c, EffectHandle fx,
                               const FilterDef &def)
    {
        auto &ctxUI = UIContext::get();
        auto &stats = ctxUI.renderStats;

        const int ds = (def.downscale >= 4) ? 4 : (def.downscale >= 2 ? 2 : 1);

        TargetHandle midTarget = c.scratch;
        Rect midDst = c.dst;
        bool acquiredMid = false;

        if (ds > 1)
        {
            const int dw = std::max(1, (int)std::ceil(c.dst.width / (float)ds));
            const int dh = std::max(1, (int)std::ceil(c.dst.height / (float)ds));
            midTarget = ctxUI.targetPool.acquire(*c.renderer, dw, dh, &stats);
            if (!midTarget.valid())
                return;
            midDst = {0.0f, 0.0f, (float)dw, (float)dh};
            acquiredMid = true;
        }

        // Passata H: legge c.src (full-res) -> scrive midTarget (full o downscaled)
        c.renderer->pushTarget(midTarget);
        stats.pushTargetCalls++;
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, c.src));
        c.renderer->setEffectVec2(fx, "direction", {1.0f, 0.0f});
        c.renderer->drawTarget(c.src, c.dst, midDst, Colors::White);
        c.renderer->popEffect();
        c.renderer->popTarget();
        stats.filterPasses++;

        // Passata V: legge midTarget -> scrive write_target o schermo (upscale bilineare)
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, midTarget));
        c.renderer->setEffectVec2(fx, "direction", {0.0f, 1.0f});

        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            stats.pushTargetCalls++;
            c.renderer->drawTarget(midTarget, midDst, c.dst, Colors::White);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(midTarget, midDst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }

        stats.filterPasses++;
        c.renderer->popEffect();

        if (acquiredMid)
            ctxUI.targetPool.release(midTarget);
    }

    static void applySilhouette(const FilterContext &c, EffectHandle fx,
                                const FilterDef &def)
    {
        auto &stats = UIContext::get().renderStats;
        float dx = 2.0f, dy = 2.0f;
        for (const auto &p : def.params)
        {
            if (p.uniform == "dx" && p.argIndex < (int)c.ref->args.size())
                dx = parseLengthArg(c.ref->args[p.argIndex]);
            if (p.uniform == "dy" && p.argIndex < (int)c.ref->args.size())
                dy = parseLengthArg(c.ref->args[p.argIndex]);
        }

        const Vec2 ts = resolveTexSize(c, c.src);

        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            stats.pushTargetCalls++;

            Rect shadowDst = {c.dst.x + dx, c.dst.y + dy,
                              c.dst.width, c.dst.height};
            c.renderer->pushEffect(fx);
            c.renderer->setEffectVec2(fx, "texSize", ts);
            c.renderer->drawTarget(c.src, c.dst, shadowDst, Colors::White);
            c.renderer->popEffect();

            c.renderer->drawTarget(c.src, c.dst, c.dst, Colors::White);

            c.renderer->popTarget();
        }
        else
        {
            Rect shadowRegion = {c.region.x + dx, c.region.y + dy,
                                 c.region.width, c.region.height};
            c.renderer->pushEffect(fx);
            c.renderer->setEffectVec2(fx, "texSize", ts);
            c.renderer->drawTarget(c.src, c.dst, shadowRegion,
                                   Colors::White.withAlpha(c.opacity));
            c.renderer->popEffect();

            c.renderer->drawTarget(c.src, c.dst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }
        stats.filterPasses++;
    }

    void applyFilter(const FilterDef &def, const FilterContext &ctx)
    {
        if (def.pattern == FilterPattern::Custom)
        {
            if (def.custom)
            {
                def.custom(ctx);
                UIContext::get().renderStats.filterPasses++;
            }
            return;
        }

        auto *assets = UIContext::get().assets;
        if (!assets)
        {
            drawUnmodified(ctx);
            return;
        }

        EffectHandle fx = assets->getEffect(def.shader);
        if (!fx.valid())
        {
            drawUnmodified(ctx);
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


    void registerBuiltinFilters()
    {
        auto &reg = FilterRegistry::get();
        if (reg.find("blur"))
            return;

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