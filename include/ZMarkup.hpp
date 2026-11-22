#pragma once

/**
 * \file ZMarkup.hpp
 * \brief Declarative UI markup: parser, registry, prefab system, and UINode.
 *
 * ZMarkup is a small XML-like DSL for building UI trees declaratively:
 *
 * \code{.txt}
 * VStack padding="16" {
 *     Text "Hello, world" font-size="24"
 *     Button#ok "OK"
 *     Button#cancel "Cancel" onclick="close"
 * }
 * \endcode
 *
 * The pipeline is:
 *
 *   1. **Parse** (`Parser`): string → `Element` tree (tag, classes, id, ref,
 *      attrs, text, children).
 *   2. **Build** (`Registry::create`): `Element` tree → `shared_ptr<Layout>`
 *      tree, using per-tag factories. Prefabs are expanded here.
 *   3. **Wrap** (`UINode`): the root plus a `BuildContext` (id map, ref map)
 *      for lookup and event wiring.
 *
 * \note The `Slot` element is handled specially: it either instantiates the
 *       children the *caller* passed when invoking a prefab, or falls back to
 *       the slot's own children.
 */

#include "Common.hpp"
#include "Logger.hpp"
#include "CoreTypes.hpp"
#include "Easing.hpp"
#include "Unit.hpp"
#include "Style.hpp"
#include "Layout.hpp"
#include "ZenitUIComponents.hpp"
#include "StyleAttr.hpp"
#include "Bindable.hpp"

#include <type_traits>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

namespace ZenitUI::ZMarkup
{

    /**
     * \struct Element
     * \brief Parsed representation of one markup node.
     */
    struct Element
    {
        std::string tag;
        std::vector<std::string> classes;
        std::string id;
        std::string ref;
        std::string text;
        std::vector<std::pair<std::string, std::string>> attrs;
        std::vector<Element> children;

        /** \brief Return the value of attribute \p k, or \p d if absent. */
        std::string attr(std::string_view k, std::string_view d = "") const
        {
            for (auto &[key, val] : attrs)
                if (key == k)
                    return val;
            return std::string(d);
        }
        /** \brief `true` if the element has attribute \p k (regardless of value). */
        bool has(std::string_view k) const
        {
            for (auto &[key, _] : attrs)
                if (key == k)
                    return true;
            return false;
        }
        /** \brief Return attribute \p k parsed as `float`, or \p d on failure. */
        float attrFloat(std::string_view k, float d = 0.0f) const
        {
            auto v = attr(k);
            if (v.empty())
                return d;
            try
            {
                return std::stof(v);
            }
            catch (...)
            {
                return d;
            }
        }
        /**
         * \brief Return attribute \p k as `bool`.
         *
         * Truthy values: `"true"`, `"1"`, `"yes"`. Anything else is false.
         */
        bool attrBool(std::string_view k, bool d = false) const
        {
            auto v = attr(k);
            if (v.empty())
                return d;
            return v == "true" || v == "1" || v == "yes";
        }
    };

    /**
     * \class Parser
     * \brief Single-pass recursive-descent parser for ZMarkup source.
     */
    class Parser
    {
    public:
        explicit Parser(std::string_view src) : s(src) {}

        /** \brief Parse the whole source as a single root element. */
        Element parseRoot()
        {
            skipWs();
            return parseElement();
        }

    private:
        std::string_view s;
        size_t p{0};

        bool eof() const { return p >= s.size(); }
        char peek() const { return eof() ? '\0' : s[p]; }
        char get() { return eof() ? '\0' : s[p++]; }

        static bool isIdentStart(char c)
        {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        }
        static bool isIdentChar(char c)
        {
            return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
        }

        void skipWs()
        {
            while (!eof())
            {
                char c = s[p];
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                {
                    ++p;
                    continue;
                }
                if (c == '/' && p + 1 < s.size() && s[p + 1] == '/')
                {
                    while (p < s.size() && s[p] != '\n')
                        ++p;
                    continue;
                }
                break;
            }
        }
        void skipInlineWs()
        {
            while (!eof() && (s[p] == ' ' || s[p] == '\t'))
                ++p;
        }

        std::string readIdent()
        {
            std::string out;
            while (!eof() && isIdentChar(s[p]))
                out.push_back(s[p++]);
            return out;
        }

        std::string readQuoted()
        {
            std::string out;
            if (peek() != '"')
                return out;
            ++p;
            while (!eof() && s[p] != '"')
            {
                if (s[p] == '\\' && p + 1 < s.size())
                {
                    ++p;
                    char c = s[p++];
                    if (c == 'n')
                        out.push_back('\n');
                    else if (c == 't')
                        out.push_back('\t');
                    else
                        out.push_back(c);
                }
                else
                {
                    out.push_back(s[p++]);
                }
            }
            if (!eof())
                ++p;
            return out;
        }

        std::string readBareToken()
        {
            std::string out;
            while (!eof())
            {
                char c = s[p];
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '{' || c == '}' || c == '"')
                    break;
                out.push_back(c);
                ++p;
            }
            return out;
        }

