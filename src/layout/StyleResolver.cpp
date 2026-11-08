#include "Layout.hpp"
#include "Debug.hpp"
#include "StyleResolver.hpp"

#define ZENIT_COUNT_FIELD(T, name, def) +1
static_assert(0 BUBBLE_STYLE_PROPS(ZENIT_COUNT_FIELD) <= 64,
			  "transitionDirtyMask_ is a uint64_t: add a second field"
			  "or upgrade to unsigned __int128");
#undef ZENIT_COUNT_FIELD

namespace ZenitUI
{

	ComputedStyle StyleResolver::resolveFor(Layout &node)
	{
		auto &theme = Theme::get();

		const auto parent = node.getParent();
		const bool cacheable =
			inlineBase.customProps.empty() &&
			inlineBase.unresolvedProps.empty() &&
			(!parent || parent->getStyle().customProps.empty());

		uint64_t key = 0;
		if (cacheable)
		{
			auto mix = [&key](uint64_t v)
			{
				key ^= v;
				key *= 1099511628211ULL;
			};

			mix(theme.version);
			mix((uint64_t)UIContext::get().viewportGeneration);
			mix((uint64_t)reinterpret_cast<uintptr_t>(&node));
			mix(node.getEnabled() ? 1u : 0u);
			mix((uint32_t)node.isHoveredState());
			mix((uint32_t)node.isPressedState());
			mix((uint32_t)node.isFocusedState());
			mix((uint32_t)node.isCheckedState());

			for (char c : node.getStyleTag())
				mix((uint64_t)(unsigned char)c);

			for (auto &c : node.getStyleClasses())
			{
				for (char ch : c)
					mix((uint64_t)(unsigned char)ch);
				mix(0xFF);
			}

			for (auto p = node.getParent(); p; p = p->getParent())
			{
				mix(p->getEnabled() ? 1u : 0u);
				mix((uint32_t)p->isHoveredState());
				mix((uint32_t)p->isPressedState());
				mix((uint32_t)p->isFocusedState());
				mix((uint32_t)p->isCheckedState());
			}

			if (cachedValid_ && cachedKey_ == key)
			{
				ComputedStyle result = cachedThemeStyle_;

				if (!node.getEnabled())
					overlayComputed(result, inlineDisabled);
				else
				{
					if (node.isHoveredState() || node.isPressedState())
						overlayComputed(result, inlineHover);
					if (node.isPressedState())
						overlayComputed(result, inlinePressed);
					if (node.isFocusedState())
						overlayComputed(result, inlineFocus);
					if (node.isCheckedState())
						overlayComputed(result, inlineChecked);
				}
				overlayComputed(result, inlineBase);

				if (inlineBase.transitions.is_set)
					result.transitions = inlineBase.transitions.value;
				if (inlineBase.animations.is_set)
					result.animations = inlineBase.animations.value;

				deriveAlignAliases(node, inlineBase,
								   cachedAlignItemsSet_,
								   cachedItemsHSet_,
								   cachedItemsVSet_,
								   cachedThemeStyle_.itemsH,
								   cachedThemeStyle_.itemsV,
								   result);

				return result;
			}
		}

		struct Match
		{
			const ThemeRule *rule;
		};
		std::vector<Match> matches;

		{
			const std::string &tag = node.getStyleTag();

			auto it = theme.rulesByTag.find(tag);
			if (it != theme.rulesByTag.end())
			{
				for (size_t idx : it->second)
				{
					const auto &r = theme.rules[idx];
					if (ruleMatches(r, &node))
						matches.push_back({&r});
				}
			}

			for (size_t idx : theme.rulesNoTag)
			{
				const auto &r = theme.rules[idx];
				if (ruleMatches(r, &node))
					matches.push_back({&r});
			}
		}

		std::stable_sort(matches.begin(), matches.end(),
						 [](const Match &a, const Match &b)
						 {
							 if (a.rule->specificity != b.rule->specificity)
								 return a.rule->specificity < b.rule->specificity;
							 return a.rule->order < b.rule->order;
						 });

		Style finalStyle;
		finalStyle.overlay(inlineDefaults);
		for (const auto &m : matches)
			finalStyle.overlay(m.rule->style);

		std::unordered_map<std::string, std::string> allCustomProps;
		if (auto p = node.getParent())
			allCustomProps = p->getStyle().customProps;
		else
			allCustomProps = theme.root.customProps;

		for (const auto &[k, v] : finalStyle.customProps)
			allCustomProps[k] = v;

		if (!finalStyle.unresolvedProps.empty())
		{
			Style resolved = resolveUnresolvedProps(finalStyle.unresolvedProps, allCustomProps);
			finalStyle.overlay(resolved);
			finalStyle.unresolvedProps.clear();
		}

		const ComputedStyle *parentStyle = nullptr;
		if (auto p = node.getParent())
			parentStyle = &p->getStyle();

		ComputedStyle result = ComputedStyle::from(finalStyle, parentStyle, &theme.root);
		result.customProps = std::move(allCustomProps);

		if (cacheable)
		{
			cachedAlignItemsSet_ = finalStyle.alignItems.is_set;
			cachedItemsHSet_ = finalStyle.itemsH.is_set;
			cachedItemsVSet_ = finalStyle.itemsV.is_set;
			cachedThemeStyle_ = result;
			cachedKey_ = key;
			cachedValid_ = true;
		}
		else
		{
			cachedValid_ = false;
		}

		if (!node.getEnabled())
			overlayComputed(result, inlineDisabled);
		else
		{
			if (node.isHoveredState() || node.isPressedState())
				overlayComputed(result, inlineHover);
			if (node.isPressedState())
				overlayComputed(result, inlinePressed);
			if (node.isFocusedState())
				overlayComputed(result, inlineFocus);
			if (node.isCheckedState())
				overlayComputed(result, inlineChecked);
		}
		overlayComputed(result, inlineBase);
		if (inlineBase.transitions.is_set)
			result.transitions = inlineBase.transitions.value;
		if (inlineBase.animations.is_set)
			result.animations = inlineBase.animations.value;

		deriveAlignAliases(node, inlineBase,
						   finalStyle.alignItems.is_set,
						   finalStyle.itemsH.is_set,
						   finalStyle.itemsV.is_set,
						   finalStyle.itemsH.get_or(Align::Start),
						   finalStyle.itemsV.get_or(Align::Start),
						   result);

		node.relevantSymbols_.clear();

		auto collectFrom = [&](SimpleSelector::Kind kind, const std::string &name)
		{
			auto it = theme.rulesByLastSymbol.find(
				Theme::SymbolKey{kind, name});
			if (it == theme.rulesByLastSymbol.end())
				return;
			for (uint32_t idx : it->second)
			{
				const auto &r = theme.rules[idx];
				for (const auto &sel : r.chain)
				{
					node.relevantSymbols_.push_back({sel.kind, sel.name});
					for (const auto &[k, n] : sel.extras)
						node.relevantSymbols_.push_back({k, n});
				}
			}
		};

		collectFrom(SimpleSelector::Kind::Tag, node.getStyleTag());
		for (const auto &cls : node.getStyleClasses())
			collectFrom(SimpleSelector::Kind::Class, cls);
		if (!node.nodeId.empty())
			collectFrom(SimpleSelector::Kind::Id, node.nodeId);

		node.relevantThemeVersion_ = theme.version;

		return result;
	}

