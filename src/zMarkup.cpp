#include "ZMarkup.hpp"
#include "ZenitUIComponents.hpp"
#include "StyleAttr.hpp"

#include <fstream>
#include <sstream>

namespace ZenitUI::ZMarkup
{

    // ---- Element ----

    std::string Element::attr(std::string_view k, std::string_view d) const
    {
        for (const auto &kv : attrs)
            if (kv.first == k)
                return kv.second;
        return std::string(d);
    }

    bool Element::has(std::string_view k) const
    {
        for (const auto &kv : attrs)
            if (kv.first == k)
                return true;
        return false;
    }

    float Element::attrFloat(std::string_view k, float d) const
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

    bool Element::attrBool(std::string_view k, bool d) const
    {
        auto v = attr(k);
        if (v.empty())
            return d;
        return v == "true" || v == "1" || v == "yes";
    }

    // ---- Parser ----

    bool Parser::isIdentStart(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool Parser::isIdentChar(char c)
    {
        return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
    }

    void Parser::skipWs()
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

    void Parser::skipInlineWs()
    {
        while (!eof() && (s[p] == ' ' || s[p] == '\t'))
            ++p;
    }

    std::string Parser::readIdent()
    {
        std::string out;
        while (!eof() && isIdentChar(s[p]))
            out.push_back(s[p++]);
        return out;
    }

    std::string Parser::readQuoted()
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

    std::string Parser::readBareToken()
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

    Element Parser::parseElement()
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

    Element Parser::parseRoot()
    {
        skipWs();
        return parseElement();
    }

    Element parse(std::string_view src)
    {
        Parser p(src);
        return p.parseRoot();
    }

    // ---- BuildContext ----

    bool BuildContext::isConditionVisible(const std::string &condName) const
    {
        if (condName.empty())
            return true;
        auto it = conditions.find(condName);
        return it != conditions.end() ? it->second.effectiveVisible : true;
    }

    // ---- Registry ----

    void Registry::reg(const std::string &tag, Factory f)
    {
        factories[tag] = std::move(f);
    }

    bool Registry::hasPrefab(const std::string &tagName) const
    {
        return prefabs.count(tagName) > 0;
    }

    void Registry::registerPrefab(const std::string &tagName, const std::string &markupSource)
    {
        Element root = parse(markupSource);
        prefabs[tagName] = std::move(root);
    }

    bool Registry::registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
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

    std::vector<std::shared_ptr<Layout>> Registry::create_impl(
        const Element &el, BuildContext &ctx, bool isPrefabRoot) const
    {
        auto created = create_nodes_impl(el, ctx, isPrefabRoot);
        std::vector<std::shared_ptr<Layout>> out;
        out.reserve(created.size());
        for (auto &cn : created)
            out.push_back(std::move(cn.node));
        return out;
    }

    std::shared_ptr<Layout> Registry::create(const Element &el, BuildContext &ctx) const
    {
        auto res = create_impl(el, ctx);
        return res.empty() ? nullptr : res.front();
    }

    std::vector<Registry::CreatedNode> Registry::create_nodes_impl(
        const Element &el, BuildContext &ctx, bool /*isPrefabRoot*/) const
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

    std::vector<Registry::CreatedNode> Registry::expandPrefab(
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

    UINode Registry::instantiate(
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

    // ---- Built-in registry ----

    void registerBuiltins(Registry &r)
    {
        using namespace ZenitUI::UI;

        r.reg("Stack", [](const Element &)
              {
                  auto l = std::make_shared<Layout>(LayoutType::Stack);
                  l->setStyleTag("Stack");
                  return l;
              });
        r.reg("Spacer", [](const Element &)
              {
                  auto l = std::make_shared<Layout>(LayoutType::Stack);
                  l->setStyleTag("Spacer");
                  return l;
              });
        r.reg("VStack", [](const Element &)
              { return VStack(); });
        r.reg("HStack", [](const Element &)
              { return HStack(); });
        r.reg("Box", [](const Element &)
              { return Box(); });

        r.reg("Text", [](const Element &e)
              {
                  auto t = Text::create(e.text);
                  if (e.has("wrap"))
                      t->setWrap(e.attrBool("wrap", true));
                  return t;
              });
        r.reg("Label", [](const Element &e)
              {
                  auto t = Text::create(e.text);
                  if (e.has("wrap"))
                      t->setWrap(e.attrBool("wrap", true));
                  return t;
              });

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
                  auto flush = [&]()
                  {
                      size_t a = cur.find_first_not_of(" \t");
                      size_t b = cur.find_last_not_of(" \t");
                      if (a != std::string::npos)
                          opts.push_back(cur.substr(a, b - a + 1));
                      cur.clear();
                  };
                  for (char c : e.attr("options", ""))
                  {
                      if (c == ',')
                          flush();
                      else
                          cur.push_back(c);
                  }
                  flush();
                  if (opts.empty())
                      opts.push_back("Default");
                  int sel = std::clamp((int)e.attrFloat("selected", 0.0f), 0, (int)opts.size() - 1);
                  return Dropdown::create(std::move(opts), sel);
              });
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

    Registry &getGlobalRegistry()
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

    // ---- UINode ----

    WeakUINode UINode::weak() const
    {
        attachToRoot_();
        return WeakUINode(root_, ctx_);
    }

    std::shared_ptr<Layout> UINode::find(const std::string &id) const
    {
        auto it = ctx_->byId.find(id);
        if (it == ctx_->byId.end())
        {
            warnOnce("find:" + id,
                     "UINode::find('" + id + "'): no node with this id");
            return nullptr;
        }
        return it->second.lock();
    }

    UINode &UINode::onClick(const std::string &id, std::function<void()> cb)
    {
        if (auto n = find(id))
            n->onClick = std::move(cb);
        return *this;
    }

    UINode &UINode::onPress(const std::string &id, std::function<void()> cb)
    {
        if (auto n = find(id))
            n->onPress = std::move(cb);
        return *this;
    }

    UINode &UINode::onRelease(const std::string &id, std::function<void()> cb)
    {
        if (auto n = find(id))
            n->onRelease = std::move(cb);
        return *this;
    }

    UINode &UINode::onHoverEnter(const std::string &id, std::function<void()> cb)
    {
        if (auto n = find(id))
            n->onHoverEnter = std::move(cb);
        return *this;
    }

    UINode &UINode::onHoverExit(const std::string &id, std::function<void()> cb)
    {
        if (auto n = find(id))
            n->onHoverExit = std::move(cb);
        return *this;
    }

    UINode &UINode::onValueChanged(const std::string &id, std::function<void(float)> cb)
    {
        if (auto s = std::dynamic_pointer_cast<UI::Slider>(find(id)))
            s->onValueChanged = std::move(cb);
        return *this;
    }

    UINode &UINode::onToggle(const std::string &id, std::function<void(bool)> cb)
    {
        if (auto t = std::dynamic_pointer_cast<UI::Toggle>(find(id)))
            t->onToggle = std::move(cb);
        return *this;
    }

    UINode &UINode::setTextRef(const std::string &refName, const std::string &text)
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

    UINode &UINode::setButtonTextRef(const std::string &refName, const std::string &text)
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

    UINode &UINode::addClassRef(const std::string &refName, const std::string &cls)
    {
        if (auto n = ref<Layout>(refName))
            n->addClass(cls);
        return *this;
    }

    UINode &UINode::removeClassRef(const std::string &refName, const std::string &cls)
    {
        if (auto n = ref<Layout>(refName))
            n->removeClass(cls);
        return *this;
    }

    UINode &UINode::toggleClassRef(const std::string &refName, const std::string &cls, bool active)
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

    UINode &UINode::setEnabledRef(const std::string &refName, bool enabled)
    {
        if (auto n = ref<Layout>(refName))
            n->setEnabled(enabled);
        return *this;
    }

    UINode &UINode::setInteractiveRef(const std::string &refName, bool interactive)
    {
        if (auto n = ref<Layout>(refName))
            n->setInteractive(interactive);
        return *this;
    }

    UINode &UINode::setVisibleRef(const std::string &refName, bool visible)
    {
        if (auto n = ref<Layout>(refName))
            n->setVisible(visible);
        return *this;
    }

    UINode &UINode::onClickRef(const std::string &refName, std::function<void()> cb)
    {
        if (auto n = ref<Layout>(refName))
            n->onClick = std::move(cb);
        return *this;
    }

    UINode &UINode::onPressRef(const std::string &refName, std::function<void()> cb)
    {
        if (auto n = ref<Layout>(refName))
            n->onPress = std::move(cb);
        return *this;
    }

    UINode &UINode::onReleaseRef(const std::string &refName, std::function<void()> cb)
    {
        if (auto n = ref<Layout>(refName))
            n->onRelease = std::move(cb);
        return *this;
    }

    UINode &UINode::onHoverEnterRef(const std::string &refName, std::function<void()> cb)
    {
        if (auto n = ref<Layout>(refName))
            n->onHoverEnter = std::move(cb);
        return *this;
    }

    UINode &UINode::onHoverExitRef(const std::string &refName, std::function<void()> cb)
    {
        if (auto n = ref<Layout>(refName))
            n->onHoverExit = std::move(cb);
        return *this;
    }

    UINode &UINode::onTextChangedRef(const std::string &refName,
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

    UINode &UINode::onValueChangedRef(const std::string &refName, std::function<void(float)> cb)
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

    UINode &UINode::onToggleRef(const std::string &refName, std::function<void(bool)> cb)
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

    UINode &UINode::setCondition(const std::string &condRef, bool value)
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

    UINode &UINode::bindCondition(const std::string &condRef,
                                  Bindable<bool> &source,
                                  bool fireImmediately)
    {
        if (ctx_->conditions.find(condRef) == ctx_->conditions.end())
        {
            warnOnce("cond:" + condRef,
                     "UINode::bindCondition('" + condRef + "'): no If with this ref");
            return *this;
        }

        attachToRoot_();

        std::weak_ptr<Layout> weakRoot = root_;
        std::weak_ptr<BuildContext> weakCtx = ctx_;
        auto sub = source.onChanged(
            [weakRoot, weakCtx, condRef](const bool &v)
            {
                auto r = weakRoot.lock();
                auto c = weakCtx.lock();
                if (!r || !c)
                    return;
                UINode ui(std::move(r), std::move(c));
                ui.setCondition(condRef, v);
            },
            fireImmediately);

        ctx_->conditionSubs[condRef] =
            std::make_shared<typename Bindable<bool>::Subscription>(std::move(sub));
        return *this;
    }

    void UINode::attachToRoot_() const
    {
        if (root_ && ctx_ && !ctx_->attachedToRoot)
        {
            ctx_->attachedToRoot = true;
            root_->retainObject(ctx_);
        }
    }

    void UINode::warnOnce(const std::string &key, const std::string &msg) const
    {
        if (warnedKeys_.insert(key).second)
            logWarn("ZMarkup", "", 0, 0, msg);
    }

    void UINode::propagateCondition_(BuildContext &ctx, const std::string &condName, bool parentEffective)
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
            for (auto &wp : eit->second.spawnedNodes)
            {
                if (auto n = wp.lock())
                    n->setVisible(eff);
            }
        }
        for (const auto &childCond : b.childConditions)
        {
            propagateCondition_(ctx, childCond, eff);
        }
    }

    void UINode::clearEachNodes_(EachBinding &binding, Layout &parent)
    {
        for (auto &wp : binding.spawnedNodes)
        {
            if (auto n = wp.lock())
                parent.removeChild(n.get());
        }
        binding.spawnedNodes.clear();
        binding.showingFallback = false;
    }

    void UINode::mountEachFallback_(EachBinding &binding, Layout &parent, const Layout *anchor, bool effVis)
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
                binding.spawnedNodes.push_back(n);
            }
        }
    }

    // ---- WeakUINode ----

    std::optional<UINode> WeakUINode::lock() const
    {
        auto r = root_.lock();
        auto c = ctx_.lock();
        if (!r || !c)
            return std::nullopt;
        return UINode(std::move(r), std::move(c));
    }

    bool WeakUINode::expired() const
    {
        return root_.expired() || ctx_.expired();
    }

    // ---- Free functions ----

    void registerPrefab(const std::string &tagName, const std::string &markupSource)
    {
        getGlobalRegistry().registerPrefab(tagName, markupSource);
    }

    bool registerPrefabFile(const std::string &tagName, const std::filesystem::path &path)
    {
        return getGlobalRegistry().registerPrefabFile(tagName, path);
    }

    bool hasPrefab(const std::string &tagName)
    {
        return getGlobalRegistry().hasPrefab(tagName);
    }

    UINode build(std::string_view source)
    {
        auto ctx = std::make_shared<BuildContext>();
        auto el = parse(source);
        auto root = getGlobalRegistry().create(el, *ctx);
        return UINode(root, ctx);
    }

    UINode loadMarkupFile(const std::filesystem::path &path)
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

    UINode instantiate(
        const std::string &tagName,
        const std::vector<std::string> &extraClasses,
        const std::string &id,
        const std::string &ref)
    {
        return getGlobalRegistry().instantiate(tagName, extraClasses, id, ref);
    }

} // namespace ZenitUI::ZMarkup