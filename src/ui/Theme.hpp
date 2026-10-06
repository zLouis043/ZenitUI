#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "AnimPrimitives.hpp"

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

			// Le pseudo-classi (:hover, :checked, ...) contano come classi.
			if (sel.requireHover || sel.requirePressed || sel.requireFocus || sel.requireDisabled || sel.requireChecked)
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
	};

	class Theme
	{
	public:
		std::vector<ThemeRule> rules;
		Style root;
		std::unordered_map<std::string, KeyframeAnimation> keyframes;

		static Theme &get()
		{
			static Theme instance;
			return instance;
		}

		void addRule(ThemeRule r)
		{
			r.specificity = computeSpecificity(r.chain);
			r.order = (int)rules.size();
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
		}
	};
}