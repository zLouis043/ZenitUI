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

	struct ThemeRule
	{
		std::vector<SimpleSelector> chain;
		std::string part;
		Style style;
		Specificity specificity;
		int order{0};
		std::optional<MediaQuery> media;
	};

	class Theme
	{
	public:
		struct SymbolKey
		{
			SimpleSelector::Kind kind;
			std::string name;

			bool operator==(const SymbolKey &o) const noexcept
			{
				return kind == o.kind && name == o.name;
			}
		};

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

		static Theme &get()
		{
			static Theme instance;
			return instance;
		}

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

		void addKeyframes(const KeyframeAnimation &anim)
		{
			keyframes[anim.name] = anim;
			++version;
		}

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