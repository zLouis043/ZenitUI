#pragma once

/**
 * \file FilterRegistry.hpp
 * \brief Post-processing filter registry and the filter execution context.
 *
 * A "filter" is a named post-processing effect applied to a node's rendered
 * layer (the off-screen target produced by `Layout::drawLayer`). Filters are
 * declared in CSS via `filter: blur(4px) drop-shadow(2, 2, 4, black)` and
 * resolved at draw time through this registry.
 *
 * Four execution patterns are supported:
 *
 *  - **SinglePass**: one shader, one draw. Used for effects that need only
 *    the source target (`drop-shadow` with a single offset, custom blurs, …).
 *  - **Separable**: two passes (horizontal then vertical). Efficient for
 *    Gaussian-style blurs. Uses `scratch` as the intermediate target.
 *  - **Silhouette**: draw the source offset by `dx/dy` through a colour
 *    shader, then the source on top. Used by `drop-shadow`.
 *  - **Custom**: a `std::function` invoked directly.
 */

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"

#include <functional>

namespace ZenitUI {

class Layout;
class IRenderer;

/** \brief Execution pattern for a filter definition. */
enum class FilterPattern {
    SinglePass,  ///< One shader, one draw.
    Separable,   ///< Two shader passes (H then V), using the scratch target.
    Silhouette,  ///< Offset silhouette draw + original draw, for drop-shadows.
    Custom       ///< Invoke the `custom` callback directly.
};

/** \brief Type of a filter parameter. */
enum class FilterParamType {
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
struct FilterParam {
    std::string     uniform;
    FilterParamType type{FilterParamType::Float};
    int             argIndex{0};
    float           fdef{0.0f};
    Vec2            vdef{};
    Color           cdef{Colors::White};

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
struct FilterContext {
    IRenderer*       renderer{nullptr};
    Layout*          node{nullptr};
    TargetHandle     src{};
    TargetHandle     scratch{};
    TargetHandle     write_target{};
    Rect             region{};
    Rect             dst{};
    float            opacity{1.0f};
    const FilterRef* ref{nullptr};
};

/** \brief Callback signature for `FilterPattern::Custom` filters. */
using FilterCustomFn = std::function<void(const FilterContext&)>;

/**
 * \struct FilterDef
 * \brief Complete description of one filter.
 */
struct FilterDef {
    std::string                name;
    std::string                shader;
    FilterPattern              pattern{FilterPattern::SinglePass};
    std::vector<FilterParam>   params;
    FilterCustomFn             custom;
};

/**
 * \class FilterRegistry
 * \brief Global name → `FilterDef` map.
 */
class FilterRegistry {
public:
    /** \brief Return the global singleton registry. */
    static FilterRegistry& get();

    /** \brief Register or replace a filter definition. */
    void add(FilterDef def);

    /** \brief Register a custom (shader-less) filter. */
    void addCustom(std::string name, FilterCustomFn fn);

    /** \brief Look up a filter by name. */
    const FilterDef* find(const std::string& name) const;

private:
    std::unordered_map<std::string, FilterDef> defs_;
};

/**
 * \brief Ensure the builtin filters (`blur`, `drop-shadow`) are registered.
 */
void registerBuiltinFilters();

/**
 * \brief Execute one filter against the given context.
 */
void applyFilter(const FilterDef& def, const FilterContext& ctx);

} // namespace ZenitUI