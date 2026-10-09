#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "Style.hpp"

#include <functional>

namespace ZenitUI {

class Layout;
class IRenderer;

enum class FilterPattern {
    SinglePass,   // push shader, draw src → region
    Separable,    // 2 pass (H, V) con uniform "direction"
    Silhouette,   // shadow color, due draw (shadow poi src)
    Custom        // lambda arbitraria
};

enum class FilterParamType {
    Float,
    Vec2,       // consuma 2 argomenti CSS consecutivi
    Vec3,       // 3 argomenti
    Vec4,       // 4 argomenti
    Color       // 1 argomento "#RRGGBB[AA]"
};

struct FilterParam {
    std::string     uniform;
    FilterParamType type{FilterParamType::Float};
    int             argIndex{0};
    float           fdef{0.0f};
    Vec2            vdef{};
    Color           cdef{Colors::White};

    FilterParam() = default;

    // Float
    FilterParam(std::string u, FilterParamType t, int ai, float fd)
        : uniform(std::move(u)), type(t), argIndex(ai), fdef(fd) {}

    // Color
    FilterParam(std::string u, FilterParamType t, int ai, Color cd)
        : uniform(std::move(u)), type(t), argIndex(ai), cdef(cd) {}
};

struct FilterContext {
    IRenderer*       renderer{nullptr};
    Layout*          node{nullptr};
    TargetHandle     src{};
    TargetHandle     scratch{};
    Rect             region{};    // dove disegnare sul framebuffer
    Rect             dst{};       // {0, 0, tw, th}: coordinate del target
    float            opacity{1.0f};
    const FilterRef* ref{nullptr};
};

using FilterCustomFn = std::function<void(const FilterContext&)>;

struct FilterDef {
    std::string                name;
    std::string                shader;
    FilterPattern              pattern{FilterPattern::SinglePass};
    std::vector<FilterParam>   params;
    FilterCustomFn             custom;   // usato se pattern == Custom
};

class FilterRegistry {
public:
    static FilterRegistry& get();

    // Registra un filtro. Sovrascrive silenziosamente se il nome esiste già.
    void add(FilterDef def);

    // Escape hatch: pipeline arbitrarie.
    void addCustom(std::string name, FilterCustomFn fn);

    // Trova il descriptor per nome. nullptr se assente.
    const FilterDef* find(const std::string& name) const;

private:
    std::unordered_map<std::string, FilterDef> defs_;
};

// Registra i built-in (blur, drop-shadow). Idempotente.
void registerBuiltinFilters();

// Applica un filtro al layer già renderizzato su `ctx.src`.
// Usato da Layout::drawLayer. Non chiamare direttamente se non sai cosa fai.
void applyFilter(const FilterDef& def, const FilterContext& ctx);

} // namespace ZenitUI