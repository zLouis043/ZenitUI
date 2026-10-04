#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "UIAnimations.hpp"

namespace ZenitUI {

	struct ThemeRule {
		std::vector<SimpleSelector> chain;
		std::string part;              // vuoto se non è ::part
		Style       style;
		int         specificity{ 0 };  // precalcolata
		int         order{ 0 };        // ordine di dichiarazione (per tie-break)
	};

	inline int computeSpecificity(const std::vector<SimpleSelector>& chain) {
		int ids = 0, classes = 0, tags = 0;
		for (const auto& s : chain) {
			if      (s.kind == SimpleSelector::Kind::Id)    ids++;
			else if (s.kind == SimpleSelector::Kind::Class) classes++;
			else                                             tags++;

			// Le pseudo-classi (:hover, :checked, ...) contano come classi.
			if (s.requireHover || s.requirePressed || s.requireFocus
				|| s.requireDisabled || s.requireChecked) classes++;
		}
		return ids * 10000 + classes * 100 + tags;
	}
class Theme {
	public:
		std::vector<ThemeRule> rules;
		Style root;
		std::unordered_map<std::string, KeyframeAnimation> keyframes;

		static Theme& get() {
			static Theme instance;
			return instance;
		}

		void addRule(ThemeRule r) {
			r.specificity = computeSpecificity(r.chain);
			r.order       = (int)rules.size();
			rules.push_back(std::move(r));
		}

		void addKeyframes(const KeyframeAnimation& anim) {
			keyframes[anim.name] = anim;
		}

		void clear() {
			rules.clear();
			keyframes.clear();
			root = Style{};
		}
	};
}