#include "FilterRegistry.hpp"
#include "Layout.hpp"
#include "UIContext.hpp"
#include "StyleAttr.hpp"

#include <cstdlib>

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
            if (c.ui)
                c.ui->renderStats.pushTargetCalls++;
            // Copy raw RGBA into the freshly cleared off-screen target without re-blending.
            c.renderer->setBlendMode(BlendMode::Replace);
            c.renderer->drawTarget(c.src, c.dst, c.dst, Colors::White);
            c.renderer->setBlendMode(BlendMode::Alpha);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(c.src, c.dst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }
    }

    FilterRegistry::FilterRegistry()
    {
        registerBuiltins();
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

    void FilterRegistry::clear()
    {
        defs_.clear();
    }

    void FilterRegistry::reset()
    {
        defs_.clear();
        registerBuiltins();
    }

    void FilterRegistry::registerBuiltins()
    {
        if (!find("blur"))
        {
            add({.name = "blur",
                 .shader = "blur",
                 .pattern = FilterPattern::Separable,
                 .params = {
                     FilterParam{"radius", FilterParamType::Float, 0, 4.0f}}});
        }

        if (!find("drop-shadow"))
        {
            add({.name = "drop-shadow",
                 .shader = "dropShadow",
                 .pattern = FilterPattern::Silhouette,
                 .params = {
                     FilterParam{"dx", FilterParamType::Float, 0, 2.0f},
                     FilterParam{"dy", FilterParamType::Float, 1, 2.0f},
                     FilterParam{"shadowColor", FilterParamType::Color, 2, Color{0, 0, 0, 180}}}});
        }
    }

    static float parseLengthArg(const std::string &s) noexcept
    {
        const char *p = s.data();
        const char *end = p + s.size();
        while (p < end && (*p == ' ' || *p == '\t'))
            ++p;
        if (p == end)
            return 0.0f;

        float sign = 1.0f;
        if (*p == '-')
        {
            sign = -1.0f;
            ++p;
        }
        else if (*p == '+')
        {
            ++p;
        }

        float val = 0.0f;
        bool hasDigits = false;
        while (p < end && *p >= '0' && *p <= '9')
        {
            val = val * 10.0f + static_cast<float>(*p - '0');
            hasDigits = true;
            ++p;
        }
        if (p < end && *p == '.')
        {
            ++p;
            float scale = 0.1f;
            while (p < end && *p >= '0' && *p <= '9')
            {
                val += static_cast<float>(*p - '0') * scale;
                scale *= 0.1f;
                hasDigits = true;
                ++p;
            }
        }
        if (!hasDigits)
            return 0.0f;
        if (p < end && (*p == 'e' || *p == 'E'))
            return std::strtof(s.c_str(), nullptr);
        return sign * val;
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
        RenderStats *stats = c.ui ? &c.ui->renderStats : nullptr;
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, c.src));

        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            if (stats)
                stats->pushTargetCalls++;
            // Preserve exact shader RGBA in off-screen target to prevent double blending on blit.
            c.renderer->setBlendMode(BlendMode::Replace);
            c.renderer->drawTarget(c.src, c.dst, c.dst, Colors::White);
            c.renderer->setBlendMode(BlendMode::Alpha);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(c.src, c.dst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }

        if (stats)
            stats->filterPasses++;
        c.renderer->popEffect();
    }

    static void applySeparable(const FilterContext &c, EffectHandle fx,
                               const FilterDef &def)
    {
        RenderStats *stats = c.ui ? &c.ui->renderStats : nullptr;
        const int ds = (def.downscale >= 4) ? 4 : (def.downscale >= 2 ? 2 : 1);

        TargetHandle midTarget = c.scratch;
        Rect midDst = c.dst;
        bool acquiredMid = false;

        if (ds > 1)
        {
            if (!c.ui)
                return;
            const int dw = std::max(1, (int)std::ceil(c.dst.width / (float)ds));
            const int dh = std::max(1, (int)std::ceil(c.dst.height / (float)ds));
            midTarget = c.ui->targetPool.acquire(*c.renderer, dw, dh, stats);
            if (!midTarget.valid())
                return;
            midDst = {0.0f, 0.0f, (float)dw, (float)dh};
            acquiredMid = true;
        }

        // Horizontal pass: read c.src (full-res) -> write midTarget (full or downscaled).
        c.renderer->pushTarget(midTarget);
        if (stats)
            stats->pushTargetCalls++;
        c.renderer->setBlendMode(BlendMode::Replace);
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, c.src));
        c.renderer->setEffectVec2(fx, "direction", {1.0f, 0.0f});
        c.renderer->drawTarget(c.src, c.dst, midDst, Colors::White);
        c.renderer->popEffect();
        c.renderer->setBlendMode(BlendMode::Alpha);
        c.renderer->popTarget();
        if (stats)
            stats->filterPasses++;

        // Vertical pass: read midTarget -> write write_target or screen (bilinear upscale).
        c.renderer->pushEffect(fx);
        c.renderer->setEffectVec2(fx, "texSize", resolveTexSize(c, midTarget));
        c.renderer->setEffectVec2(fx, "direction", {0.0f, 1.0f});

        if (c.write_target.valid())
        {
            c.renderer->pushTarget(c.write_target);
            if (stats)
                stats->pushTargetCalls++;
            c.renderer->setBlendMode(BlendMode::Replace);
            c.renderer->drawTarget(midTarget, midDst, c.dst, Colors::White);
            c.renderer->setBlendMode(BlendMode::Alpha);
            c.renderer->popTarget();
        }
        else
        {
            c.renderer->drawTarget(midTarget, midDst, c.region,
                                   Colors::White.withAlpha(c.opacity));
        }

        if (stats)
            stats->filterPasses++;
        c.renderer->popEffect();

        if (acquiredMid)
            c.ui->targetPool.release(midTarget);
    }

    static void applySilhouette(const FilterContext &c, EffectHandle fx,
                                const FilterDef &def)
    {
        RenderStats *stats = c.ui ? &c.ui->renderStats : nullptr;
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
            if (stats)
                stats->pushTargetCalls++;

            // Composite foreground over shadow with separate alpha; output is premultiplied.
            c.renderer->setBlendMode(BlendMode::Alpha);

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
        if (stats)
            stats->filterPasses++;
    }

    void applyFilter(const FilterDef &def, const FilterContext &ctx)
    {
        if (def.pattern == FilterPattern::Custom)
        {
            if (def.custom)
            {
                def.custom(ctx);
                if (ctx.ui)
                    ctx.ui->renderStats.filterPasses++;
            }
            return;
        }

        IAssetProvider *assets = ctx.ui ? ctx.ui->assets : nullptr;
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

} // namespace ZenitUI