#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "AnimPrimitives.hpp"
#include "Media.hpp"

namespace ZenitUI
{
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

			// Componenti aggiuntivi del compound selector.
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

			// Le pseudo-classi (:hover, :checked, ...) contano come classi.
			if (sel.requireHover || sel.requirePressed || sel.requireFocus ||
				sel.requireDisabled || sel.requireChecked)
				s.classes++;
		}
		return s;
	}

	struct ThemeRule
	{
		std::vector<SimpleSelector> chain;
		std::string part; // vuoto se non è ::part
		Style style;
		Specificity specificity;
		int order{0}; // ordine di dichiarazione (per tie-break)
		std::optional<MediaQuery> media;
	};

	class Theme
	{
	public:
		// Ordered list of all rules. The index in this vector is the
		// rule's `order`. When two rules have the same specificity,
		// the one with the larger `order` wins.
		std::vector<ThemeRule> rules;

		// Style root (`:root { ... }`).
		Style root;

		// Keyframes by name.
		std::unordered_map<std::string, KeyframeAnimation> keyframes;

		// --- Index (built incrementally by addRule) ---------------------
		//
		// `resolveFor` needs to test every rule against a node. Scanning
		// `rules` linearly is O(R) per node, where R is the total rule
		// count. Most of those rules can never match a given node because
		// their rightmost selector is a tag that isn't the node's tag.
		//
		// The index splits rules into two buckets:
		//
		//   - `rulesByTag[tag]`  — rules whose rightmost selector is that
		//                          tag (possibly with classes/ids/extras
		//                          or state flags attached).
		//   - `rulesNoTag`       — rules whose rightmost selector is a
		//                          class or an id. These can match any node
		//                          and must always be tested.
		//
		// `::part` rules are not indexed: they are resolved by `partFor`,
		// not by `resolveFor`.
		//
		// Both containers store indices into `rules`. They must stay in
		// sync with `rules` — `addRule` and `clear` are the only methods
		// that should touch them.
		std::unordered_map<std::string, std::vector<size_t>> rulesByTag;
		std::vector<size_t> rulesNoTag;

		static Theme &get()
		{
			static Theme instance;
			return instance;
		}

		void addRule(ThemeRule r)
		{
			r.specificity = computeSpecificity(r.chain);
			r.order = (int)rules.size();

			// Index the rule. Skip ::part rules.
			if (r.part.empty() && !r.chain.empty())
			{
				const auto &back = r.chain.back();
				if (back.kind == SimpleSelector::Kind::Tag)
					rulesByTag[back.name].push_back((size_t)r.order);
				else
					rulesNoTag.push_back((size_t)r.order);
			}

			rules.push_back(std::move(r));
		}

		void addKeyframes(const KeyframeAnimation &anim)
		{
			keyframes[anim.name] = anim;
		}

		void clear()
		{
			rules.clear();
			keyframes.clear();
			root = Style{};
			rulesByTag.clear();
			rulesNoTag.clear();
		}
	};
}