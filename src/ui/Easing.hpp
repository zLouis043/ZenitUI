#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"

namespace ZenitUI {

	enum class TransitionFunction {
		Linear,
		EaseInQuad, EaseOutQuad, EaseInOutQuad,
		EaseInCubic, EaseOutCubic, EaseInOutCubic,
		EaseInBack, EaseOutBack,
		EaseOutElastic, EaseOutBounce
	};

	inline float getRatio(float t, TransitionFunction fn) {
		if (t <= 0.0f) return 0.0f;
		if (t >= 1.0f) return 1.0f;
		constexpr float pi = 3.14159265358979323846f;
		switch (fn) {
		case TransitionFunction::Linear:        return t;
		case TransitionFunction::EaseInQuad:    return t * t;
		case TransitionFunction::EaseOutQuad:   return t * (2.0f - t);
		case TransitionFunction::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
		case TransitionFunction::EaseInCubic:   return t * t * t;
		case TransitionFunction::EaseOutCubic:  return 1.0f - std::pow(1.0f - t, 3.0f);
		case TransitionFunction::EaseInOutCubic:
			return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
		case TransitionFunction::EaseInBack: {
			const float c1 = 1.70158f, c3 = c1 + 1.0f;
			return c3 * t * t * t - c1 * t * t;
		}
		case TransitionFunction::EaseOutBack: {
			const float c1 = 1.70158f, c3 = c1 + 1.0f;
			float f = t - 1.0f;
			return 1.0f + c3 * f * f * f + c1 * f * f;
		}
		case TransitionFunction::EaseOutElastic: {
			const float c4 = (2.0f * pi) / 3.0f;
			return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
		}
		case TransitionFunction::EaseOutBounce: {
			const float n1 = 7.5625f, d1 = 2.75f;
			if (t < 1.0f / d1) return n1 * t * t;
			if (t < 2.0f / d1) { t -= 1.5f / d1;   return n1 * t * t + 0.75f; }
			if (t < 2.5f / d1) { t -= 2.25f / d1;  return n1 * t * t + 0.9375f; }
			t -= 2.625f / d1;                      return n1 * t * t + 0.984375f;
		}
		}
		return t;
	}

	inline bool parseEasing(std::string_view name, TransitionFunction& out) {
		std::string n;
		for (char c : name) {
			if (c == '-' || c == '_' || c == ' ') continue;
			n.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		struct Entry { const char* key; TransitionFunction fn; };
		static const Entry table[] = {
			{"linear", TransitionFunction::Linear},
			{"easeinquad", TransitionFunction::EaseInQuad},   {"easeoutquad", TransitionFunction::EaseOutQuad},
			{"easeinoutquad", TransitionFunction::EaseInOutQuad},
			{"easeincubic", TransitionFunction::EaseInCubic}, {"easeoutcubic", TransitionFunction::EaseOutCubic},
			{"easeinoutcubic", TransitionFunction::EaseInOutCubic},
			{"easeinback", TransitionFunction::EaseInBack},   {"easeoutback", TransitionFunction::EaseOutBack},
			{"easeoutelastic", TransitionFunction::EaseOutElastic},
			{"easeoutbounce", TransitionFunction::EaseOutBounce},
		};
		for (const auto& e : table) {
			if (n == e.key) { out = e.fn; return true; }
		}
		return false;
	}
}