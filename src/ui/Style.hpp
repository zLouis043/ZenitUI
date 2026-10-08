#pragma once

#include "Common.hpp"

#include "Easing.hpp"
#include "Unit.hpp"

namespace ZenitUI
{

	template <typename T>
	struct Opt
	{
		T value{};
		bool is_set{false};

		Opt() = default;
		Opt(const T &v) : value(v), is_set(true) {}
		Opt &operator=(const T &v)
		{
			value = v;
			is_set = true;
			return *this;
		}
		void reset() { is_set = false; }
		T get_or(const T &fallback) const { return is_set ? value : fallback; }
	};

	struct Spacing
	{
		Value top{0.0f}, right{0.0f}, bottom{0.0f}, left{0.0f};

		Spacing() = default;
		Spacing(float all) : top(all), right(all), bottom(all), left(all) {}
		Spacing(Value all) : top(all), right(all), bottom(all), left(all) {}
		Spacing(Value v, Value h) : top(v), right(h), bottom(v), left(h) {}
		Spacing(Value v, Value h, Value b) : top(v), right(h), bottom(b), left(h) {}
		Spacing(Value t, Value r, Value b, Value l) : top(t), right(r), bottom(b), left(l) {}

		bool operator==(const Spacing &o) const
		{
			return top == o.top && right == o.right && bottom == o.bottom && left == o.left;
		}
		bool operator!=(const Spacing &o) const { return !(*this == o); }
	};

	struct TransitionSpec
	{
		std::string prop;
		float duration{0.15f};
		TransitionFunction ease{TransitionFunction::Linear};
		float delay{0.0f};
	};

	struct AnimationRef
	{
		std::string name;
		float duration{1.0f};
		TransitionFunction ease{TransitionFunction::Linear};
		float delay{0.0f};
		int iterations{1};
		bool alternate{false};
		bool fillForwards{false};
		bool blocksInput{false};
	};

	enum class Position
	{
		Static,
		Relative,
		Absolute
	};

	struct ZIndex
	{
		bool isAuto{true};
		int value{0};

		ZIndex() = default;
		ZIndex(int v) : isAuto(false), value(v) {}

		static ZIndex Auto() { return ZIndex(); }

		bool operator==(const ZIndex &o) const { return isAuto == o.isAuto && value == o.value; }
		bool operator!=(const ZIndex &o) const { return !(*this == o); }
	};

	enum class Overflow
	{
		Visible,
		Hidden,
		Scroll,
		Auto
	};

	struct TextureRef
	{
		std::string name;
		int left{0}, top{0}, right{0}, bottom{0};

		bool isNineSlice() const { return left || top || right || bottom; }

		bool operator==(const TextureRef &o) const
		{
			return name == o.name &&
				   left == o.left && top == o.top &&
				   right == o.right && bottom == o.bottom;
		}
		bool operator!=(const TextureRef &o) const { return !(*this == o); }
	};

	// Un "elemento" di un selettore composto. Es. per "Toggle:checked .knob"
	// la chain è: [ {Tag, "Toggle", requireChecked}, {Class, "knob", idle} ].
	struct SimpleSelector
	{
		enum class Kind
		{
			Tag,
			Class,
			Id
		};
		std::string name;
		Kind kind{Kind::Tag};

		bool requireHover{false};
		bool requirePressed{false};
		bool requireFocus{false};
		bool requireDisabled{false};
		bool requireChecked{false};
	};

	// Un filtro CSS (post-process): "blur(4px)", "drop-shadow(2px, 2px, #000)".
	// Il Layout lo traduce in un EffectHandle per nome; gli args restano raw.
	struct FilterRef
	{
		std::string name;
		std::vector<std::string> args;

		bool operator==(const FilterRef &o) const
		{
			return name == o.name && args == o.args;
		}
		bool operator!=(const FilterRef &o) const { return !(*this == o); }
	};

