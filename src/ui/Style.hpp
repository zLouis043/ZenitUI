#pragma once

#include "Common.hpp"

#include "Easing.hpp"
#include "Unit.hpp"

namespace ZenitUI {

	template <typename T>
	struct Opt {
		T value{};
		bool is_set{ false };

		Opt() = default;
		Opt(const T& v) : value(v), is_set(true) {}
		Opt& operator=(const T& v) { value = v; is_set = true; return *this; }
		void reset() { is_set = false; }
		T get_or(const T& fallback) const { return is_set ? value : fallback; }
	};

	struct Spacing {
		Value top{ 0.0f }, right{ 0.0f }, bottom{ 0.0f }, left{ 0.0f };

		Spacing() = default;
		Spacing(float all) : top(all), right(all), bottom(all), left(all) {}
		Spacing(Value all) : top(all), right(all), bottom(all), left(all) {}
		Spacing(Value v, Value h) : top(v), right(h), bottom(v), left(h) {}
		Spacing(Value v, Value h, Value b) : top(v), right(h), bottom(b), left(h) {}
		Spacing(Value t, Value r, Value b, Value l) : top(t), right(r), bottom(b), left(l) {}

		bool operator==(const Spacing& o) const {
			return top == o.top && right == o.right && bottom == o.bottom && left == o.left;
		}
		bool operator!=(const Spacing& o) const { return !(*this == o); }
	};

	struct TransitionSpec {
		std::string prop;
		float duration{ 0.15f };
		TransitionFunction ease{ TransitionFunction::Linear };
		float delay{ 0.0f };
	};

	struct AnimationRef {
		std::string name;
		float duration{ 1.0f };
		TransitionFunction ease{ TransitionFunction::Linear };
		float delay{ 0.0f };
		int iterations{ 1 };
		bool alternate{ false };
		bool fillForwards{ false };
		bool blocksInput{ false };
	};

#define BUBBLE_STYLE_PROPS(X) \
	X(Value,   width,          Value::autoSize()) \
	X(Value,   height,         Value::autoSize()) \
	X(Value,   minWidth,       Value::autoSize()) \
	X(Value,   minHeight,      Value::autoSize()) \
	X(Value,   maxWidth,       Value::autoSize()) \
	X(Value,   maxHeight,      Value::autoSize()) \
	X(float,   grow,           0.0f) \
	X(Value,   gap,            Value(0.0f)) \
	X(Spacing, margin,         Spacing()) \
	X(Spacing, padding,        Spacing()) \
	X(Align,   alignH,         Align::Auto) \
	X(Align,   alignV,         Align::Auto) \
	X(Align,   itemsH,         Align::Start) \
	X(Align,   itemsV,         Align::Start) \
	X(Justify, justify,        Justify::Start) \
	X(Color,   background,     Colors::Blank) \
	X(Color,   color,          Colors::White) \
	X(Color,   tint,           Colors::White) \
	X(Color,   borderColor,    Colors::Blank) \
	X(Value,   borderWidth,    Value(0.0f)) \
	X(Value,   radius,         Value(0.0f)) \
	X(float,   opacity,        1.0f) \
	X(float,   scale,          1.0f) \
	X(float,   rotation,       0.0f) \
	X(Value,   translateX,     Value(0.0f)) \
	X(Value,   translateY,     Value(0.0f)) \
	X(Value,   fontSize,       Value(20.0f)) \
	X(Value,   letterSpacing,  Value(2.0f)) \
	X(float,   transitionTime, 0.15f) \
	X(TransitionFunction, ease, TransitionFunction::Linear)

	struct Style {
#define X(T, name, def) Opt<T> name;
		BUBBLE_STYLE_PROPS(X)
#undef X

		// Transizioni per-property. Assente = eredita dal livello precedente.
		Opt<std::vector<TransitionSpec>> transitions;
		Opt<std::vector<AnimationRef>>   animations;

		Style& overlay(const Style& o) {
#define X(T, name, def) if (o.name.is_set) name = o.name;
			BUBBLE_STYLE_PROPS(X)
#undef X
			if (o.transitions.is_set) transitions = o.transitions;
			if (o.animations.is_set)  animations  = o.animations;
			return *this;
		}
	};

	struct ComputedStyle {
#define X(T, name, def) T name = def;
		BUBBLE_STYLE_PROPS(X)
#undef X

		std::vector<TransitionSpec> transitions;
		std::vector<AnimationRef>   animations;

		static ComputedStyle from(const Style& s) {
			ComputedStyle c;
#define X(T, name, def) c.name = s.name.get_or(def);
			BUBBLE_STYLE_PROPS(X)
#undef X
			if (s.transitions.is_set) c.transitions = s.transitions.value;
			if (s.animations.is_set)  c.animations  = s.animations.value;
			return c;
		}
	};

