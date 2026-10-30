#pragma once

#include "Common.hpp"
#include "Logger.hpp"
#include "CoreTypes.hpp"
#include "Easing.hpp"
#include "Unit.hpp"
#include "Style.hpp"
#include "Layout.hpp"
#include "UIComponents.hpp"
#include "StyleAttr.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

namespace ZenitUI::ZMarkup
{

    // =========================================================================
    //  AST
    // =========================================================================
    struct Element
    {
        std::string tag;
        std::vector<std::string> classes;
        std::string id;
        std::string ref;
        std::string text;
        std::vector<std::pair<std::string, std::string>> attrs;
        std::vector<Element> children;

        std::string attr(std::string_view k, std::string_view d = "") const
        {
            for (auto &[key, val] : attrs)
                if (key == k)
                    return val;
            return std::string(d);
        }
        bool has(std::string_view k) const
        {
            for (auto &[key, _] : attrs)
                if (key == k)
                    return true;
            return false;
        }
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
        bool attrBool(std::string_view k, bool d = false) const
        {
            auto v = attr(k);
            if (v.empty())
                return d;
            return v == "true" || v == "1" || v == "yes";
        }
    };

    // =========================================================================
    //  Parser
    // =========================================================================
    class Parser
    {
    public:
        explicit Parser(std::string_view src) : s(src) {}

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

    inline Element parse(std::string_view src)
    {
        Parser p(src);
        return p.parseRoot();
    }

    // =========================================================================
    //  BuildContext
    // =========================================================================
    struct BuildContext
    {
        std::unordered_map<std::string, std::shared_ptr<Layout>> byId;
        std::unordered_map<std::string, std::vector<std::shared_ptr<Layout>>> byRef;

        const std::vector<Element> *currentSlotChildren{nullptr};
    };

    // =========================================================================
    //  Registry
    // =========================================================================
    class Registry
    {
    public:
        using Factory = std::function<std::shared_ptr<Layout>(const Element &)>;

        void reg(const std::string &tag, Factory f) { factories[tag] = std::move(f); }

        void registerPrefab(const std::string &tagName, const std::string &markupSource)
        {
            Element root = parse(markupSource);
            prefabs[tagName] = std::move(root);
        }

        bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f)
            {
                logError("ZMarkup", path.string(), 0, 0, "Impossible aprire il file per il prefab");
                return false;
            }
            std::stringstream ss;
            ss << f.rdbuf();
            registerPrefab(tagName, ss.str());
            return true;
        }

        // Il nuovo create_impl ritorna un VETTORE di nodi. Questo risolve il problema dello Slot.
        std::vector<std::shared_ptr<Layout>> create_impl(const Element &el, BuildContext &ctx, bool isPrefabRoot = false)
        {
            std::vector<std::shared_ptr<Layout>> result;

            // 1. È uno Slot? (Ritorna i figli direttamente al padre chiamante!)
            if (el.tag == "Slot")
            {
                const std::vector<Element> *childrenToBuild = nullptr;
                if (ctx.currentSlotChildren && !ctx.currentSlotChildren->empty())
                {
                    childrenToBuild = ctx.currentSlotChildren;
                }
                else if (!el.children.empty())
                {
                    childrenToBuild = &el.children;
                }

                if (childrenToBuild)
                {
                    auto previousSlot = ctx.currentSlotChildren;
                    ctx.currentSlotChildren = nullptr;

                    for (const auto &child : *childrenToBuild)
                    {
                        auto childNodes = create_impl(child, ctx);
                        result.insert(result.end(), childNodes.begin(), childNodes.end());
                    }
                    ctx.currentSlotChildren = previousSlot;
                }
                return result;
            }

            // 2. È un Prefab?
            if (!isPrefabRoot)
            {
                auto prefabIt = prefabs.find(el.tag);
                if (prefabIt != prefabs.end())
                {
                    Element instanceTree = prefabIt->second;

                    for (const auto &c : el.classes)
                        instanceTree.classes.push_back(c);
                    if (!el.id.empty())
                        instanceTree.id = el.id;
                    if (!el.ref.empty())
                        instanceTree.ref = el.ref;
                    for (const auto &attr : el.attrs)
                        instanceTree.attrs.push_back(attr);

                    auto previousSlot = ctx.currentSlotChildren;
                    ctx.currentSlotChildren = &el.children;

                    auto nodes = create_impl(instanceTree, ctx, true);

                    ctx.currentSlotChildren = previousSlot;
                    return nodes; // Propaga la radice istanziata
                }
            }

            // 3. Nodo normale
            auto it = factories.find(el.tag);
            if (it == factories.end())
                return {};

            auto node = it->second(el);
            if (!node)
                return {};

            if (el.has("passthrough"))
                node->setPassThrough(el.attrBool("passthrough"));

            for (auto &c : el.classes)
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

            for (auto &[k, v] : el.attrs)
                applyStyleAttr(node->getInlineBase(), k, v);

            Layout *target = node->contentTarget();

            auto previousSlot = ctx.currentSlotChildren;
            if (!isPrefabRoot)
            {
                ctx.currentSlotChildren = nullptr;
            }

            for (auto &child : el.children)
            {
                auto childNodes = create_impl(child, ctx);
                for (auto &c : childNodes)
                {
                    target->addChild(c);
                }
            }

            ctx.currentSlotChildren = previousSlot;

            result.push_back(node);
            return result;
        }