	Style StyleResolver::partFor(Layout &node, const std::string &partName)
	{
		auto &theme = Theme::get();

		std::vector<const ThemeRule *> matches;
		for (const auto &r : theme.rules)
		{
			if (r.part != partName)
				continue;
			if (!ruleMatches(r, &node))
				continue;
			matches.push_back(&r);
		}

		std::stable_sort(matches.begin(), matches.end(),
						 [](const ThemeRule *a, const ThemeRule *b)
						 {
							 if (a->specificity != b->specificity)
								 return a->specificity < b->specificity;
							 return a->order < b->order;
						 });

		Style target;
		for (const auto *r : matches)
			target.overlay(r->style);

		double now = UIContext::get().time;
		auto &st = partTransitions[partName];

		if (!st.initialized)
		{
			st.target = target;
			st.current = target;
			st.active = false;
			st.initialized = true;
		}
		else if (stylesDiffer(target, st.target))
		{
			float tp = 1.0f;
			if (st.active && st.duration > 0.001f)
				tp = (float)std::clamp((now - st.startTime) / st.duration, 0.0, 1.0);

			st.current = lerpStyleParts(st.current, st.target, getRatio(tp, st.ease));

			st.target = target;
			st.startTime = now;

			float maxDur = 0.0f;
			TransitionFunction ease = TransitionFunction::Linear;
			if (target.transitions.is_set)
			{
				for (const auto &spec : target.transitions.value)
				{
					if (spec.duration > maxDur)
					{
						maxDur = spec.duration;
						ease = spec.ease;
					}
				}
			}
			st.duration = maxDur;
			st.ease = ease;
			st.active = (maxDur > 0.001f);
		}

		float partT = 1.0f;
		if (st.active)
		{
			partT = (float)std::clamp((now - st.startTime) / st.duration, 0.0, 1.0);
			if (partT >= 1.0f)
			{
				st.active = false;
				partT = 1.0f;
			}
		}

		Style result = lerpStyleParts(st.current, st.target, getRatio(partT, st.ease));

		node.anim_.applyCssToPart(node, partName, result);

		return result;
	}