	inline bool operator==(const ComputedStyle& a, const ComputedStyle& b) {
#define X(T, name, def) if (!(a.name == b.name)) return false;
		BUBBLE_STYLE_PROPS(X)
#undef X
		return true;
	}
	inline bool operator!=(const ComputedStyle& a, const ComputedStyle& b) { return !(a == b); }

	struct StyleSet {
		Style base, hover, pressed, disabled;
	};

	inline float lerpProp(float a, float b, float t) { return a + (b - a) * t; }

	inline Color lerpProp(const Color& a, const Color& b, float t) {
		auto mix = [t](uint8_t x, uint8_t y) {
			float v = static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t;
			return static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
		};
		return { mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a) };
	}

	inline Value lerpProp(const Value& a, const Value& b, float t) {
		if (a.unit == b.unit) return Value(lerpProp(a.amount, b.amount, t), a.unit, lerpProp(a.px, b.px, t));
		return t > 0.0f ? b : a;
	}

	inline Spacing lerpProp(const Spacing& a, const Spacing& b, float t) {
		return Spacing(lerpProp(a.top, b.top, t), lerpProp(a.right, b.right, t),
			lerpProp(a.bottom, b.bottom, t), lerpProp(a.left, b.left, t));
	}

	template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
	inline E lerpProp(E a, E b, float t) { return t > 0.0f ? b : a; }

	// Lerp globale (retro-compat): t è già normalizzato in [0,1].
	inline ComputedStyle lerpStyle(const ComputedStyle& a, const ComputedStyle& b, float t) {
		ComputedStyle r;
#define X(T, name, def) r.name = lerpProp(a.name, b.name, t);
		BUBBLE_STYLE_PROPS(X)
#undef X
		r.transitions = b.transitions;
		return r;
	}

	// Lerp per-property: realElapsed è il tempo in secondi dall'inizio della transizione.
	// Ogni prop usa la sua duration/ease se presente in transitions; altrimenti
	// fallback su transitionTime/ease globali di "b".
	inline ComputedStyle lerpStyleTimed(const ComputedStyle& a, const ComputedStyle& b, float realElapsed) {
		ComputedStyle r;

		auto timingFor = [&](const char* propName) -> std::pair<float, TransitionFunction> {
			for (const auto& spec : b.transitions) {
				if (spec.prop == propName || spec.prop == "all")
					return { spec.duration, spec.ease };
			}
			return { b.transitionTime, b.ease };
		};

#define X(T, name, def) \
		{ \
			auto [dur, ease] = timingFor(#name); \
			float tp = (dur > 0.0f) ? std::clamp(realElapsed / dur, 0.0f, 1.0f) : 1.0f; \
			r.name = lerpProp(a.name, b.name, getRatio(tp, ease)); \
		}
		BUBBLE_STYLE_PROPS(X)
#undef X

		r.transitions = b.transitions;
		return r;
	}

	// Applica un Style (con Opt) su un ComputedStyle esistente.
	// Usato per l'overlay delle animazioni CSS sul render-style.
	inline void overlayComputed(ComputedStyle& dst, const Style& src) {
#define X(T, name, def) if (src.name.is_set) dst.name = src.name.value;
		BUBBLE_STYLE_PROPS(X)
#undef X
	}

	using PropValue = std::variant<float, Value, Color, Spacing, Align, Justify, TransitionFunction>;

	struct PropDesc {
		const char* name;
		PropValue(*get)(const ComputedStyle&);
		void (*set)(ComputedStyle&, const PropValue&);
	};

	inline const std::vector<PropDesc>& propTable() {
		static const std::vector<PropDesc> table = {
#define X(T, name, def) PropDesc{ #name, \
			[](const ComputedStyle& c) -> PropValue { return c.name; }, \
			[](ComputedStyle& c, const PropValue& v) { if (const T* p = std::get_if<T>(&v)) c.name = *p; } },
			BUBBLE_STYLE_PROPS(X)
#undef X
		};
		return table;
	}

	inline const PropDesc* findProp(std::string_view name) {
		for (const auto& d : propTable()) {
			if (name == d.name) return &d;
		}
		return nullptr;
	}

	inline PropValue lerpValue(const PropValue& a, const PropValue& b, float t) {
		return std::visit([&](const auto& x) -> PropValue {
			using T = std::decay_t<decltype(x)>;
			if (const T* y = std::get_if<T>(&b)) return lerpProp(x, *y, t);
			return x;
		}, a);
	}
}