        std::shared_ptr<Layout> create(const Element &el, BuildContext &ctx)
        {
            auto res = create_impl(el, ctx);
            return res.empty() ? nullptr : res.front();
        }

    private:
        std::unordered_map<std::string, Factory> factories;
        std::unordered_map<std::string, Element> prefabs;
    };

    // =========================================================================
    //  Componenti built-in
    // =========================================================================
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
              { return TextInput::create(e.attr("value", e.text)); });
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
        return Dropdown::create(std::move(opts), 0); });
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

    // =========================================================================
    //  Accesso Globale
    // =========================================================================
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

    // =========================================================================
    //  UINode
    // =========================================================================
    class UINode
    {
    public:
        UINode(std::shared_ptr<Layout> root, std::shared_ptr<BuildContext> ctx)
            : root_(std::move(root)), ctx_(std::move(ctx)) {}

        std::shared_ptr<Layout> root() const { return root_; }
        std::shared_ptr<BuildContext> context() const { return ctx_; }

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

        template <typename T = Layout>
        std::shared_ptr<T> find(const std::string &id) const
        {
            auto n = find(id);
            return n ? std::static_pointer_cast<T>(n) : nullptr;
        }

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
        UINode &onValueChanged(const std::string &id, std::function<void(float)> cb)
        {
            if (auto s = find<UI::Slider>(id))
                s->onValueChanged = std::move(cb);
            return *this;
        }
        UINode &onToggle(const std::string &id, std::function<void(bool)> cb)
        {
            if (auto t = find<UI::Toggle>(id))
                t->onToggle = std::move(cb);
            return *this;
        }

    private:
        void warnOnce(const std::string &key, const std::string &msg) const
        {
            if (warnedKeys_.insert(key).second)
                logWarn("ZMarkup", "", 0, 0, msg);
        }

        std::shared_ptr<Layout> root_;
        std::shared_ptr<BuildContext> ctx_;
        mutable std::unordered_set<std::string> warnedKeys_;
    };

    // =========================================================================
    //  Entry point
    // =========================================================================

    inline void registerPrefab(const std::string &tagName, const std::string &markupSource)
    {
        getGlobalRegistry().registerPrefab(tagName, markupSource);
    }

    inline bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
    {
        return getGlobalRegistry().registerPrefabFile(tagName, path);
    }

    inline UINode build(std::string_view source)
    {
        auto ctx = std::make_shared<BuildContext>();
        auto el = parse(source);
        auto root = getGlobalRegistry().create(el, *ctx);
        return UINode(root, ctx);
    }

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

} // namespace ZenitUI::ZMarkup