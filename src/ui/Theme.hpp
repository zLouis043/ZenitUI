#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "UIAnimations.hpp"

namespace ZenitUI {

	class Theme {
	public:
	std::unordered_map<std::string, StyleSet> tags; 
		std::unordered_map<std::string, StyleSet> classes;
		std::unordered_map<std::string, StyleSet> ids;
		std::unordered_map<std::string, KeyframeAnimation> keyframes;

		static Theme& get() {
			static Theme instance;
			return instance;
		}

		Theme& add(const std::string& name, const StyleSet& set) {
			classes[name] = set;
			return *this;
		}
		Theme& addTag(const std::string& name, const StyleSet& set) {
			tags[name] = set;
			return *this;
		}
		Theme& addId(const std::string& name, const StyleSet& set) {
			ids[name] = set;
			return *this;
		}
		Theme& addKeyframes(const KeyframeAnimation& anim) {
			keyframes[anim.name] = anim;
			return *this;
		}
		void clear() {
			classes.clear();
			keyframes.clear();
			tags.clear();
			ids.clear();
		}
	};
}