#pragma once

/**
 * \file ZMarkup.hpp
 * \brief Declarative UI markup: parser, registry, prefab system, UINode, and WeakUINode.
 *
 * Non-template implementation lives in `src/ZMarkup.cpp` (compiled into `libZenitUI`).
 * This header retains type definitions, declarations, and template methods only.
 */

#include "Common.hpp"
#include "Logger.hpp"
#include "Layout.hpp"
#include "Bindable.hpp"

#include <filesystem>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ZenitUI::ZMarkup
{
    /** \addtogroup grp_zmarkup
     *  @{
     */

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
        std::string attr(std::string_view k, std::string_view d = "") const;
        /** \brief `true` if the element has attribute \p k (regardless of value). */
        bool has(std::string_view k) const;
        /** \brief Return attribute \p k parsed as `float`, or \p d on failure. */
        float attrFloat(std::string_view k, float d = 0.0f) const;
        /** \brief Return attribute \p k as `bool` (`"true"`, `"1"`, `"yes"`). */
        bool attrBool(std::string_view k, bool d = false) const;
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
        Element parseRoot();

    private:
        std::string_view s;
        size_t p{0};

        bool eof() const { return p >= s.size(); }
        char peek() const { return eof() ? '\0' : s[p]; }
        char get() { return eof() ? '\0' : s[p++]; }

        static bool isIdentStart(char c);
        static bool isIdentChar(char c);

        void skipWs();
        void skipInlineWs();
        std::string readIdent();
        std::string readQuoted();
        std::string readBareToken();
        Element parseElement();
    };

    /** \brief Parse \p src into a single root `Element`. */
    Element parse(std::string_view src);

    class UINode;
    class WeakUINode;

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
        std::vector<std::weak_ptr<Layout>> spawnedNodes;
        std::shared_ptr<IKeyedStore> keyedStore;
        bool showingFallback{false};
    };

    /**
     * \struct BuildContext
     * \brief Per-build accumulator for `#id`, `@ref`, and runtime structural bindings.
     *
     * Holds only non-owning (`weak_ptr`) references to `Layout` nodes so that the
     * root `Layout` can safely retain `BuildContext` via `retainObject` without
     * introducing reference cycles.
     */
    struct BuildContext
    {
        std::unordered_map<std::string, std::weak_ptr<Layout>> byId;
        std::unordered_map<std::string, std::vector<std::weak_ptr<Layout>>> byRef;

        std::vector<SlotFrame> slotStack;
        std::vector<std::string> conditionStack;

        std::unordered_map<std::string, ConditionBinding> conditions;
        std::unordered_map<std::string, EachBinding> eaches;

        // Type-erased Bindable<T>::Subscription handles (RAII cleanup on destroy).
        std::unordered_map<std::string, std::shared_ptr<void>> conditionSubs;
        std::unordered_map<std::string, std::shared_ptr<void>> eachSubs;

        bool attachedToRoot{false};

        bool isConditionVisible(const std::string &condName) const;
    };

    template <typename KeyT>
    struct KeyedStore : IKeyedStore
    {
        struct Entry
        {
            std::weak_ptr<Layout> root;
            std::shared_ptr<BuildContext> ctx;
        };
        std::unordered_map<KeyT, Entry> entries;
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
        void reg(const std::string &tag, Factory f);

        /** \brief `true` if a prefab is registered under \p tagName. */
        bool hasPrefab(const std::string &tagName) const;

        /** \brief Register a prefab from inline markup source. */
        void registerPrefab(const std::string &tagName, const std::string &markupSource);

        /**
         * \brief Register a prefab from a file.
         * \return `false` if the file could not be read.
         */
        bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path);

        /** \brief Build one or more nodes from \p el. */
        std::vector<std::shared_ptr<Layout>> create_impl(
            const Element &el, BuildContext &ctx, bool isPrefabRoot = false) const;

        /**
         * \brief Convenience wrapper: build exactly one root node from \p el.
         * \return `nullptr` if the element expanded to zero nodes.
         */
        std::shared_ptr<Layout> create(const Element &el, BuildContext &ctx) const;

        /** \brief Directly instantiate a registered prefab without invoking the parser. */
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
            const Element &el, BuildContext &ctx, bool isPrefabRoot = false) const;

        std::vector<CreatedNode> expandPrefab(
            const std::string &tagName,
            const Element &prefabAst,
            const std::vector<std::string> &extraClasses,
            const std::string &id,
            const std::string &ref,
            const std::vector<std::pair<std::string, std::string>> &extraAttrs,
            const std::vector<Element> &callerChildren,
            BuildContext &ctx) const;

        std::unordered_map<std::string, Factory> factories;
        std::unordered_map<std::string, Element> prefabs;
    };

    /** \brief Register the built-in widgets into \p r. */
    void registerBuiltins(Registry &r);

    /** \brief Return the global registry, initializing built-ins on first call. */
    Registry &getGlobalRegistry();

    /**
     * \class UINode
     * \brief Root handle returned by `build` / `instantiate`: root node + build context + wiring helpers.
     */
    class UINode
    {
    public:
        UINode(std::shared_ptr<Layout> root, std::shared_ptr<BuildContext> ctx)
            : root_(std::move(root)), ctx_(std::move(ctx)) {}

        std::shared_ptr<Layout> root() const { return root_; }
        std::shared_ptr<BuildContext> context() const { return ctx_; }

        /**
         * \brief Return a non-owning `WeakUINode` safe to capture in child callbacks.
         *
         * Lazily binds this `BuildContext`'s lifetime to `root_` so the context
         * stays alive as long as the root remains mounted in the UI tree.
         */
        WeakUINode weak() const;

        /**
         * \brief Look up a node by its `#id`.
         * \return The node, or `nullptr` if not found or expired (warns once per id).
         */
        std::shared_ptr<Layout> find(const std::string &id) const;

        /** \brief Typed `find`: casts the result to `T`. */
        template <typename T = Layout>
        std::shared_ptr<T> find(const std::string &id) const
        {
            auto n = find(id);
            return n ? std::static_pointer_cast<T>(n) : nullptr;
        }

        /** \brief Look up a live node by `@ref`. */
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
            std::shared_ptr<Layout> firstLive;
            size_t liveCount = 0;
            for (const auto &wp : it->second)
            {
                if (auto sp = wp.lock())
                {
                    if (!firstLive)
                        firstLive = sp;
                    ++liveCount;
                }
            }
            if (!firstLive)
                return nullptr;
            if (liveCount > 1)
            {
                warnOnce("ref-ambig:" + name,
                         "UINode::ref('" + name + "'): " +
                             std::to_string(liveCount) +
                             " nodes match, returning the first. "
                             "Use refAll for the full list.");
            }
            return std::static_pointer_cast<T>(firstLive);
        }

        /** \brief Return every live node sharing the given `@ref`. */
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
            for (const auto &wp : it->second)
            {
                if (auto sp = wp.lock())
                    out.push_back(std::static_pointer_cast<T>(sp));
            }
            return out;
        }

        // --- #id-based event helpers ---

        UINode &onClick(const std::string &id, std::function<void()> cb);
        UINode &onPress(const std::string &id, std::function<void()> cb);
        UINode &onRelease(const std::string &id, std::function<void()> cb);
        UINode &onHoverEnter(const std::string &id, std::function<void()> cb);
        UINode &onHoverExit(const std::string &id, std::function<void()> cb);
        UINode &onValueChanged(const std::string &id, std::function<void(float)> cb);
        UINode &onToggle(const std::string &id, std::function<void(bool)> cb);

        // --- @ref-based chainable helpers (§5.36) ---

        UINode &setTextRef(const std::string &refName, const std::string &text);
        UINode &setButtonTextRef(const std::string &refName, const std::string &text);
        UINode &addClassRef(const std::string &refName, const std::string &cls);
        UINode &removeClassRef(const std::string &refName, const std::string &cls);
        UINode &toggleClassRef(const std::string &refName, const std::string &cls, bool active);
        UINode &setEnabledRef(const std::string &refName, bool enabled);
        UINode &setInteractiveRef(const std::string &refName, bool interactive);
        UINode &setVisibleRef(const std::string &refName, bool visible);
        UINode &onClickRef(const std::string &refName, std::function<void()> cb);
        UINode &onPressRef(const std::string &refName, std::function<void()> cb);
        UINode &onReleaseRef(const std::string &refName, std::function<void()> cb);
        UINode &onHoverEnterRef(const std::string &refName, std::function<void()> cb);
        UINode &onHoverExitRef(const std::string &refName, std::function<void()> cb);
        UINode &onTextChangedRef(const std::string &refName, std::function<void(const std::string &)> cb);
        UINode &onValueChangedRef(const std::string &refName, std::function<void(float)> cb);
        UINode &onToggleRef(const std::string &refName, std::function<void(bool)> cb);

        // --- Runtime structural If & Each (§5.37) ---

        UINode &setCondition(const std::string &condRef, bool value);
        UINode &bindCondition(const std::string &condRef,
                              Bindable<bool> &source,
                              bool fireImmediately = true);

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
                binding.spawnedNodes.push_back(itemRoot);
                ++idx;
            }
            return *this;
        }

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

            attachToRoot_();

            std::weak_ptr<Layout> weakRoot = root_;
            std::weak_ptr<BuildContext> weakCtx = ctx_;
            auto sub = source.onChanged(
                [weakRoot, weakCtx, eachRef, binder = std::move(binder)](const std::vector<T> &vec) mutable
                {
                    auto r = weakRoot.lock();
                    auto c = weakCtx.lock();
                    if (!r || !c)
                        return;
                    UINode ui(std::move(r), std::move(c));
                    ui.syncEach(eachRef, vec, binder);
                },
                fireImmediately);

            ctx_->eachSubs[eachRef] =
                std::make_shared<typename Bindable<std::vector<T>>::Subscription>(std::move(sub));
            return *this;
        }

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

            auto *store = dynamic_cast<KeyedStore<KeyT> *>(binding.keyedStore.get());
            if (!store)
            {
                auto newStore = std::make_shared<KeyedStore<KeyT>>();
                store = newStore.get();
                binding.keyedStore = std::move(newStore);
            }

            std::unordered_map<KeyT, typename KeyedStore<KeyT>::Entry> nextEntries;
            nextEntries.reserve(store->entries.size() + 4);
            std::vector<std::shared_ptr<Layout>> nextNodes;

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
                std::shared_ptr<Layout> existingRoot =
                    (existing != store->entries.end()) ? existing->second.root.lock() : nullptr;

                if (existingRoot)
                {
                    auto entry = std::move(existing->second);
                    UINode itemUi(existingRoot, entry.ctx);
                    invokeEachCallback_(onUpdate, itemUi, elem, idx);
                    if (!effVis)
                        existingRoot->setVisible(false);

                    nextNodes.push_back(std::move(existingRoot));
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

                    nextEntries.emplace(std::move(key),
                                        typename KeyedStore<KeyT>::Entry{itemRoot, itemUi.context()});
                    nextNodes.push_back(std::move(itemRoot));
                }
                ++idx;
            }

            bool sameOrder = (nextNodes.size() == binding.spawnedNodes.size());
            if (sameOrder)
            {
                for (size_t i = 0; i < nextNodes.size(); ++i)
                {
                    if (nextNodes[i].get() != binding.spawnedNodes[i].lock().get())
                    {
                        sameOrder = false;
                        break;
                    }
                }
            }

            if (!sameOrder)
            {
                for (auto &oldWp : binding.spawnedNodes)
                {
                    if (auto oldNode = oldWp.lock())
                        parent->removeChild(oldNode.get());
                }
                for (auto &newNode : nextNodes)
                {
                    parent->insertChildBefore(anchor.get(), newNode);
                }
            }

            store->entries = std::move(nextEntries);
            binding.spawnedNodes.clear();
            binding.spawnedNodes.reserve(nextNodes.size());
            for (const auto &n : nextNodes)
                binding.spawnedNodes.push_back(n);
            binding.showingFallback = false;
            return *this;
        }

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

            attachToRoot_();

            std::weak_ptr<Layout> weakRoot = root_;
            std::weak_ptr<BuildContext> weakCtx = ctx_;
            auto sub = source.onChanged(
                [weakRoot, weakCtx, eachRef,
                 keyFn = std::move(keyFn),
                 onCreate = std::move(onCreate),
                 onUpdate = std::move(onUpdate)](const std::vector<T> &vec) mutable
                {
                    auto r = weakRoot.lock();
                    auto c = weakCtx.lock();
                    if (!r || !c)
                        return;
                    UINode ui(std::move(r), std::move(c));
                    ui.syncEachKeyed(eachRef, vec, keyFn, onCreate, onUpdate);
                },
                fireImmediately);

            ctx_->eachSubs[eachRef] =
                std::make_shared<typename Bindable<std::vector<T>>::Subscription>(std::move(sub));
            return *this;
        }

    private:
        void attachToRoot_() const;
        void warnOnce(const std::string &key, const std::string &msg) const;

        static void propagateCondition_(BuildContext &ctx, const std::string &condName, bool parentEffective);
        static void clearEachNodes_(EachBinding &binding, Layout &parent);
        static void mountEachFallback_(EachBinding &binding, Layout &parent, const Layout *anchor, bool effVis);

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

    /**
     * \class WeakUINode
     * \brief Non-owning handle to a `UINode`, safe to capture inside child callbacks.
     */
    class WeakUINode
    {
    public:
        WeakUINode() = default;
        WeakUINode(std::weak_ptr<Layout> root, std::weak_ptr<BuildContext> ctx)
            : root_(std::move(root)), ctx_(std::move(ctx)) {}

        std::optional<UINode> lock() const;
        bool expired() const;

    private:
        std::weak_ptr<Layout> root_;
        std::weak_ptr<BuildContext> ctx_;
    };

    // --- Free-function entry points ---

    void registerPrefab(const std::string &tagName, const std::string &markupSource);
    bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path);
    bool hasPrefab(const std::string &tagName);

    UINode build(std::string_view source);
    UINode loadMarkupFile(const std::filesystem::path &path);
    UINode instantiate(
        const std::string &tagName,
        const std::vector<std::string> &extraClasses = {},
        const std::string &id = "",
        const std::string &ref = "");
        /** @} */
} // namespace ZenitUI::ZMarkup