        Element parseElement()
        {
            Element e;
            e.tag = readIdent();

            if (e.tag.empty())
            {
                if (!eof())
                    ++p;
                return e;
            }

            while (peek() == '.' || peek() == '#' || peek() == '@')
            {
                char m = get();
                std::string name = readIdent();
                if (m == '.')
                {
                    e.classes.push_back(std::move(name));
                }
                else if (m == '#')
                {
                    e.id = std::move(name);
                }
                else // '@'
                {
                    if (!e.ref.empty())
                    {
                        logWarn("ZMarkup", "", 0, 0,
                                "duplicate @ref '" + name +
                                    "' on the same tag, keeping the last one");
                    }
                    e.ref = std::move(name);
                }
            }

            while (true)
            {
                skipInlineWs();
                char c = peek();
                if (c == '"' || c == '{' || c == '}' || c == '\0' || c == '\n' || c == '\r')
                    break;
                if (!isIdentStart(c))
                    break;

                std::string key = readIdent();
                skipInlineWs();
                if (peek() == '=')
                {
                    ++p;
                    skipInlineWs();
                    std::string val = (peek() == '"') ? readQuoted() : readBareToken();
                    e.attrs.emplace_back(std::move(key), std::move(val));
                }
                else
                {
                    e.attrs.emplace_back(std::move(key), "true");
                }
            }

            skipInlineWs();
            if (peek() == '"')
                e.text = readQuoted();

            skipInlineWs();
            if (peek() == '{')
            {
                ++p;
                while (true)
                {
                    skipWs();
                    if (eof() || peek() == '}')
                        break;
                    e.children.push_back(parseElement());
                }
                if (peek() == '}')
                    ++p;
            }
            return e;
        }
    };

    /** \brief Parse \p src into a single root `Element`. */
    inline Element parse(std::string_view src)
    {
        Parser p(src);
        return p.parseRoot();
    }

    class UINode;

    /**
     * \struct SlotFrame
     * \brief Per-prefab-invocation slot routing table.
     */
    struct SlotFrame
    {
        std::string prefabName;
        std::unordered_map<std::string, std::vector<const Element *>> buckets;
        std::unordered_set<std::string> seenSlots;
        std::unordered_set<std::string> consumedSlots;
    };

    /**
     * \struct ConditionBinding
     * \brief Runtime state for a structural `If@ref` block.
     */
    struct ConditionBinding
    {
        bool ownState{false};
        bool effectiveVisible{false};
        std::string parentCondition;
        std::vector<std::weak_ptr<Layout>> directNodes;
        std::vector<std::string> childConditions;
        std::vector<std::string> childEaches;
    };

    /**
     * \struct IKeyedStore
     * \brief Type-erased storage base for keyed `Each` reconciliation.
     */
    struct IKeyedStore
    {
        virtual ~IKeyedStore() = default;
    };

    struct BuildContext;

    /**
     * \struct EachBinding
     * \brief Runtime state for a structural `Each@ref prefab="..."` slice.
     */
    struct EachBinding
    {
        std::weak_ptr<Layout> anchor;
        std::string prefabName;
        std::string parentCondition;
        std::vector<Element> fallbackAst;
        std::vector<std::shared_ptr<Layout>> spawnedNodes;
        std::vector<std::shared_ptr<BuildContext>> spawnedContexts;
        std::shared_ptr<IKeyedStore> keyedStore;
        bool showingFallback{false};
    };

    /**
     * \struct BuildContext
     * \brief Per-build accumulator for `#id`, `@ref`, and runtime structural bindings.
     */
    struct BuildContext
    {
        std::unordered_map<std::string, std::shared_ptr<Layout>> byId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<Layout>>> byRef;

        std::vector<SlotFrame> slotStack;
        std::vector<std::string> conditionStack;

        std::unordered_map<std::string, ConditionBinding> conditions;
        std::unordered_map<std::string, EachBinding> eaches;

        // Type-erased Bindable<T>::Subscription handles (RAII cleanup on destroy).
        std::unordered_map<std::string, std::shared_ptr<void>> conditionSubs;
        std::unordered_map<std::string, std::shared_ptr<void>> eachSubs;

        bool isConditionVisible(const std::string &condName) const
        {
            if (condName.empty())
                return true;
            auto it = conditions.find(condName);
            return it != conditions.end() ? it->second.effectiveVisible : true;
        }
    };

    /**
     * \class Registry
     * \brief Global tag → factory map plus the prefab table.
     */
    class Registry
    {
    public:
        /** \brief Signature for tag factories: `Element` → node. */
        using Factory = std::function<std::shared_ptr<Layout>(const Element &)>;

        /** \brief Register or replace a tag factory. */
        void reg(const std::string &tag, Factory f) { factories[tag] = std::move(f); }

        /** \brief `true` if a prefab is registered under \p tagName. */
        bool hasPrefab(const std::string &tagName) const
        {
            return prefabs.count(tagName) > 0;
        }

        /**
         * \brief Register a prefab from inline markup source.
         */
        void registerPrefab(const std::string &tagName, const std::string &markupSource)
        {
            Element root = parse(markupSource);
            prefabs[tagName] = std::move(root);
        }

        /**
         * \brief Register a prefab from a file.
         * \return `false` if the file could not be read.
         */
        bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f)
            {
                logError("ZMarkup", path.string(), 0, 0, "Impossibile aprire il file per il prefab");
                return false;
            }
            std::stringstream ss;
            ss << f.rdbuf();
            registerPrefab(tagName, ss.str());
            return true;
        }

        /**
         * \brief Build one or more nodes from \p el.
         */
        std::vector<std::shared_ptr<Layout>> create_impl(
            const Element &el, BuildContext &ctx, bool isPrefabRoot = false) const
        {
            auto created = create_nodes_impl(el, ctx, isPrefabRoot);
            std::vector<std::shared_ptr<Layout>> out;
            out.reserve(created.size());
            for (auto &cn : created)
                out.push_back(std::move(cn.node));
            return out;
        }

        /**
         * \brief Convenience wrapper: build exactly one root node from \p el.
         * \return `nullptr` if the element expanded to zero nodes.
         */
        std::shared_ptr<Layout> create(const Element &el, BuildContext &ctx) const
        {
            auto res = create_impl(el, ctx);
            return res.empty() ? nullptr : res.front();
        }

        /**
         * \brief Directly instantiate a registered prefab without invoking the parser.
         */
        UINode instantiate(
            const std::string &tagName,
            const std::vector<std::string> &extraClasses = {},
            const std::string &id = "",
            const std::string &ref = "") const;

    private:
        struct CreatedNode
        {
            std::shared_ptr<Layout> node;
            bool ownedByStructural{false};
        };

        std::vector<CreatedNode> create_nodes_impl(
            const Element &el, BuildContext &ctx, bool isPrefabRoot = false) const
        {
            std::vector<CreatedNode> result;

            if (el.tag == "Slot")
            {
                const std::string slotName = el.attr("name", "");

                if (ctx.slotStack.empty())
                {
                    logWarn("ZMarkup", "", 0, 0,
                            slotName.empty()
                                ? "default Slot used outside of a prefab; rendering fallback"
                                : "Slot name='" + slotName + "' used outside of a prefab; rendering fallback");
                    for (const auto &child : el.children)
                    {
                        auto childNodes = create_nodes_impl(child, ctx);
                        result.insert(result.end(),
                                      std::make_move_iterator(childNodes.begin()),
                                      std::make_move_iterator(childNodes.end()));
                    }
                    return result;
                }

                auto &top = ctx.slotStack.back();
                if (!top.seenSlots.insert(slotName).second)
                {
                    logWarn("ZMarkup", "", 0, 0,
                            slotName.empty()
                                ? "duplicate default Slot in prefab '" + top.prefabName + "', ignoring duplicate"
                                : "duplicate Slot name='" + slotName + "' in prefab '" + top.prefabName + "', ignoring duplicate");
                    return result;
                }

                auto bucketIt = top.buckets.find(slotName);
                const bool hasInjected = (bucketIt != top.buckets.end() && !bucketIt->second.empty());

                if (hasInjected)
                {
                    top.consumedSlots.insert(slotName);

                    SlotFrame saved = std::move(ctx.slotStack.back());
                    ctx.slotStack.pop_back();

                    for (const Element *child : saved.buckets[slotName])
                    {
                        auto childNodes = create_nodes_impl(*child, ctx);
                        result.insert(result.end(),
                                      std::make_move_iterator(childNodes.begin()),
                                      std::make_move_iterator(childNodes.end()));
                    }

                    ctx.slotStack.push_back(std::move(saved));
                }
                else if (!el.children.empty())
                {
                    for (const auto &child : el.children)
                    {
                        auto childNodes = create_nodes_impl(child, ctx);
                        result.insert(result.end(),
                                      std::make_move_iterator(childNodes.begin()),
                                      std::make_move_iterator(childNodes.end()));
                    }
                }
                return result;
            }

            if (el.tag == "If")
            {
                if (el.ref.empty())
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "If block missing @ref (e.g. If@flag { ... }); skipping");
                    return result;
                }
                if (ctx.conditions.count(el.ref) > 0)
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "duplicate If @" + el.ref + " in same build context; ignoring duplicate");
                    return result;
                }

                const bool initial = el.attrBool("initial", false);
                const std::string parentCond = ctx.conditionStack.empty() ? "" : ctx.conditionStack.back();
                const bool parentEff = ctx.isConditionVisible(parentCond);
                const bool effVisible = initial && parentEff;

                ConditionBinding binding;
                binding.ownState = initial;
                binding.effectiveVisible = effVisible;
                binding.parentCondition = parentCond;
                ctx.conditions[el.ref] = std::move(binding);

                if (!parentCond.empty())
                    ctx.conditions[parentCond].childConditions.push_back(el.ref);

                ctx.conditionStack.push_back(el.ref);

                for (const auto &child : el.children)
                {
                    auto childNodes = create_nodes_impl(child, ctx);
                    auto &curBinding = ctx.conditions[el.ref];
                    for (auto &cn : childNodes)
                    {
                        if (!cn.ownedByStructural && cn.node)
                        {
                            cn.node->setVisible(effVisible);
                            curBinding.directNodes.push_back(cn.node);
                        }
                        result.push_back({std::move(cn.node), true});
                    }
                }

                ctx.conditionStack.pop_back();
                return result;
            }

            if (el.tag == "Each")
            {
                if (el.ref.empty())
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "Each block missing @ref (e.g. Each@items prefab=\"Card\"); skipping");
                    return result;
                }
                if (ctx.eaches.count(el.ref) > 0)
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "duplicate Each @" + el.ref + " in same build context; ignoring duplicate");
                    return result;
                }
                const std::string prefabName = el.attr("prefab", "");
                if (prefabName.empty())
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "Each @" + el.ref + " missing prefab=\"...\" attribute; skipping");
                    return result;
                }
                if (!hasPrefab(prefabName))
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "Each @" + el.ref + " references unregistered prefab '" + prefabName + "'");
                }

                auto anchor = std::make_shared<Layout>(LayoutType::Stack);
                anchor->setStyleTag("EachAnchor");
                anchor->setVisible(false);

                const std::string parentCond = ctx.conditionStack.empty() ? "" : ctx.conditionStack.back();
                const bool effVisible = ctx.isConditionVisible(parentCond);

                EachBinding binding;
                binding.anchor = anchor;
                binding.prefabName = prefabName;
                binding.parentCondition = parentCond;
                binding.fallbackAst = el.children;

                if (!el.children.empty())
                {
                    auto fbCtx = std::make_shared<BuildContext>();
                    binding.showingFallback = true;
                    for (const auto &fbEl : el.children)
                    {
                        auto fbNodes = create_nodes_impl(fbEl, *fbCtx);
                        for (auto &cn : fbNodes)
                        {
                            if (cn.node)
                            {
                                cn.node->setVisible(effVisible);
                                binding.spawnedNodes.push_back(cn.node);
                                result.push_back({cn.node, true});
                            }
                        }
                    }
                    binding.spawnedContexts.push_back(std::move(fbCtx));
                }

                ctx.eaches[el.ref] = std::move(binding);
                if (!parentCond.empty())
                    ctx.conditions[parentCond].childEaches.push_back(el.ref);

                result.push_back({std::move(anchor), true});
                return result;
            }

            bool alreadyExpanding = false;
            for (const auto &frame : ctx.slotStack)
            {
                if (frame.prefabName == el.tag)
                {
                    alreadyExpanding = true;
                    break;
                }
            }

            if (!alreadyExpanding)
            {
                auto prefabIt = prefabs.find(el.tag);
                if (prefabIt != prefabs.end())
                {
                    return expandPrefab(el.tag, prefabIt->second,
                                        el.classes, el.id, el.ref,
                                        el.attrs, el.children, ctx);
                }
            }

            auto it = factories.find(el.tag);
            if (it == factories.end())
                return {};

            auto node = it->second(el);
            if (!node)
                return {};

            if (el.has("passthrough"))
                node->setPassThrough(el.attrBool("passthrough"));

            for (const auto &c : el.classes)
                node->cls(c);
            if (!el.id.empty())
            {
                node->id(el.id);
                ctx.byId[el.id] = node;
            }
            if (!el.ref.empty())
            {
                node->setRef(el.ref);
                ctx.byRef[el.ref].push_back(node);
            }

            for (const auto &[k, v] : el.attrs)
            {
                if (k == "slot" || k == "passthrough")
                    continue;
                applyStyleAttr(node->getInlineBase(), k, v);
            }

            Layout *target = node->contentTarget();

            for (const auto &child : el.children)
            {
                auto childNodes = create_nodes_impl(child, ctx);
                for (auto &cn : childNodes)
                {
                    target->addChild(std::move(cn.node));
                }
            }

            result.push_back({std::move(node), false});
            return result;
        }

        std::vector<CreatedNode> expandPrefab(
            const std::string &tagName,
            const Element &prefabAst,
            const std::vector<std::string> &extraClasses,
            const std::string &id,
            const std::string &ref,
            const std::vector<std::pair<std::string, std::string>> &extraAttrs,
            const std::vector<Element> &callerChildren,
            BuildContext &ctx) const
        {
            Element instanceTree = prefabAst;

            for (const auto &c : extraClasses)
                instanceTree.classes.push_back(c);
            if (!id.empty())
                instanceTree.id = id;
            if (!ref.empty())
                instanceTree.ref = ref;
            for (const auto &attr : extraAttrs)
            {
                if (attr.first != "slot")
                    instanceTree.attrs.push_back(attr);
            }

            SlotFrame frame;
            frame.prefabName = tagName;
            for (const auto &child : callerChildren)
            {
                std::string targetSlot = child.attr("slot", "");
                frame.buckets[targetSlot].push_back(&child);
            }

            ctx.slotStack.push_back(std::move(frame));
            auto nodes = create_nodes_impl(instanceTree, ctx, true);

            SlotFrame finished = std::move(ctx.slotStack.back());
            ctx.slotStack.pop_back();

            for (const auto &[slotName, elems] : finished.buckets)
            {
                if (!elems.empty() && !finished.consumedSlots.count(slotName))
                {
                    if (slotName.empty())
                    {
                        logWarn("ZMarkup", "", 0, 0,
                                "prefab '" + tagName +
                                    "' has no default Slot; ignoring " +
                                    std::to_string(elems.size()) + " child(ren)");
                    }
                    else
                    {
                        logWarn("ZMarkup", "", 0, 0,
                                "prefab '" + tagName +
                                    "' has no Slot named '" + slotName +
                                    "'; ignoring " +
                                    std::to_string(elems.size()) + " child(ren)");
                    }
                }
            }

            return nodes;
        }

        std::unordered_map<std::string, Factory> factories;
        std::unordered_map<std::string, Element> prefabs;
    };

    /**
     * \brief Register the built-in widgets into \p r.
     */
    inline void registerBuiltins(Registry &r)
    {
        using namespace ZenitUI::UI;

        r.reg("Stack", [](const Element &)
              {
        auto l = std::make_shared<Layout>(LayoutType::Stack);
        l->setStyleTag("Stack");
        return l; });
        r.reg("Spacer", [](const Element &)
              {
        auto l = std::make_shared<Layout>(LayoutType::Stack);
        l->setStyleTag("Spacer");
        return l; });
        r.reg("VStack", [](const Element &)
              { return VStack(); });
        r.reg("HStack", [](const Element &)
              { return HStack(); });
        r.reg("Box", [](const Element &)
              { return Box(); });

        r.reg("Text", [](const Element &e)
              {
        auto t = Text::create(e.text);
        if (e.has("wrap")) t->setWrap(e.attrBool("wrap", true));
        return t; });
        r.reg("Label", [](const Element &e)
              {
        auto t = Text::create(e.text);
        if (e.has("wrap")) t->setWrap(e.attrBool("wrap", true));
        return t; });

        r.reg("TextInput", [](const Element &e)
              {
                  auto ti = TextInput::create(e.attr("value", e.text));
                  if (e.has("placeholder"))
                      ti->setPlaceholder(e.attr("placeholder", ""));
                  if (e.has("placeholder-wrap"))
                      ti->setPlaceholderWrap(e.attr("placeholder-wrap", "false") == "true");
                  return ti;
              });
        r.reg("TextArea", [](const Element &e)
              {
                  auto ta = TextArea::create(e.attr("value", e.text),
                                             std::max(1, (int)e.attrFloat("rows", 3.0f)));
                  if (e.has("placeholder"))
                      ta->setPlaceholder(e.attr("placeholder", ""));
                  if (e.has("placeholder-wrap"))
                      ta->setPlaceholderWrap(e.attr("placeholder-wrap", "true") != "false");
                  return ta;
              });
        r.reg("Checkbox", [](const Element &e)
              { return Checkbox::create(e.attrBool("checked", false)); });
        r.reg("ProgressBar", [](const Element &e)
              { return ProgressBar::create(e.attrFloat("value", 0.0f)); });
        r.reg("Dropdown", [](const Element &e)
              {
        std::vector<std::string> opts;
        std::string cur;
        auto flush = [&]() {
            size_t a = cur.find_first_not_of(" \t");
            size_t b = cur.find_last_not_of(" \t");
            if (a != std::string::npos) opts.push_back(cur.substr(a, b - a + 1));
            cur.clear();
        };
        for (char c : e.attr("options", "")) {
            if (c == ',') flush(); else cur.push_back(c);
        }
        flush();
        if (opts.empty()) opts.push_back("Default");
        int sel = std::clamp((int)e.attrFloat("selected", 0.0f), 0, (int)opts.size() - 1);
        return Dropdown::create(std::move(opts), sel); });
        r.reg("Button", [](const Element &e)
              { return Btn(e.text); });
        r.reg("Panel", [](const Element &)
              { return Pan(); });
        r.reg("Slider", [](const Element &e)
              { return Slider::create(e.attrFloat("value", 0.5f)); });
        r.reg("Toggle", [](const Element &e)
              { return Toggle::create(e.attrBool("checked", false)); });
        r.reg("ScrollView", [](const Element &)
              { return ScrollView::create(); });
        r.reg("Canvas", [](const Element &)
              { return CanvasLayout::create(); });
        r.reg("Modal", [](const Element &)
              { return Modal::create(); });
        r.reg("Popup", [](const Element &)
              { return Popup::create(); });
    }

    /**
     * \brief Return the global registry, initializing built-ins on first call.
     */
    inline Registry &getGlobalRegistry()
    {
        static Registry registry;
        static bool initialized = false;
        if (!initialized)
        {
            registerBuiltins(registry);
            initialized = true;
        }
        return registry;
    }

    /**
     * \class UINode
     * \brief Root handle returned by `build`: root node + build context + event-wiring helpers.
     */
    class UINode
    {
    public:
        UINode(std::shared_ptr<Layout> root, std::shared_ptr<BuildContext> ctx)
            : root_(std::move(root)), ctx_(std::move(ctx)) {}

        std::shared_ptr<Layout> root() const { return root_; }
        std::shared_ptr<BuildContext> context() const { return ctx_; }

        /**
         * \brief Look up a node by its `#id`.
         * \return The node, or `nullptr` if not found (warns once per id).
         */
        std::shared_ptr<Layout> find(const std::string &id) const
        {
            auto it = ctx_->byId.find(id);
            if (it == ctx_->byId.end())
            {
                warnOnce("find:" + id,
                         "UINode::find('" + id + "'): no node with this id");
                return nullptr;
            }
            return it->second;
        }

        /** \brief Typed `find`: casts the result to `T`. */
        template <typename T = Layout>
        std::shared_ptr<T> find(const std::string &id) const
        {
            auto n = find(id);
            return n ? std::static_pointer_cast<T>(n) : nullptr;
        }

        /**
         * \brief Look up a node by `@ref`.
         */
        template <typename T = Layout>
        std::shared_ptr<T> ref(const std::string &name) const
        {
            auto it = ctx_->byRef.find(name);
            if (it == ctx_->byRef.end() || it->second.empty())
            {
                warnOnce("ref:" + name,
                         "UINode::ref('" + name + "'): no node with this ref");
                return nullptr;
            }
            if (it->second.size() > 1)
            {
                warnOnce("ref-ambig:" + name,
                         "UINode::ref('" + name + "'): " +
                             std::to_string(it->second.size()) +
                             " nodes match, returning the first. "
                             "Use refAll for the full list.");
            }
            return std::static_pointer_cast<T>(it->second.front());
        }

        /** \brief Return every node sharing the given `@ref`. */
        template <typename T = Layout>
        std::vector<std::shared_ptr<T>> refAll(const std::string &name) const
        {
            auto it = ctx_->byRef.find(name);
            if (it == ctx_->byRef.end() || it->second.empty())
            {
                logDebug("ZMarkup", "", 0, 0,
                         "UINode::refAll('" + name + "'): empty");
                return {};
            }
            std::vector<std::shared_ptr<T>> out;
            out.reserve(it->second.size());
            for (const auto &n : it->second)
                out.push_back(std::static_pointer_cast<T>(n));
            return out;
        }

        UINode &onClick(const std::string &id, std::function<void()> cb)
        {
            if (auto n = find(id))
                n->onClick = std::move(cb);
            return *this;
        }
        UINode &onPress(const std::string &id, std::function<void()> cb)
        {
            if (auto n = find(id))
                n->onPress = std::move(cb);
            return *this;
        }
        UINode &onRelease(const std::string &id, std::function<void()> cb)
        {
            if (auto n = find(id))
                n->onRelease = std::move(cb);
            return *this;
        }
        UINode &onHoverEnter(const std::string &id, std::function<void()> cb)
        {
            if (auto n = find(id))
                n->onHoverEnter = std::move(cb);
            return *this;
        }
        UINode &onHoverExit(const std::string &id, std::function<void()> cb)
        {
            if (auto n = find(id))
                n->onHoverExit = std::move(cb);
            return *this;
        }
        /** \brief Wire a `Slider`'s `onValueChanged` by id. No-op if the id is not a Slider. */
        UINode &onValueChanged(const std::string &id, std::function<void(float)> cb)
        {
            if (auto s = find<UI::Slider>(id))
                s->onValueChanged = std::move(cb);
            return *this;
        }
        /** \brief Wire a `Toggle`'s `onToggle` by id. No-op if the id is not a Toggle. */
        UINode &onToggle(const std::string &id, std::function<void(bool)> cb)
        {
            if (auto t = find<UI::Toggle>(id))
                t->onToggle = std::move(cb);
            return *this;
        }

        // --- @ref-based chainable helpers (§5.36) ---

        UINode &setTextRef(const std::string &refName, const std::string &text)
        {
            auto n = ref<Layout>(refName);
            if (!n)
                return *this;
            if (auto t = std::dynamic_pointer_cast<UI::Text>(n))
                t->setText(text);
            else if (auto b = std::dynamic_pointer_cast<UI::Button>(n))
                b->setText(text);
            else if (auto ti = std::dynamic_pointer_cast<UI::TextInput>(n))
                ti->setText(text);
            else if (auto ta = std::dynamic_pointer_cast<UI::TextArea>(n))
                ta->setText(text);
            else
                warnOnce("setTextRef-type:" + refName,
                         "UINode::setTextRef('" + refName + "'): node does not support setText");
            return *this;
        }

        UINode &setButtonTextRef(const std::string &refName, const std::string &text)
        {
            auto n = ref<Layout>(refName);
            if (!n)
                return *this;
            if (auto b = std::dynamic_pointer_cast<UI::Button>(n))
                b->setText(text);
            else
                warnOnce("setButtonTextRef-type:" + refName,
                         "UINode::setButtonTextRef('" + refName + "'): node is not a Button");
            return *this;
        }

        UINode &addClassRef(const std::string &refName, const std::string &cls)
        {
            if (auto n = ref<Layout>(refName))
                n->addClass(cls);
            return *this;
        }

        UINode &removeClassRef(const std::string &refName, const std::string &cls)
        {
            if (auto n = ref<Layout>(refName))
                n->removeClass(cls);
            return *this;
        }

        UINode &toggleClassRef(const std::string &refName, const std::string &cls, bool active)
        {
            if (auto n = ref<Layout>(refName))
            {
                if (active)
                    n->addClass(cls);
                else
                    n->removeClass(cls);
            }
            return *this;
        }

        UINode &setEnabledRef(const std::string &refName, bool enabled)
        {
            if (auto n = ref<Layout>(refName))
                n->setEnabled(enabled);
            return *this;
        }

        UINode &setInteractiveRef(const std::string &refName, bool interactive)
        {
            if (auto n = ref<Layout>(refName))
                n->setInteractive(interactive);
            return *this;
        }

        UINode &onClickRef(const std::string &refName, std::function<void()> cb)
        {
            if (auto n = ref<Layout>(refName))
                n->onClick = std::move(cb);
            return *this;
        }

        UINode &onPressRef(const std::string &refName, std::function<void()> cb)
        {
            if (auto n = ref<Layout>(refName))
                n->onPress = std::move(cb);
            return *this;
        }

        UINode &onReleaseRef(const std::string &refName, std::function<void()> cb)
        {
            if (auto n = ref<Layout>(refName))
                n->onRelease = std::move(cb);
            return *this;
        }

        UINode &onHoverEnterRef(const std::string &refName, std::function<void()> cb)
        {
            if (auto n = ref<Layout>(refName))
                n->onHoverEnter = std::move(cb);
            return *this;
        }

        UINode &onHoverExitRef(const std::string &refName, std::function<void()> cb)
        {
            if (auto n = ref<Layout>(refName))
                n->onHoverExit = std::move(cb);
            return *this;
        }

        UINode &onTextChangedRef(const std::string &refName,
                                 std::function<void(const std::string &)> cb)
        {
            auto n = ref<Layout>(refName);
            if (!n)
                return *this;
            if (auto ti = std::dynamic_pointer_cast<UI::TextInput>(n))
                ti->onTextChanged = std::move(cb);
            else if (auto ta = std::dynamic_pointer_cast<UI::TextArea>(n))
                ta->onTextChanged = std::move(cb);
            else
                warnOnce("onTextChangedRef-type:" + refName,
                         "UINode::onTextChangedRef('" + refName + "'): node is not a TextInput or TextArea");
            return *this;
        }

        UINode &onValueChangedRef(const std::string &refName, std::function<void(float)> cb)
        {
            auto n = ref<Layout>(refName);
            if (!n)
                return *this;
            if (auto s = std::dynamic_pointer_cast<UI::Slider>(n))
                s->onValueChanged = std::move(cb);
            else
                warnOnce("onValueChangedRef-type:" + refName,
                         "UINode::onValueChangedRef('" + refName + "'): node is not a Slider");
            return *this;
        }

        UINode &onToggleRef(const std::string &refName, std::function<void(bool)> cb)
        {
            auto n = ref<Layout>(refName);
            if (!n)
                return *this;
            if (auto t = std::dynamic_pointer_cast<UI::Toggle>(n))
                t->onToggle = std::move(cb);
            else
                warnOnce("onToggleRef-type:" + refName,
                         "UINode::onToggleRef('" + refName + "'): node is not a Toggle");
            return *this;
        }

        UINode &setVisibleRef(const std::string &refName, bool visible)
        {
            if (auto n = ref<Layout>(refName))
                n->setVisible(visible);
            return *this;
        }

        // --- Runtime structural If & Each (§5.37) ---

        /**
         * \brief Imperatively toggle a structural `If@condRef` block at runtime.
         */
        UINode &setCondition(const std::string &condRef, bool value)
        {
            auto it = ctx_->conditions.find(condRef);
            if (it == ctx_->conditions.end())
            {
                warnOnce("cond:" + condRef,
                         "UINode::setCondition('" + condRef + "'): no If with this ref");
                return *this;
            }
            it->second.ownState = value;
            bool parentEff = ctx_->isConditionVisible(it->second.parentCondition);
            propagateCondition_(*ctx_, condRef, parentEff);
            return *this;
        }

        /**
         * \brief Reactively bind a structural `If@condRef` block to a `Bindable<bool>`.
         *
         * The subscription lives in this `UINode`'s `BuildContext` and automatically
         * disconnects when the `UINode` (and its context) is destroyed.
         */
        UINode &bindCondition(const std::string &condRef,
                              Bindable<bool> &source,
                              bool fireImmediately = true)
        {
            if (ctx_->conditions.find(condRef) == ctx_->conditions.end())
            {
                warnOnce("cond:" + condRef,
                         "UINode::bindCondition('" + condRef + "'): no If with this ref");
                return *this;
            }

            std::weak_ptr<Layout> weakRoot = root_;
            std::weak_ptr<BuildContext> weakCtx = ctx_;
            auto sub = source.onChanged(
                [weakRoot, weakCtx, condRef](const bool &v)
                {
                    auto c = weakCtx.lock();
                    if (!c)
                        return;
                    UINode ui(weakRoot.lock(), std::move(c));
                    ui.setCondition(condRef, v);
                },
                fireImmediately);

            ctx_->conditionSubs[condRef] =
                std::make_shared<typename Bindable<bool>::Subscription>(std::move(sub));
            return *this;
        }

        /**
         * \brief Imperatively synchronize an `Each@eachRef` slice with \p items.
         *
         * Replaces only the nodes owned by this `Each`, preserving all sibling nodes
         * before and after it. When \p items is empty, renders the `Each`'s fallback
         * children (if any).
         */
        template <typename Range, typename Fn>
        UINode &syncEach(const std::string &eachRef, const Range &items, Fn &&binder)
        {
            auto it = ctx_->eaches.find(eachRef);
            if (it == ctx_->eaches.end())
            {
                warnOnce("each:" + eachRef,
                         "UINode::syncEach('" + eachRef + "'): no Each with this ref");
                return *this;
            }

            auto &binding = it->second;
            auto anchor = binding.anchor.lock();
            auto parent = anchor ? anchor->getParent() : nullptr;
            if (!parent)
                return *this;

            binding.keyedStore.reset();
            clearEachNodes_(binding, *parent);

            const bool effVis = ctx_->isConditionVisible(binding.parentCondition);
            const bool isEmpty = (std::begin(items) == std::end(items));

            if (isEmpty)
            {
                mountEachFallback_(binding, *parent, anchor.get(), effVis);
                return *this;
            }

            binding.showingFallback = false;
            size_t idx = 0;
            for (const auto &elem : items)
            {
                UINode itemUi = getGlobalRegistry().instantiate(binding.prefabName);
                auto itemRoot = itemUi.root();
                if (!itemRoot)
                    break;

                invokeEachCallback_(binder, itemUi, elem, idx);
                if (!effVis)
                    itemRoot->setVisible(false);

                parent->insertChildBefore(anchor.get(), itemRoot);
                binding.spawnedNodes.push_back(std::move(itemRoot));
                binding.spawnedContexts.push_back(itemUi.context());
                ++idx;
            }
            return *this;
        }

        /**
         * \brief Reactively bind an `Each@eachRef` slice to a `Bindable<std::vector<T>>`.
         */
        template <typename T, typename Fn>
        UINode &bindEach(const std::string &eachRef,
                         Bindable<std::vector<T>> &source,
                         Fn binder,
                         bool fireImmediately = true)
        {
            if (ctx_->eaches.find(eachRef) == ctx_->eaches.end())
            {
                warnOnce("each:" + eachRef,
                         "UINode::bindEach('" + eachRef + "'): no Each with this ref");
                return *this;
            }

            std::weak_ptr<Layout> weakRoot = root_;
            std::weak_ptr<BuildContext> weakCtx = ctx_;
            auto sub = source.onChanged(
                [weakRoot, weakCtx, eachRef, binder = std::move(binder)](const std::vector<T> &vec) mutable
                {
                    auto c = weakCtx.lock();
                    if (!c)
                        return;
                    UINode ui(weakRoot.lock(), std::move(c));
                    ui.syncEach(eachRef, vec, binder);
                },
                fireImmediately);

            ctx_->eachSubs[eachRef] =
                std::make_shared<typename Bindable<std::vector<T>>::Subscription>(std::move(sub));
            return *this;
        }

        /**
         * \brief Keyed reconciliation for an `Each@eachRef` slice.
         *
         * Existing keys reuse their previously instantiated `UINode` (preserving
         * focus, input state, and animations) and run only \p onUpdate. New keys
         * instantiate the prefab and run \p onCreate followed by \p onUpdate.
         * Missing keys are detached and destroyed.
         */
        template <typename Range, typename KeyFn, typename OnCreateFn, typename OnUpdateFn>
        UINode &syncEachKeyed(const std::string &eachRef,
                              const Range &items,
                              KeyFn &&keyFn,
                              OnCreateFn &&onCreate,
                              OnUpdateFn &&onUpdate)
        {
            auto it = ctx_->eaches.find(eachRef);
            if (it == ctx_->eaches.end())
            {
                warnOnce("each:" + eachRef,
                         "UINode::syncEachKeyed('" + eachRef + "'): no Each with this ref");
                return *this;
            }

            auto &binding = it->second;
            auto anchor = binding.anchor.lock();
            auto parent = anchor ? anchor->getParent() : nullptr;
            if (!parent)
                return *this;

            using ElemT = std::decay_t<decltype(*std::begin(items))>;
            using KeyT = std::decay_t<std::invoke_result_t<KeyFn &, const ElemT &>>;

            struct KeyedStore : IKeyedStore
            {
                struct Entry
                {
                    std::shared_ptr<Layout> root;
                    std::shared_ptr<BuildContext> ctx;
                };
                std::unordered_map<KeyT, Entry> entries;
            };

            const bool effVis = ctx_->isConditionVisible(binding.parentCondition);
            const bool isEmpty = (std::begin(items) == std::end(items));

            if (isEmpty)
            {
                binding.keyedStore.reset();
                clearEachNodes_(binding, *parent);
                mountEachFallback_(binding, *parent, anchor.get(), effVis);
                return *this;
            }

            if (binding.showingFallback)
                clearEachNodes_(binding, *parent);

            auto *store = dynamic_cast<KeyedStore *>(binding.keyedStore.get());
            if (!store)
            {
                auto newStore = std::make_shared<KeyedStore>();
                store = newStore.get();
                binding.keyedStore = std::move(newStore);
            }

            std::unordered_map<KeyT, typename KeyedStore::Entry> nextEntries;
            nextEntries.reserve(store->entries.size() + 4);
            std::vector<std::shared_ptr<Layout>> nextNodes;
            std::vector<std::shared_ptr<BuildContext>> nextContexts;

            size_t idx = 0;
            for (const auto &elem : items)
            {
                KeyT key = keyFn(elem);
                if (nextEntries.count(key) > 0)
                {
                    logWarn("ZMarkup", "", 0, 0,
                            "UINode::syncEachKeyed('" + eachRef + "'): duplicate key detected, skipping duplicate item");
                    ++idx;
                    continue;
                }

                auto existing = store->entries.find(key);
                if (existing != store->entries.end())
                {
                    auto entry = std::move(existing->second);
                    UINode itemUi(entry.root, entry.ctx);
                    invokeEachCallback_(onUpdate, itemUi, elem, idx);
                    if (!effVis)
                        entry.root->setVisible(false);

                    nextNodes.push_back(entry.root);
                    nextContexts.push_back(entry.ctx);
                    nextEntries.emplace(std::move(key), std::move(entry));
                }
                else
                {
                    UINode itemUi = getGlobalRegistry().instantiate(binding.prefabName);
                    auto itemRoot = itemUi.root();
                    if (!itemRoot)
                        break;

                    invokeEachCallback_(onCreate, itemUi, elem, idx);
                    invokeEachCallback_(onUpdate, itemUi, elem, idx);
                    if (!effVis)
                        itemRoot->setVisible(false);

                    nextNodes.push_back(itemRoot);
                    nextContexts.push_back(itemUi.context());
                    nextEntries.emplace(std::move(key),
                                        typename KeyedStore::Entry{std::move(itemRoot), itemUi.context()});
                }
                ++idx;
            }

            bool sameOrder = (nextNodes.size() == binding.spawnedNodes.size());
            if (sameOrder)
            {
                for (size_t i = 0; i < nextNodes.size(); ++i)
                {
                    if (nextNodes[i].get() != binding.spawnedNodes[i].get())
                    {
                        sameOrder = false;
                        break;
                    }
                }
            }

            if (!sameOrder)
            {
                for (auto &oldNode : binding.spawnedNodes)
                {
                    if (oldNode)
                        parent->removeChild(oldNode.get());
                }
                for (auto &newNode : nextNodes)
                {
                    parent->insertChildBefore(anchor.get(), newNode);
                }
            }

            store->entries = std::move(nextEntries);
            binding.spawnedNodes = std::move(nextNodes);
            binding.spawnedContexts = std::move(nextContexts);
            binding.showingFallback = false;
            return *this;
        }

        /**
         * \brief Reactively bind a keyed `Each@eachRef` slice to a `Bindable<std::vector<T>>`.
         */
        template <typename T, typename KeyFn, typename OnCreateFn, typename OnUpdateFn>
        UINode &bindEachKeyed(const std::string &eachRef,
                              Bindable<std::vector<T>> &source,
                              KeyFn keyFn,
                              OnCreateFn onCreate,
                              OnUpdateFn onUpdate,
                              bool fireImmediately = true)
        {
            if (ctx_->eaches.find(eachRef) == ctx_->eaches.end())
            {
                warnOnce("each:" + eachRef,
                         "UINode::bindEachKeyed('" + eachRef + "'): no Each with this ref");
                return *this;
            }

            std::weak_ptr<Layout> weakRoot = root_;
            std::weak_ptr<BuildContext> weakCtx = ctx_;
            auto sub = source.onChanged(
                [weakRoot, weakCtx, eachRef,
                 keyFn = std::move(keyFn),
                 onCreate = std::move(onCreate),
                 onUpdate = std::move(onUpdate)](const std::vector<T> &vec) mutable
                {
                    auto c = weakCtx.lock();
                    if (!c)
                        return;
                    UINode ui(weakRoot.lock(), std::move(c));
                    ui.syncEachKeyed(eachRef, vec, keyFn, onCreate, onUpdate);
                },
                fireImmediately);

            ctx_->eachSubs[eachRef] =
                std::make_shared<typename Bindable<std::vector<T>>::Subscription>(std::move(sub));
            return *this;
        }

    private:
        /** \brief Log a warning for \p key only the first time it is called. */
        void warnOnce(const std::string &key, const std::string &msg) const
        {
            if (warnedKeys_.insert(key).second)
                logWarn("ZMarkup", "", 0, 0, msg);
        }

        static void propagateCondition_(BuildContext &ctx, const std::string &condName, bool parentEffective)
        {
            auto it = ctx.conditions.find(condName);
            if (it == ctx.conditions.end())
                return;
            auto &b = it->second;
            const bool eff = b.ownState && parentEffective;
            b.effectiveVisible = eff;

            for (auto &wp : b.directNodes)
            {
                if (auto n = wp.lock())
                    n->setVisible(eff);
            }
            for (const auto &eachName : b.childEaches)
            {
                auto eit = ctx.eaches.find(eachName);
                if (eit == ctx.eaches.end())
                    continue;
                for (auto &n : eit->second.spawnedNodes)
                {
                    if (n)
                        n->setVisible(eff);
                }
            }
            for (const auto &childCond : b.childConditions)
            {
                propagateCondition_(ctx, childCond, eff);
            }
        }

        static void clearEachNodes_(EachBinding &binding, Layout &parent)
        {
            for (auto &n : binding.spawnedNodes)
            {
                if (n)
                    parent.removeChild(n.get());
            }
            binding.spawnedNodes.clear();
            binding.spawnedContexts.clear();
            binding.showingFallback = false;
        }

        static void mountEachFallback_(EachBinding &binding, Layout &parent, const Layout *anchor, bool effVis)
        {
            if (binding.fallbackAst.empty())
                return;
            auto fbCtx = std::make_shared<BuildContext>();
            binding.showingFallback = true;
            auto &reg = getGlobalRegistry();
            for (const auto &fbEl : binding.fallbackAst)
            {
                auto fbNodes = reg.create_impl(fbEl, *fbCtx);
                for (auto &n : fbNodes)
                {
                    if (!n)
                        continue;
                    n->setVisible(effVis);
                    parent.insertChildBefore(anchor, n);
                    binding.spawnedNodes.push_back(std::move(n));
                }
            }
            binding.spawnedContexts.push_back(std::move(fbCtx));
        }

        template <typename Fn, typename Elem>
        static void invokeEachCallback_(Fn &fn, UINode &itemUi, const Elem &elem, size_t idx)
        {
            if constexpr (std::is_invocable_v<Fn &, UINode &, const Elem &, size_t>)
                fn(itemUi, elem, idx);
            else if constexpr (std::is_invocable_v<Fn &, UINode &, const Elem &>)
                fn(itemUi, elem);
            else
                fn(itemUi);
        }

        std::shared_ptr<Layout> root_;
        std::shared_ptr<BuildContext> ctx_;
        mutable std::unordered_set<std::string> warnedKeys_;
    };


    /** \brief Free-function wrapper around `getGlobalRegistry().registerPrefab`. */
    inline void registerPrefab(const std::string &tagName, const std::string &markupSource)
    {
        getGlobalRegistry().registerPrefab(tagName, markupSource);
    }

    /** \brief Free-function wrapper around `getGlobalRegistry().registerPrefabFile`. */
    inline bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
    {
        return getGlobalRegistry().registerPrefabFile(tagName, path);
    }

    /** \brief Parse \p source and build the corresponding UI tree. */
    inline UINode build(std::string_view source)
    {
        auto ctx = std::make_shared<BuildContext>();
        auto el = parse(source);
        auto root = getGlobalRegistry().create(el, *ctx);
        return UINode(root, ctx);
    }

    /**
     * \brief Load markup from a file and build the UI tree.
     *
     * On read failure, logs an error and returns an empty `UINode` (root is
     * `nullptr`). Callers should check `ui.root() != nullptr`.
     */
    inline UINode loadMarkupFile(const std::filesystem::path &path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            logError("ZMarkup", path.string(), 0, 0, "impossibile aprire il file");
            return UINode(nullptr, std::make_shared<BuildContext>());
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return build(ss.str());
    }

    inline UINode Registry::instantiate(
        const std::string &tagName,
        const std::vector<std::string> &extraClasses,
        const std::string &id,
        const std::string &ref) const
    {
        auto ctx = std::make_shared<BuildContext>();
        auto it = prefabs.find(tagName);
        if (it == prefabs.end())
        {
            logWarn("ZMarkup", "", 0, 0,
                    "Registry::instantiate('" + tagName + "'): no prefab registered with this name");
            return UINode(nullptr, ctx);
        }
        auto nodes = expandPrefab(tagName, it->second, extraClasses, id, ref, {}, {}, *ctx);
        return UINode(nodes.empty() ? nullptr : nodes.front().node, ctx);
    }

    /** \brief Free-function wrapper around `getGlobalRegistry().hasPrefab`. */
    inline bool hasPrefab(const std::string &tagName)
    {
        return getGlobalRegistry().hasPrefab(tagName);
    }

    /** \brief Free-function wrapper around `getGlobalRegistry().instantiate`. */
    inline UINode instantiate(
        const std::string &tagName,
        const std::vector<std::string> &extraClasses = {},
        const std::string &id = "",
        const std::string &ref = "")
    {
        return getGlobalRegistry().instantiate(tagName, extraClasses, id, ref);
    }

} // namespace ZenitUI::ZMarkup