	void StyleResolver::beginStateTransition(Layout &node, UIState newState)
	{
		currentState = newState;
		transitionTimer = 0.0f;
		transitionStartStyle = currentStyle;
		targetStyle = resolveFor(node);
		node.internal_.pendingTransition = 0;

		transitionDirtyMask_ = 0;
		int i = 0;
#define X(T, name, def)                           \
	if (!(currentStyle.name == targetStyle.name)) \
		transitionDirtyMask_ |= (1ULL << i);      \
	++i;
		BUBBLE_STYLE_PROPS(X)
#undef X
	}

	void StyleResolver::resolvePendingTransition(Layout &node)
	{
		ComputedStyle newTarget = resolveFor(node);
		node.internal_.pendingTransition = 0;

		if (newTarget != targetStyle)
		{
			if (node.anim_.hasActiveImperative())
			{
				currentStyle = newTarget;
				targetStyle = std::move(newTarget);
				transitionStartStyle = currentStyle;
				transitionTimer = 1.0f;
				transitionDirtyMask_ = 0;
			}
			else
			{
				transitionDirtyMask_ = 0;
				int i = 0;
#define X(T, name, def)                         \
	if (!(currentStyle.name == newTarget.name)) \
		transitionDirtyMask_ |= (1ULL << i);    \
	++i;
				BUBBLE_STYLE_PROPS(X)
#undef X
				transitionStartStyle = currentStyle;
				transitionTimer = 0.0f;
				targetStyle = std::move(newTarget);
			}
		}
		else
		{
			targetStyle = std::move(newTarget);
			if (transitionTimer >= 1.0f)
			{
				currentStyle = targetStyle;
				transitionStartStyle = currentStyle;
				transitionDirtyMask_ = 0;
			}
		}
	}