	inline bool operator==(const std::vector<FilterRef> &a, const std::vector<FilterRef> &b)
	{
		if (a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); ++i)
			if (!(a[i] == b[i]))
				return false;
		return true;
	}
	inline bool operator!=(const std::vector<FilterRef> &a, const std::vector<FilterRef> &b)
	{
		return !(a == b);
	}

#define BUBBLE_STYLE_PROPS(X)                               \
	X(Value, width, Value::autoSize())                      \
	X(Value, height, Value::autoSize())                     \
	X(Value, minWidth, Value::autoSize())                   \
	X(Value, minHeight, Value::autoSize())                  \
	X(Value, maxWidth, Value::autoSize())                   \
	X(Value, maxHeight, Value::autoSize())                  \
	X(float, grow, 0.0f)                                    \
	X(float, shrink, 1.0f)                                  \
	X(Value, gap, Value(0.0f))                              \
	X(Spacing, margin, Spacing())                           \
	X(Spacing, padding, Spacing())                          \
	X(Align, alignH, Align::Auto)                           \
	X(Align, alignV, Align::Auto)                           \
	X(Align, itemsH, Align::Start)                          \
	X(Align, itemsV, Align::Start)                          \
	X(Justify, justify, Justify::Start)                     \
	X(Align, textAlign, Align::Auto)                        \
	X(Color, background, Colors::Blank)                     \
	X(Color, color, Colors::White)                          \
	X(Color, tint, Colors::White)                           \
	X(TextureRef, backgroundTexture, TextureRef{})          \
	X(std::string, effect, "")                              \
	X(std::vector<FilterRef>, filters, {})                  \
	X(Color, borderColor, Colors::Blank)                    \
	X(Value, borderWidth, Value(0.0f))                      \
	X(Value, radius, Value(0.0f))                           \
	X(float, opacity, 1.0f)                                 \
	X(float, scale, 1.0f)                                   \
	X(float, rotation, 0.0f)                                \
	X(Value, translateX, Value(0.0f))                       \
	X(Value, translateY, Value(0.0f))                       \
	X(Value, fontSize, Value(20.0f))                        \
	X(Value, letterSpacing, Value(2.0f))                    \
	X(float, transitionTime, 0.15f)                         \
	X(TransitionFunction, ease, TransitionFunction::Linear) \
	X(Overflow, overflowX, Overflow::Visible)               \
	X(Overflow, overflowY, Overflow::Visible)               \
	X(Position, position, Position::Static)                 \
	X(ZIndex, zIndex, ZIndex::Auto())                       \
	X(Value, top, Value::autoSize())                        \
	X(Value, left, Value::autoSize())                       \
	X(Value, right, Value::autoSize())                      \
	X(Value, bottom, Value::autoSize())                     \
	X(std::string, font, "")

	inline bool isInheritedProp(std::string_view name)
	{
		return name == "font" || name == "fontSize" || name == "color" || name == "letterSpacing" || name == "textAlign";
	}

	struct Style; // forward
	inline bool clearStyleProp(Style &s, std::string_view name);

	struct Style
	{
#define X(T, name, def) Opt<T> name;
		BUBBLE_STYLE_PROPS(X)
#undef X

		// Transizioni per-property. Assente = eredita dal livello precedente.
		Opt<std::vector<TransitionSpec>> transitions;
		Opt<std::vector<AnimationRef>> animations;
		// Custom properties (--name). Il valore è raw (non parsato).
		std::unordered_map<std::string, std::string> customProps;
		// Prop che contengono var() e non sono risolvibili a parse time.
		std::unordered_map<std::string, std::string> unresolvedProps;

		Style &overlay(const Style &o)
		{
#define X(T, name, def) \
	if (o.name.is_set)  \
		name = o.name;
			BUBBLE_STYLE_PROPS(X)
#undef X
			if (o.transitions.is_set)
				transitions = o.transitions;
			if (o.animations.is_set)
				animations = o.animations;

			if (!o.customProps.empty())
				for (const auto &[k, v] : o.customProps)
					customProps[k] = v;
			if (!o.unresolvedProps.empty())
				for (const auto &[k, v] : o.unresolvedProps)
				{
					unresolvedProps[k] = v;
					clearStyleProp(*this, k); // cancella la versione tipizzata precedente
				}

			return *this;
		}
	};

	inline bool clearStyleProp(Style &s, std::string_view name)
	{
		// Usiamo una lambda per non dover scrivere 40 if.
		bool cleared = false;
#define X(T, n, def)            \
	if (!cleared && name == #n) \
	{                           \
		s.n.reset();            \
		cleared = true;         \
	}
		BUBBLE_STYLE_PROPS(X)
#undef X
		return cleared;
	}

	struct ComputedStyle
	{
#define X(T, name, def) T name = def;
		BUBBLE_STYLE_PROPS(X)
#undef X

		std::vector<TransitionSpec> transitions;
		std::vector<AnimationRef> animations;
		std::unordered_map<std::string, std::string> customProps;

		static ComputedStyle from(const Style &s,
								  const ComputedStyle *parent = nullptr,
								  const Style *root = nullptr)
		{
			ComputedStyle c;
#define X(T, name, def)                        \
	if (s.name.is_set)                         \
		c.name = s.name.value;                 \
	else if (parent && isInheritedProp(#name)) \
		c.name = parent->name;                 \
	else if (root && root->name.is_set)        \
		c.name = root->name.value;             \
	else                                       \
		c.name = def;
			BUBBLE_STYLE_PROPS(X)
#undef X
			if (c.overflowX == Overflow::Visible && c.overflowY != Overflow::Visible)
				c.overflowX = Overflow::Auto;
			else if (c.overflowY == Overflow::Visible && c.overflowX != Overflow::Visible)
				c.overflowY = Overflow::Auto;

			if (s.transitions.is_set)
				c.transitions = s.transitions.value;
			if (s.animations.is_set)
				c.animations = s.animations.value;

			if (parent)
				c.customProps = parent->customProps;
			for (const auto &[k, v] : s.customProps)
				c.customProps[k] = v;
			return c;
		}
	};

	inline bool operator==(const ComputedStyle &a, const ComputedStyle &b)
	{
#define X(T, name, def)      \
	if (!(a.name == b.name)) \
		return false;
		BUBBLE_STYLE_PROPS(X)
#undef X
		return true;
	}
	inline bool operator!=(const ComputedStyle &a, const ComputedStyle &b) { return !(a == b); }

	inline float lerpProp(float a, float b, float t) { return a + (b - a) * t; }

	inline Color lerpProp(const Color &a, const Color &b, float t)
	{
		auto mix = [t](uint8_t x, uint8_t y)
		{
			float v = static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t;
			return static_cast<uint8_t>(std::clamp(std::lround(v), 0L, 255L));
		};
		return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
	}

	inline Value lerpProp(const Value &a, const Value &b, float t)
	{
		// Auto su entrambi → resta Auto
		if (a.isAuto() && b.isAuto())
			return Value::autoSize();
		// Auto su uno solo → snap (non sappiamo da quale valore partire)
		if (a.isAuto() || b.isAuto())
			return t > 0.0f ? b : a;

		// Raccogli i coefficienti per unità in due array (una entry per unità).
		float ca[7] = {0}, cb[7] = {0};
		for (const auto &term : a.terms)
		{
			int i = static_cast<int>(term.unit);
			if (i >= 0 && i < 7)
				ca[i] += term.coeff;
		}
		for (const auto &term : b.terms)
		{
			int i = static_cast<int>(term.unit);
			if (i >= 0 && i < 7)
				cb[i] += term.coeff;
		}

		// Interpola termine-a-termine, includendo le unità presenti in una sola
		// delle due (l'altra contribuisce con coefficiente 0).
		Value r;
		for (int i = 0; i < 7; ++i)
		{
			float v = ca[i] + (cb[i] - ca[i]) * t;
			if (v != 0.0f)
				r.terms.push_back({v, static_cast<Unit>(i)});
		}
		return r;
	}

	inline Spacing lerpProp(const Spacing &a, const Spacing &b, float t)
	{
		return Spacing(lerpProp(a.top, b.top, t), lerpProp(a.right, b.right, t),
					   lerpProp(a.bottom, b.bottom, t), lerpProp(a.left, b.left, t));
	}

	template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
	inline E lerpProp(E a, E b, float t) { return t > 0.0f ? b : a; }

	inline ZIndex lerpProp(const ZIndex &a, const ZIndex &b, float t) { return t > 0.0f ? b : a; }

	inline std::string lerpProp(const std::string &a, const std::string &b, float t)
	{
		return t > 0.0f ? b : a;
	}

	inline std::vector<FilterRef> lerpProp(const std::vector<FilterRef> &a,
										   const std::vector<FilterRef> &b,
										   float t)
	{
		return t > 0.0f ? b : a;
	}

	inline TextureRef lerpProp(const TextureRef &a, const TextureRef &b, float t)
	{
		return t > 0.0f ? b : a;
	}

	// Lerp globale (retro-compat): t è già normalizzato in [0,1].
	inline ComputedStyle lerpStyle(const ComputedStyle &a, const ComputedStyle &b, float t)
	{
		ComputedStyle r;
#define X(T, name, def) r.name = lerpProp(a.name, b.name, t);
		BUBBLE_STYLE_PROPS(X)
#undef X
		r.transitions = b.transitions;
		r.customProps = b.customProps;
		return r;
	}

	// Lerp per-property: realElapsed è il tempo in secondi dall'inizio della transizione.
	// Ogni prop usa la sua duration/ease se presente in transitions; altrimenti
	// fallback su transitionTime/ease globali di "b".
	inline ComputedStyle lerpStyleTimed(const ComputedStyle &a, const ComputedStyle &b, float realElapsed)
	{
		ComputedStyle r;

		auto timingFor = [&](const char *propName) -> std::pair<float, TransitionFunction>
		{
			for (const auto &spec : b.transitions)
			{
				if (spec.prop == propName || spec.prop == "all")
					return {spec.duration, spec.ease};
			}
			return {b.transitionTime, b.ease};
		};

#define X(T, name, def)                                                             \
	{                                                                               \
		auto [dur, ease] = timingFor(#name);                                        \
		float tp = (dur > 0.0f) ? std::clamp(realElapsed / dur, 0.0f, 1.0f) : 1.0f; \
		r.name = lerpProp(a.name, b.name, getRatio(tp, ease));                      \
	}
		BUBBLE_STYLE_PROPS(X)
#undef X

		r.transitions = b.transitions;
		r.customProps = b.customProps;
		return r;
	}

	// Confronto "cambia qualcosa?" fra due Style. Salta transitions/animations.
	inline bool stylesDiffer(const Style &a, const Style &b)
	{
#define X(T, name, def)                                   \
	if (a.name.is_set != b.name.is_set)                   \
		return true;                                      \
	if (a.name.is_set && !(a.name.value == b.name.value)) \
		return true;
		BUBBLE_STYLE_PROPS(X)
#undef X

		// Animations: se cambia l'insieme (o i parametri) delle keyframe dichiarate,
		// il part deve aggiornare il target → parte/ferma l'animazione.
		if (a.animations.is_set != b.animations.is_set)
			return true;
		if (a.animations.is_set)
		{
			const auto &av = a.animations.value;
			const auto &bv = b.animations.value;
			if (av.size() != bv.size())
				return true;
			for (size_t i = 0; i < av.size(); ++i)
			{
				if (av[i].name != bv[i].name ||
					av[i].duration != bv[i].duration ||
					av[i].delay != bv[i].delay ||
					av[i].iterations != bv[i].iterations ||
					av[i].alternate != bv[i].alternate ||
					av[i].fillForwards != bv[i].fillForwards ||
					av[i].ease != bv[i].ease)
					return true;
			}
		}
		return false;
	}

	// Lerp element-wise di due Style, con "snap" se una prop è set solo da un lato.
	template <typename T>
	inline Opt<T> lerpOpt(const Opt<T> &a, const Opt<T> &b, float t)
	{
		if (a.is_set && b.is_set)
			return Opt<T>(lerpProp(a.value, b.value, t));
		return b;
	}

	inline Style lerpStyleParts(const Style &a, const Style &b, float t)
	{
		Style r;
#define X(T, name, def) r.name = lerpOpt(a.name, b.name, t);
		BUBBLE_STYLE_PROPS(X)
#undef X
		r.transitions = b.transitions;
		r.animations = b.animations;
		r.customProps = b.customProps;
		r.unresolvedProps = b.unresolvedProps;
		return r;
	}

	// Applica un Style (con Opt) su un ComputedStyle esistente.
	// Usato per l'overlay delle animazioni CSS sul render-style.
	inline void overlayComputed(ComputedStyle &dst, const Style &src)
	{
#define X(T, name, def)  \
	if (src.name.is_set) \
		dst.name = src.name.value;
		BUBBLE_STYLE_PROPS(X)
#undef X
	}

	// Itera le prop "set" di uno Style (esclusi transitions/animations)
	template <typename F>
	inline void forEachSetStyleProp(const Style &s, F &&fn)
	{
#define X(T, name, def) \
	if (s.name.is_set)  \
		fn(#name);
		BUBBLE_STYLE_PROPS(X)
#undef X
	}

	// Copia una prop per nome. Ritorna true se esiste (era set) nella src.
	inline bool copyStyleProp(Style &dst, const Style &src, std::string_view name)
	{
#define X(T, n, def)       \
	if (name == #n)        \
	{                      \
		if (src.n.is_set)  \
		{                  \
			dst.n = src.n; \
			return true;   \
		}                  \
		return false;      \
	}
		BUBBLE_STYLE_PROPS(X)
#undef X
		return false;
	}

	// Ritorna true se la prop è set nella Style.
	inline bool hasStyleProp(const Style &s, std::string_view name)
	{
#define X(T, n, def) \
	if (name == #n)  \
		return s.n.is_set;
		BUBBLE_STYLE_PROPS(X)
#undef X
		return false;
	}

	using PropValue = std::variant<float, Value, Color, Spacing, Align, Justify,
								   TransitionFunction, Overflow, Position, ZIndex,
								   std::string, TextureRef, std::vector<FilterRef>>;

	struct PropDesc
	{
		const char *name;
		PropValue (*get)(const ComputedStyle &);
		void (*set)(ComputedStyle &, const PropValue &);
	};

	inline const std::vector<PropDesc> &propTable()
	{
		static const std::vector<PropDesc> table = {
#define X(T, name, def) PropDesc{#name,                                                      \
								 [](const ComputedStyle &c) -> PropValue { return c.name; }, \
								 [](ComputedStyle &c, const PropValue &v) { if (const T* p = std::get_if<T>(&v)) c.name = *p; }},
			BUBBLE_STYLE_PROPS(X)
#undef X
		};
		return table;
	}

	inline const PropDesc *findProp(std::string_view name)
	{
		for (const auto &d : propTable())
		{
			if (name == d.name)
				return &d;
		}
		return nullptr;
	}

	inline PropValue lerpValue(const PropValue &a, const PropValue &b, float t)
	{
		return std::visit([&](const auto &x) -> PropValue
						  {
			using T = std::decay_t<decltype(x)>;
			if (const T* y = std::get_if<T>(&b)) return lerpProp(x, *y, t);
			return x; }, a);
	}
}