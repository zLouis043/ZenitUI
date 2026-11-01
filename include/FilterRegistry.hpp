#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"

#include <functional>

namespace ZenitUI {

class Layout;
class IRenderer;

enum class FilterPattern {
    SinglePass,
    Separable,
    Silhouette,
    Custom
};

enum class FilterParamType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Color
};

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

using FilterCustomFn = std::function<void(const FilterContext&)>;

struct FilterDef {
    std::string                name;
    std::string                shader;
    FilterPattern              pattern{FilterPattern::SinglePass};
    std::vector<FilterParam>   params;
    FilterCustomFn             custom;
};

class FilterRegistry {
public:
    static FilterRegistry& get();

    void add(FilterDef def);

    void addCustom(std::string name, FilterCustomFn fn);

    const FilterDef* find(const std::string& name) const;

private:
    std::unordered_map<std::string, FilterDef> defs_;
};

void registerBuiltinFilters();

void applyFilter(const FilterDef& def, const FilterContext& ctx);

} // namespace ZenitUI