	void StyleResolver::tick(Layout & /*node*/, float dt)
	{
		if (transitionTimer >= 1.0f)
			return;

		float maxDur = targetStyle.transitionTime;
		for (const auto &spec : targetStyle.transitions)
			maxDur = std::max(maxDur, spec.duration);

		if (maxDur <= 0.001f)
		{
			transitionTimer = 1.0f;
			currentStyle = targetStyle;
			transitionDirtyMask_ = 0;
			return;
		}

		float elapsed = transitionTimer * maxDur;
		transitionTimer += dt / maxDur;
		if (transitionTimer >= 1.0f)
		{
			transitionTimer = 1.0f;
			currentStyle = targetStyle;
			transitionDirtyMask_ = 0;
			return;
		}

		if (transitionDirtyMask_ == 0)
			return;

		auto timingFor = [&](const char *propName) -> std::pair<float, TransitionFunction>
		{
			for (const auto &spec : targetStyle.transitions)
			{
				if (spec.prop == propName || spec.prop == "all")
					return {spec.duration, spec.ease};
			}
			return {targetStyle.transitionTime, targetStyle.ease};
		};

		int i = 0;
#define X(T, name, def)                                                           \
	if (transitionDirtyMask_ & (1ULL << i))                                       \
	{                                                                             \
		auto [dur, ease] = timingFor(#name);                                      \
		float tp = (dur > 0.0f) ? std::clamp(elapsed / dur, 0.0f, 1.0f) : 1.0f;   \
		currentStyle.name = lerpProp(transitionStartStyle.name, targetStyle.name, \
									 getRatio(tp, ease));                         \
	}                                                                             \
	++i;
		BUBBLE_STYLE_PROPS(X)
#undef X
	}

	void StyleResolver::propagateInheritance(Layout &node)
	{
		bool changed =
			lastInherited.hovered != node.isHoveredState() ||
			lastInherited.pressed != node.isPressedState() ||
			lastInherited.focused != node.isFocusedState() ||
			lastInherited.enabled != node.getEnabled() ||
			lastInherited.checked != node.isCheckedState() ||
			lastInherited.font != currentStyle.font ||
			!(lastInherited.fontSize == currentStyle.fontSize) ||
			!(lastInherited.color == currentStyle.color) ||
			!(lastInherited.letterSpacing == currentStyle.letterSpacing) ||
			!(lastInherited.textAlign == currentStyle.textAlign);

		if (!changed)
			return;

		for (auto &c : node.children)
			c->markInheritanceDirty();

		lastInherited.hovered = node.isHoveredState();
		lastInherited.pressed = node.isPressedState();
		lastInherited.focused = node.isFocusedState();
		lastInherited.enabled = node.getEnabled();
		lastInherited.checked = node.isCheckedState();
		lastInherited.font = currentStyle.font;
		lastInherited.fontSize = currentStyle.fontSize;
		lastInherited.color = currentStyle.color;
		lastInherited.letterSpacing = currentStyle.letterSpacing;
		lastInherited.textAlign = currentStyle.textAlign;
		lastInherited.overflowX = currentStyle.overflowX;
		lastInherited.overflowY = currentStyle.overflowY;
	}

	void StyleResolver::deriveAlignAliases(Layout &node,
										   const Style &inlineBase,
										   bool alignItemsSet, bool itemsHSet, bool itemsVSet,
										   Align styleItemsH, Align styleItemsV,
										   ComputedStyle &result)
	{
		const auto parent = node.getParent();
		const LayoutType selfType = node.getLayoutType();
		const LayoutType parentType = parent ? parent->getLayoutType()
											 : LayoutType::Stack;

		const bool hasExplicitItemsH = itemsHSet || inlineBase.itemsH.is_set;
		const bool hasExplicitItemsV = itemsVSet || inlineBase.itemsV.is_set;
		const Align itemsHVal = inlineBase.itemsH.is_set ? inlineBase.itemsH.value : styleItemsH;
		const Align itemsVVal = inlineBase.itemsV.is_set ? inlineBase.itemsV.value : styleItemsV;

		// Priority 1: explicit align-items on this node.
		// Non-CSS behavior kept for compatibility: also acts as an align-self
		// hint when the node is a child of a VStack/HStack.
		if (alignItemsSet || inlineBase.alignItems.is_set)
		{
			if (parent)
			{
				if (parentType == LayoutType::Vertical && !inlineBase.alignH.is_set && result.alignH == Align::Auto)
				{
					result.alignH = result.alignItems;
				}
				else if (parentType == LayoutType::Horizontal && !inlineBase.alignV.is_set && result.alignV == Align::Auto)
				{
					result.alignV = result.alignItems;
				}
			}
			return;
		}

		// Priority 2a: parent-level deprecated alias.
		// The node itself is a VStack/HStack/Stack that used itemsH/itemsV to
		// control how its own children align along the cross-axis.
		if (hasExplicitItemsH || hasExplicitItemsV)
		{
			if (selfType == LayoutType::Vertical)
			{
				if (hasExplicitItemsH)
					result.alignItems = itemsHVal;
			}
			else if (selfType == LayoutType::Horizontal)
			{
				if (hasExplicitItemsV)
					result.alignItems = itemsVVal;
			}
			else if (selfType == LayoutType::Stack)
			{
				if (hasExplicitItemsH)
					result.alignItems = itemsHVal;
				else if (hasExplicitItemsV)
					result.alignItems = itemsVVal;
			}
		}

		// Priority 2b: child-level deprecated alias (align-self hint).
		// The node's parent is a VStack/HStack and itemsH/itemsV on the node
		// indicate where the user wanted the node itself to align along the
		// parent's cross-axis. This matches the tests written for §5.25 and is
		// kept for compatibility with pre-rename code that set itemsH/itemsV
		// on a leaf child.
		if (parent)
		{
			if (parentType == LayoutType::Vertical && hasExplicitItemsH && !inlineBase.alignH.is_set && result.alignH == Align::Auto)
			{
				result.alignH = itemsHVal;
			}
			else if (parentType == LayoutType::Horizontal && hasExplicitItemsV && !inlineBase.alignV.is_set && result.alignV == Align::Auto)
			{
				result.alignV = itemsVVal;
			}
		}
	}
} // namespace ZenitUI