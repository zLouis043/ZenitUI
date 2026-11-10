#pragma once

/**
 * \file Theme.hpp
 * \brief Global stylesheet storage and rule matching.
 *
 * A `Theme` holds:
 *  - the parsed rule list, indexed by tag / last symbol for fast lookup;
 *  - the `:root` style used as fallback for inherited properties;
 *  - the `@keyframes` registry, keyed by animation name;
 *  - a monotonically increasing `version` used to invalidate caches.
 *
 * `StyleParser` populates a `Theme` via `addRule` / `addKeyframes`. The
 * `StyleResolver` reads it to compute per-node styles. Multiple `Theme`
 * instances are technically supported, but the resolver and most components
 * reference `Theme::get()` (the global).
 */

#include "Common.hpp"
#include "Style.hpp"
#include "AnimPrimitives.hpp"
#include "Media.hpp"

namespace ZenitUI
{
    /**
     * \brief CSS-style specificity triple used to sort matching rules.
     */
    struct Specificity
    {
        int ids{0};
        int classes{0};
        int tags{0};

        bool operator<(const Specificity &o) const
        {
            if (ids != o.ids)
                return ids < o.ids;
            if (classes != o.classes)
                return classes < o.classes;
            return tags < o.tags;
        }

        bool operator==(const Specificity &o) const
        {
            return ids == o.ids && classes == o.classes && tags == o.tags;
        }
        bool operator!=(const Specificity &o) const
        {
            return !(*this == o);
        }
    };

    /** \brief Compute the specificity of a full selector chain. */
    inline Specificity computeSpecificity(const std::vector<SimpleSelector> &chain)
    {
        Specificity s;
        for (const auto &sel : chain)
        {
            if (sel.kind == SimpleSelector::Kind::Id)
                s.ids++;
            else if (sel.kind == SimpleSelector::Kind::Class)
                s.classes++;
            else
                s.tags++;

            for (const auto &[kind, name] : sel.extras)
            {
                (void)name;
                if (kind == SimpleSelector::Kind::Id)
                    s.ids++;
                else if (kind == SimpleSelector::Kind::Class)
                    s.classes++;
                else
                    s.tags++;
            }

            if (sel.requireHover || sel.requirePressed || sel.requireFocus ||
                sel.requireDisabled || sel.requireChecked)
                s.classes++;
        }
        return s;
    }

    /**
     * \struct ThemeRule
     * \brief A single selector-plus-declarations entry in the stylesheet.
     */
    struct ThemeRule
    {
        std::vector<SimpleSelector> chain;
        std::string part;
        Style style;
        Specificity specificity;
        int order{0};
        std::optional<MediaQuery> media;
    };

    /**
     * \class Theme
     * \brief Global stylesheet: rules, keyframes, and root style.
     */
    class Theme
    {
    public:
        /** \brief Key for `rulesByLastSymbol`: the last selector component. */
        struct SymbolKey
        {
            SimpleSelector::Kind kind;
            std::string name;

            bool operator==(const SymbolKey &o) const noexcept
            {
                return kind == o.kind && name == o.name;
            }
        };

        /** \brief Hash for `SymbolKey`. */
        struct SymbolKeyHash
        {
            size_t operator()(const SymbolKey &k) const noexcept
            {
                return std::hash<std::string>{}(k.name) ^
                       (static_cast<size_t>(k.kind) * 0x9e3779b97f4a7c15ULL);
            }
        };

        std::vector<ThemeRule> rules;
        Style root;
        std::unordered_map<std::string, KeyframeAnimation> keyframes;
        std::unordered_map<std::string, std::vector<size_t>> rulesByTag;
        std::vector<size_t> rulesNoTag;
        std::unordered_map<SymbolKey, std::vector<uint32_t>, SymbolKeyHash> rulesByLastSymbol;
        uint32_t version{0};

        /** \brief Return the global singleton theme. */
        static Theme &get()
        {
            static Theme instance;
            return instance;
        }

        /**
         * \brief Insert a rule into the theme.
         */
        void addRule(ThemeRule r)
        {
            r.specificity = computeSpecificity(r.chain);
            r.order = (int)rules.size();

            if (r.part.empty() && !r.chain.empty())
            {
                const auto &back = r.chain.back();
                if (back.kind == SimpleSelector::Kind::Tag)
                    rulesByTag[back.name].push_back((size_t)r.order);
                else
                    rulesNoTag.push_back((size_t)r.order);

                rulesByLastSymbol[SymbolKey{back.kind, back.name}]
                    .push_back((uint32_t)r.order);
                for (const auto &[kind, name] : back.extras)
                    rulesByLastSymbol[SymbolKey{kind, name}]
                        .push_back((uint32_t)r.order);
            }

            rules.push_back(std::move(r));
            ++version;
        }

        /** \brief Insert or replace a keyframe animation and bump the version. */
        void addKeyframes(const KeyframeAnimation &anim)
        {
            keyframes[anim.name] = anim;
            ++version;
        }

        /** \brief Remove every rule, keyframe, and root declaration. */
        void clear()
        {
            rules.clear();
            keyframes.clear();
            root = Style{};
            rulesByTag.clear();
            rulesNoTag.clear();
            rulesByLastSymbol.clear();
            ++version;
        }
    };
}