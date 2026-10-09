#pragma once

#include "Layout.hpp"
#include "Theme.hpp"
#include "Unit.hpp"
#include "CoreTypes.hpp"

#ifdef ZENITUI_DEBUG

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

namespace ZenitUI::Debug
{

    // ============================================================
    //  toString per i tipi che appaiono nelle style props
    // ============================================================
    inline const char *unitName(Unit u)
    {
        switch (u)
        {
        case Unit::Auto:
            return "auto";
        case Unit::Number:
            return "";
        case Unit::Pixel:
            return "px";
        case Unit::Percent:
            return "%";
        case Unit::VW:
            return "vw";
        case Unit::VH:
            return "vh";
        case Unit::PW:
            return "pw";
        case Unit::PH:
            return "ph";
        }
        return "?";
    }

    inline std::string valueToString(const Value &v)
    {
        if (v.isAuto())
            return "auto";
        std::ostringstream os;
        for (size_t i = 0; i < v.terms.size(); ++i)
        {
            const auto &t = v.terms[i];
            if (i == 0)
            {
                if (t.coeff < 0.0f)
                    os << "-";
                os << std::abs(t.coeff) << unitName(t.unit);
            }
            else
            {
                os << (t.coeff < 0.0f ? " - " : " + ");
                os << std::abs(t.coeff) << unitName(t.unit);
            }
        }
        return os.str();
    }

    inline std::string toString(float v)
    {
        std::ostringstream os;
        os << v;
        return os.str();
    }
    inline std::string toString(const std::string &s) { return s.empty() ? "\"\"" : s; }
    inline std::string toString(const Value &v) { return valueToString(v); }

    inline std::string toString(const Color &c)
    {
        std::ostringstream os;
        os << "#" << std::hex << std::setfill('0')
           << std::setw(2) << (int)c.r
           << std::setw(2) << (int)c.g
           << std::setw(2) << (int)c.b
           << std::setw(2) << (int)c.a
           << std::dec;
        return os.str();
    }

    inline std::string toString(const Spacing &s)
    {
        return "[" + valueToString(s.top) + " " + valueToString(s.right) + " " + valueToString(s.bottom) + " " + valueToString(s.left) + "]";
    }

    inline std::string toString(Align a)
    {
        switch (a)
        {
        case Align::Auto:
            return "auto";
        case Align::Start:
            return "start";
        case Align::Center:
            return "center";
        case Align::End:
            return "end";
        case Align::Stretch:
            return "stretch";
        }
        return "?";
    }

    inline std::string toString(Justify j)
    {
        switch (j)
        {
        case Justify::Start:
            return "start";
        case Justify::Center:
            return "center";
        case Justify::End:
            return "end";
        case Justify::SpaceBetween:
            return "space-between";
        }
        return "?";
    }

    inline std::string toString(Position p)
    {
        switch (p)
        {
        case Position::Static:
            return "static";
        case Position::Relative:
            return "relative";
        case Position::Absolute:
            return "absolute";
        }
        return "?";
    }

    inline std::string toString(Overflow o)
    {
        switch (o)
        {
        case Overflow::Visible:
            return "visible";
        case Overflow::Hidden:
            return "hidden";
        case Overflow::Scroll:
            return "scroll";
        case Overflow::Auto:
            return "auto";
        }
        return "?";
    }

    inline std::string toString(TransitionFunction f)
    {
        switch (f)
        {
        case TransitionFunction::Linear:
            return "linear";
        case TransitionFunction::EaseInQuad:
            return "ease-in-quad";
        case TransitionFunction::EaseOutQuad:
            return "ease-out-quad";
        case TransitionFunction::EaseInOutQuad:
            return "ease-in-out-quad";
        case TransitionFunction::EaseInCubic:
            return "ease-in-cubic";
        case TransitionFunction::EaseOutCubic:
            return "ease-out-cubic";
        case TransitionFunction::EaseInOutCubic:
            return "ease-in-out-cubic";
        case TransitionFunction::EaseInBack:
            return "ease-in-back";
        case TransitionFunction::EaseOutBack:
            return "ease-out-back";
        case TransitionFunction::EaseOutElastic:
            return "ease-out-elastic";
        case TransitionFunction::EaseOutBounce:
            return "ease-out-bounce";
        }
        return "?";
    }

    inline std::string toString(const ZIndex &z)
    {
        return z.isAuto ? "auto" : std::to_string(z.value);
    }

    inline std::string toString(const TextureRef &t)
    {
        std::ostringstream os;
        os << t.name;
        if (t.isNineSlice())
            os << " " << t.left << " " << t.top << " " << t.right << " " << t.bottom;
        return os.str();
    }

    inline std::string toString(const FilterRef &f)
    {
        if (f.args.empty())
            return f.name;
        std::string s = f.name + "(";
        for (size_t i = 0; i < f.args.size(); ++i)
        {
            if (i > 0)
                s += ", ";
            s += f.args[i];
        }
        s += ")";
        return s;
    }

