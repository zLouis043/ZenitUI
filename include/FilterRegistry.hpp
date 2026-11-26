#pragma once

/**
 * \file FilterRegistry.hpp
 * \brief Post-processing filter registry and the filter execution context.
 *
 * A "filter" is a named post-processing effect applied to a node's rendered
 * layer (the off-screen target produced by `Layout::drawLayer`). Filters are
 * declared in CSS via `filter: blur(4px), drop-shadow(2px, 2px, #000)` and
 * resolved at draw time through the active `UIContext::filters` registry.
 *
 * Four execution patterns are supported:
 *
 *  - **SinglePass**: one shader, one draw. Used for effects that need only
 *    the source target (color matrices, vignettes, custom shaders, …).
 *  - **Separable**: two passes (horizontal then vertical). Efficient for
 *    Gaussian-style blurs. Uses `scratch` (or a downscaled pooled target) as
 *    the intermediate target.
 *  - **Silhouette**: draw the source offset by `dx/dy` through a colour
 *    shader, then the source on top. Used by `drop-shadow`.
 *  - **Custom**: a `std::function` invoked directly.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"

#include <functional>

namespace ZenitUI
{

    /** \addtogroup grp_anim
     *  @{
     */

    class Layout;
    class IRenderer;
    struct UIContext;

    /** \brief Execution pattern for a filter definition. */
    enum class FilterPattern
    {
        SinglePass, ///< One shader, one draw.
        Separable,  ///< Two shader passes (H then V), using the scratch target.
        Silhouette, ///< Offset silhouette draw + original draw, for drop-shadows.
        Custom      ///< Invoke the `custom` callback directly.
    };

    /** \brief Type of a filter parameter. */
    enum class FilterParamType
    {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Color
    };

    /**
     * \struct FilterParam
     * \brief One uniform exposed by a filter shader.
     *
     * `argIndex` is the index into `FilterRef::args` that supplies this
     * parameter's value; if the call site did not provide enough arguments, the
     * default (`fdef`, `vdef`, or `cdef`) is used instead.
     */
    struct FilterParam
    {
        std::string uniform;
        FilterParamType type{FilterParamType::Float};
        int argIndex{0};
        float fdef{0.0f};
        Vec2 vdef{};
        Color cdef{Colors::White};

        FilterParam() = default;

        FilterParam(std::string u, FilterParamType t, int ai, float fd)
            : uniform(std::move(u)), type(t), argIndex(ai), fdef(fd) {}

        FilterParam(std::string u, FilterParamType t, int ai, Color cd)
            : uniform(std::move(u)), type(t), argIndex(ai), cdef(cd) {}
    };

    /**
     * \struct FilterContext
     * \brief Per-invocation state passed to `applyFilter` and to custom filters.
     */
    struct FilterContext
    {
        UIContext *ui{nullptr};       ///< Owning UI context (assets, target pool, render stats).
        IRenderer *renderer{nullptr}; ///< Active renderer backend.
        Layout *node{nullptr};        ///< Filtered node currently being rendered.
        TargetHandle src{};           ///< Source off-screen target for this pass.
        TargetHandle scratch{};       ///< Intermediate target for full-res separable/custom passes.
        TargetHandle write_target{};  ///< Destination off-screen target, or invalid when drawing to screen.
        Rect region{};                ///< Screen-space destination rectangle.
        Rect dst{};                   ///< Logical content sub-rectangle inside off-screen targets.
        Vec2 texSize{};               ///< Backing texture dimensions of `src` in logical pixels.
        float opacity{1.0f};          ///< Final compositing opacity (applied on the screen pass).
        const FilterRef *ref{nullptr};///< Parsed CSS filter reference supplying parameter arguments.
    };

    /** \brief Callback signature for `FilterPattern::Custom` filters. */
    using FilterCustomFn = std::function<void(const FilterContext &)>;

    /**
     * \struct FilterDef
     * \brief Complete description of one filter.
     */
    struct FilterDef
    {
        std::string name;
        std::string shader;
        FilterPattern pattern{FilterPattern::SinglePass};
        std::vector<FilterParam> params;
        FilterCustomFn custom;
        int downscale{1}; ///< 1, 2, or 4 (Separable filters only).
    };

    /**
     * \class FilterRegistry
     * \brief Per-context map from CSS filter name to `FilterDef`.
     *
     * Owned by `UIContext::filters`. Default construction automatically
     * registers the builtin filters (`blur`, `drop-shadow`).
     */
    class FilterRegistry
    {
    public:
        /** \brief Construct the registry and populate builtin filters. */
        FilterRegistry();

        /** \brief Register or replace a filter definition. */
        void add(FilterDef def);

        /** \brief Register a custom (shader-less) filter. */
        void addCustom(std::string name, FilterCustomFn fn);

        /** \brief Look up a filter by name; returns `nullptr` if not found. */
        const FilterDef *find(const std::string &name) const;

        /** \brief Register the builtin filters (`blur`, `drop-shadow`) if absent. */
        void registerBuiltins();

        /** \brief Remove all registered filters, including builtins. */
        void clear();

        /** \brief Clear all filters and re-register only the builtins. */
        void reset();

    private:
        std::unordered_map<std::string, FilterDef> defs_;
    };

    /**
     * \brief Execute one filter against the given context.
     * \param def Filter definition to execute.
     * \param ctx Invocation context providing renderer, targets, and `UIContext`.
     */
    void applyFilter(const FilterDef &def, const FilterContext &ctx);

    /** @} */
} // namespace ZenitUI