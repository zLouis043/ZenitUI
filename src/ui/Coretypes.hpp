#pragma once

#include "Common.hpp"

namespace ZenitUI {

	struct Vec2 {
		float x{ 0.0f }, y{ 0.0f };
	};
	inline Vec2 operator+(Vec2 a, Vec2 b) { return { a.x + b.x, a.y + b.y }; }
	inline Vec2 operator-(Vec2 a, Vec2 b) { return { a.x - b.x, a.y - b.y }; }
	inline Vec2 operator*(Vec2 a, float k) { return { a.x * k, a.y * k }; }

	struct Rect {
		float x{ 0.0f }, y{ 0.0f }, width{ 0.0f }, height{ 0.0f };

		bool contains(Vec2 p) const {
			return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
		}
		Vec2 center() const { return { x + width * 0.5f, y + height * 0.5f }; }
		Vec2 size() const { return { width, height }; }
	};

	struct Color {
		uint8_t r{ 255 }, g{ 255 }, b{ 255 }, a{ 255 };
		Color withAlpha(float k) const {
			float f = std::clamp(k, 0.0f, 1.0f);
			return { r, g, b, static_cast<uint8_t>(std::lround(a * f)) };
		}
		bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
		bool operator!=(const Color& o) const { return !(*this == o); }
	};

	namespace Colors {
		inline constexpr Color White{ 255, 255, 255, 255 };
		inline constexpr Color Black{ 0, 0, 0, 255 };
		inline constexpr Color Blank{ 0, 0, 0, 0 };
		inline constexpr Color Red{ 230, 41, 55, 255 };
		inline constexpr Color Maroon{ 190, 33, 55, 255 };
		inline constexpr Color Green{ 0, 228, 48, 255 };
		inline constexpr Color DarkGreen{ 0, 117, 44, 255 };
		inline constexpr Color Blue{ 0, 121, 241, 255 };
		inline constexpr Color Yellow{ 253, 249, 0, 255 };
		inline constexpr Color Gray{ 130, 130, 130, 255 };
		inline constexpr Color DarkGray{ 80, 80, 80, 255 };
		inline constexpr Color LightGray{ 200, 200, 200, 255 };
	}

	struct TextureHandle {
		uint32_t id{ 0 };
		int width{ 0 }, height{ 0 };
		bool valid() const { return id != 0; }
	};
	struct FontHandle   { uint32_t id{ 0 }; };   // id 0 = font di default del backend
	struct EffectHandle { uint32_t id{ 0 }; bool valid() const { return id != 0; } };   // ex Shader
	struct TargetHandle {
		uint32_t id{ 0 };
		int width{ 0 }, height{ 0 };
		bool valid() const { return id != 0; }
	};
	struct NineSlice { int left{ 0 }, top{ 0 }, right{ 0 }, bottom{ 0 }; };

	struct Transform2D {
		Vec2 pivot;
		Vec2 translate;
		float rotationDeg{ 0.0f };
		float scale{ 1.0f };
	};

	struct PointerState {
		Vec2 pos;
		bool down{ false };
		bool pressed{ false };
		bool released{ false };
		bool rightDown{ false };
		bool rightPressed{ false };
		bool rightReleased{ false };
		float wheelY{ 0.0f };
	};

	struct Metrics {
		static inline Vec2 viewport{ 1280.0f, 720.0f };
	};

	// Codici tasto "speciali" (non caratteri). Indipendenti dal backend.
	namespace Key {
		constexpr int Backspace = 1;
		constexpr int Delete    = 2;
		constexpr int Enter     = 3;
		constexpr int Escape    = 4;
		constexpr int Left      = 5;
		constexpr int Right     = 6;
		constexpr int Home      = 7;
		constexpr int End       = 8;
		constexpr int Tab       = 9;
		constexpr int Space     = 10;
	}

	struct InputEvents {
		std::vector<int> chars;  // codici Unicode digitati questo frame
		std::vector<int> keys;   // codici Key::* speciali premuti questo frame
		std::vector<int> held; 
	};

	enum class Align { Auto, Start, Center, End, Stretch };
	enum class Justify { Start, Center, End, SpaceBetween };

	inline Vec2 applyTransform(const Transform2D& t, Vec2 p) {
		float dx = p.x - t.pivot.x;
		float dy = p.y - t.pivot.y;
		dx *= t.scale; dy *= t.scale;

		float rad = t.rotationDeg * 3.14159265358979323846f / 180.0f;
		float c = std::cos(rad), s = std::sin(rad);
		float rx = dx * c - dy * s;
		float ry = dx * s + dy * c;

		return { rx + t.pivot.x + t.translate.x, ry + t.pivot.y + t.translate.y };
	}

	inline Vec2 applyInverseTransform(const Transform2D& t, Vec2 p) {
		p.x -= t.translate.x;
		p.y -= t.translate.y;
		float dx = p.x - t.pivot.x, dy = p.y - t.pivot.y;
		float rad = -t.rotationDeg * 3.14159265358979323846f / 180.0f;
		float c = std::cos(rad), s = std::sin(rad);
		float rx = dx * c - dy * s;
		float ry = dx * s + dy * c;
		rx /= t.scale; ry /= t.scale;
		return { rx + t.pivot.x, ry + t.pivot.y };
	}
}