    inline std::string toString(const std::vector<FilterRef> &fs)
    {
        if (fs.empty())
            return "[]";
        std::string s;
        for (size_t i = 0; i < fs.size(); ++i)
        {
            if (i > 0)
                s += " ";
            s += toString(fs[i]);
        }
        return s;
    }

    inline std::string toString(const BoxShadow &b)
    {
        if (!b.enabled)
            return "none";
        std::ostringstream os;
        os << valueToString(b.x) << " " << valueToString(b.y)
           << " " << valueToString(b.blur) << " " << toString(b.color);
        return os.str();
    }

    // ============================================================
    //  Style / ComputedStyle
    // ============================================================
    inline std::string styleToString(const Style &s)
    {
        std::ostringstream os;
        bool first = true;
        auto sep = [&]()
        { if (!first) os << ", "; first = false; };

#define X(T, name, def)                            \
    if (s.name.is_set)                             \
    {                                              \
        sep();                                     \
        os << #name "=" << toString(s.name.value); \
    }
        BUBBLE_STYLE_PROPS(X)
#undef X

        if (s.transitions.is_set)
        {
            sep();
            os << "transitions(" << s.transitions.value.size() << ")";
        }
        if (s.animations.is_set)
        {
            sep();
            os << "animations(" << s.animations.value.size() << ")";
        }
        return os.str();
    }

    inline std::string computedStyleToString(const ComputedStyle &cs)
    {
        std::ostringstream os;
        os << "size=" << valueToString(cs.width) << "x" << valueToString(cs.height);
        os << " bg=" << toString(cs.background);
        os << " color=" << toString(cs.color);
        if (!cs.font.empty())
            os << " font=" << cs.font;
        os << " fs=" << valueToString(cs.fontSize);
        os << " padding=" << toString(cs.padding);
        os << " margin=" << toString(cs.margin);
        os << " radius=" << valueToString(cs.radius);
        if (!cs.borderWidth.isAuto())
        {
            os << " borderW=" << valueToString(cs.borderWidth);
            os << " borderC=" << toString(cs.borderColor);
        }
        if (cs.opacity != 1.0f)
            os << " opacity=" << cs.opacity;
        if (cs.position != Position::Static)
            os << " pos=" << toString(cs.position);
        if (!cs.zIndex.isAuto)
            os << " zIndex=" << cs.zIndex.value;
        if (cs.overflowX != Overflow::Visible)
            os << " overflow-x=" << toString(cs.overflowX);
        if (cs.overflowY != Overflow::Visible)
            os << " overflow-y=" << toString(cs.overflowY);
        if (!cs.left.isAuto())
            os << " left=" << valueToString(cs.left);
        if (!cs.top.isAuto())
            os << " top=" << valueToString(cs.top);
        if (!cs.right.isAuto())
            os << " right=" << valueToString(cs.right);
        if (!cs.bottom.isAuto())
            os << " bottom=" << valueToString(cs.bottom);
        if (!cs.translateX.isAuto() || !cs.translateY.isAuto())
            os << " translate=(" << valueToString(cs.translateX) << "," << valueToString(cs.translateY) << ")";
        if (cs.scale != 1.0f)
            os << " scale=" << cs.scale;
        if (cs.rotation != 0.0f)
            os << " rotation=" << cs.rotation;
        if (!cs.customProps.empty())
        {
            os << " vars{";
            bool first = true;
            for (const auto &[k, v] : cs.customProps)
            {
                if (!first)
                    os << ", ";
                os << k << "=" << v;
                first = false;
            }
            os << "}";
        }
        return os.str();
    }

    // ============================================================
    //  Utilità per selettori
    // ============================================================
    inline std::string selectorToString(const ThemeRule &r)
    {
        std::ostringstream os;
        for (size_t i = 0; i < r.chain.size(); ++i)
        {
            if (i > 0)
                os << " ";
            const auto &ss = r.chain[i];

            auto printComponent = [&](SimpleSelector::Kind kind,
                                      const std::string &name)
            {
                if (kind == SimpleSelector::Kind::Class)
                    os << ".";
                else if (kind == SimpleSelector::Kind::Id)
                    os << "#";
                os << name;
            };

            printComponent(ss.kind, ss.name);
            for (const auto &[kind, name] : ss.extras)
                printComponent(kind, name);

            if (ss.requireHover)
                os << ":hover";
            if (ss.requirePressed)
                os << ":pressed";
            if (ss.requireFocus)
                os << ":focus";
            if (ss.requireDisabled)
                os << ":disabled";
            if (ss.requireChecked)
                os << ":checked";
        }
        if (!r.part.empty())
            os << "::" << r.part;
        return os.str();
    }

