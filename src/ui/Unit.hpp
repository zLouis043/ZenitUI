#pragma once

#include "Common.hpp"

#include "CoreTypes.hpp"

namespace ZenitUI {

	enum class Unit { Auto, Pixel, Percent, VW, VH };

	struct Value {
		float amount{ 0.0f };
		Unit unit{ Unit::Auto };
		float px{ 0.0f };

		Value() = default;
		Value(float pixels) : amount(pixels), unit(Unit::Pixel) {}
		Value(float a, Unit u) : amount(a), unit(u) {}
		Value(float a, Unit u, float extraPx) : amount(a), unit(u), px(extraPx) {}

		static Value autoSize() { return Value(0.0f, Unit::Auto); }
		bool isAuto() const { return unit == Unit::Auto; }

		float resolve(float parent, float autoValue = 0.0f) const {
			switch (unit) {
			case Unit::Auto:    return autoValue;
			case Unit::Pixel:   return amount + px;
			case Unit::Percent: return amount * 0.01f * parent + px;
			case Unit::VW:      return amount * 0.01f * Metrics::viewport.x + px;
			case Unit::VH:      return amount * 0.01f * Metrics::viewport.y + px;
			}
			return 0.0f;
		}

		float resolveSelf(float selfSize) const {
			switch (unit) {
			case Unit::Auto:    return 0.0f;
			case Unit::Pixel:   return amount + px;
			case Unit::Percent: return amount * 0.01f * selfSize + px;
			case Unit::VW:      return amount * 0.01f * Metrics::viewport.x + px;
			case Unit::VH:      return amount * 0.01f * Metrics::viewport.y + px;
			}
			return 0.0f;
		}

		Value operator+(const Value& o) const {
			assert(!isAuto() && !o.isAuto() && "Auto non supporta operator+/-");
			if (unit == o.unit) return Value(amount + o.amount, unit, px + o.px);
			if (unit == Unit::Pixel) return Value(o.amount, o.unit, o.px + px + amount);
			if (o.unit == Unit::Pixel) return Value(amount, unit, px + o.px + o.amount);
			assert(unit != Unit::Percent && o.unit != Unit::Percent && "Percent non e' combinabile con vw/vh");
			return Value(resolve(0.0f) + o.resolve(0.0f));
		}
		Value operator-() const { return Value(-amount, unit, -px); }
		Value operator-(const Value& o) const { return *this + (-o); }
		Value operator*(float k) const { return Value(amount * k, unit, px * k); }

		bool operator==(const Value& o) const { return amount == o.amount && unit == o.unit && px == o.px; }
		bool operator!=(const Value& o) const { return !(*this == o); }
	};

	inline Value Auto() { return Value::autoSize(); }
	inline Value Px(float p) { return Value(p, Unit::Pixel); }
	inline Value Percent(float p) { return Value(p, Unit::Percent); }
	inline Value VW(float p) { return Value(p, Unit::VW); }
	inline Value VH(float p) { return Value(p, Unit::VH); }
}