    inline std::string stateFlagsToString(const Layout &node)
    {
        std::string s;
        if (node.isHoveredState())
            s += "H";
        if (node.isPressedState())
            s += "P";
        if (node.isFocusedState())
            s += "F";
        if (node.isCheckedState())
            s += "C";
        if (node.isDisabledState())
            s += "D";
        return s.empty() ? "-" : s;
    }

    // ============================================================
    //  Dump del singolo nodo
    // ============================================================
    inline void dumpStyle(const Layout &node, std::ostream &os = std::cerr)
    {
        os << "=== Node ===\n";
        os << "  tag      : " << node.getStyleTag() << "\n";
        if (!node.nodeId.empty())
            os << "  id       : #" << node.nodeId << "\n";
        if (!node.getStyleClasses().empty())
        {
            os << "  classes  :";
            for (const auto &c : node.getStyleClasses())
                os << " ." << c;
            os << "\n";
        }
        os << "  state    : " << stateFlagsToString(node) << "\n";
        const auto &r = node.getRect();
        os << "  rect     : (" << r.x << ", " << r.y << "  " << r.width << "x" << r.height << ")\n";
        os << "  style    : " << computedStyleToString(node.getStyle()) << "\n";

        auto matches = node.getMatchingRules();
        os << "  matches  : " << matches.size() << " rule(s)\n";
        for (const auto *rule : matches)
        {
            os << "spec=" << rule->specificity.ids << "."
               << rule->specificity.classes << "."
               << rule->specificity.tags << "] "
               << selectorToString(*rule) << "\n";
            os << "        -> { " << styleToString(rule->style) << " }\n";
        }
    }

    // ============================================================
    //  Dump dell'albero
    // ============================================================
    inline void dumpTree(const Layout &node, std::ostream &os = std::cerr,
                         int maxDepth = -1, int depth = 0)
    {
        std::string indent(depth * 2, ' ');
        os << indent << "- ";
        if (!node.nodeId.empty())
            os << "#" << node.nodeId << " ";
        os << node.getStyleTag();
        for (const auto &c : node.getStyleClasses())
            os << "." << c;

        const auto &r = node.getRect();
        os << "  [" << r.width << "x" << r.height << " @ " << r.x << "," << r.y << "]";

        std::string st = stateFlagsToString(node);
        if (st != "-")
            os << " {" << st << "}";
        os << "\n";

        if (maxDepth >= 0 && depth >= maxDepth)
            return;
        for (const auto &c : node.children)
            dumpTree(*c, os, maxDepth, depth + 1);
    }

    // ============================================================
    //  Dump del tema
    // ============================================================
    inline void dumpTheme(std::ostream &os = std::cerr)
    {
        auto &theme = Theme::get();

        os << "=== Theme ===\n";
        os << "Rules    : " << theme.rules.size() << "\n";
        os << "Keyframes: " << theme.keyframes.size() << "\n";

        if (!theme.keyframes.empty())
        {
            os << "\n--- Keyframes ---\n";
            for (const auto &[name, kf] : theme.keyframes)
            {
                os << "  " << name << " (" << kf.keyframes.size() << " frames)\n";
            }
        }

        os << "\n--- Rules ---\n";
        for (size_t i = 0; i < theme.rules.size(); ++i)
        {
            const auto &r = theme.rules[i];
            os << "  [" << std::setw(3) << i << "] spec=" << std::setw(6) << r.specificity.ids << "."
               << r.specificity.classes << "."
               << r.specificity.tags << "  "
               << selectorToString(r) << "\n";
            os << "         { " << styleToString(r.style) << " }\n";
        }
        os << "\n";
    }

    // ============================================================
    //  Dump delle part attive su un nodo
    // ============================================================
    inline void dumpParts(const Layout &node, std::ostream &os = std::cerr)
    {
        static const char *candidates[] = {
            "track", "fill", "knob", "box", "mark", "cursor"};
        os << "=== Parts of " << node.getStyleTag() << " ===\n";
        for (auto *name : candidates)
        {
            auto &theme = Theme::get();
            int matchCount = 0;
            for (const auto &r : theme.rules)
            {
                if (r.part != name)
                    continue;
                if (ruleMatches(r, &node))
                    matchCount++;
            }
            if (matchCount > 0)
            {
                os << "  ::" << name << "  (" << matchCount << " matching rule(s))\n";
            }
        }
    }

} // namespace ZenitUI::Debug

#else // !ZENITUI_DEBUG

#include <iostream>

// ============================================================
//  Stub: compilano ma non fanno nulla. Le firme devono restare
//  identiche ai call site.
// ============================================================
namespace ZenitUI::Debug
{

    inline void dumpStyle(const Layout &, std::ostream & = std::cerr) {}
    inline void dumpTree(const Layout &, std::ostream & = std::cerr, int = -1, int = 0) {}
    inline void dumpTheme(std::ostream & = std::cerr) {}
    inline void dumpParts(const Layout &, std::ostream & = std::cerr) {}

} // namespace ZenitUI::Debug

#endif // ZENITUI_DEBUG