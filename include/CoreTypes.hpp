#pragma once

/**
 * \file CoreTypes.hpp
 * \brief Small value types shared by the layout, rendering and input layers.
 *
 * Nothing in this header depends on other ZenitUI headers except
 * `Common.hpp` and `SmallVector.hpp`, so it can be included from anywhere.
 */

#include "Common.hpp"
#include "SmallVector.hpp"

#include <algorithm>

namespace ZenitUI
{

    /** \brief 2D vector with component-wise arithmetic. */
    struct Vec2
    {
        float x{0.0f}, y{0.0f};
    };
    inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
    inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
    inline Vec2 operator*(Vec2 a, float k) { return {a.x * k, a.y * k}; }

    /** \brief 3D vector. Used by renderer effect uniforms. */
    struct Vec3
    {
        float x{0.0f}, y{0.0f}, z{0.0f};
    };

    /** \brief 4D vector with an implicit `w = 1` default. Used for RGBA uniforms. */
    struct Vec4
    {
        float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};
    };

    /** \brief Axis-aligned rectangle in screen coordinates. */
    struct Rect
    {
        float x{0.0f}, y{0.0f}, width{0.0f}, height{0.0f};

        /** \brief Inclusive point-in-rect test. Touching the edge counts as inside. */
        bool contains(Vec2 p) const
        {
            return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
        }
        /** \brief Center point of the rectangle. */
        Vec2 center() const { return {x + width * 0.5f, y + height * 0.5f}; }
        /** \brief Same data as a `Vec2`. */
        Vec2 size() const { return {width, height}; }
    };

    /** \brief Four-sided edge inset (used for padding, margin, borders). */
    struct EdgeInsets
    {
        float top{0.0f}, right{0.0f}, bottom{0.0f}, left{0.0f};

        bool empty() const
        {
            return top == 0.0f && right == 0.0f && bottom == 0.0f && left == 0.0f;
        }
        bool operator==(const EdgeInsets &o) const
        {
            return top == o.top && right == o.right && bottom == o.bottom && left == o.left;
        }
        bool operator!=(const EdgeInsets &o) const { return !(*this == o); }
    };

    /** \brief 8-bit-per-channel RGBA color, non-premultiplied. */
    struct Color
    {
        uint8_t r{255}, g{255}, b{255}, a{255};

        /**
         * \brief Return a copy with the alpha channel scaled by \p k.
         * \param k Multiplier in `[0, 1]`, clamped. Multiplies the existing alpha.
         */
        Color withAlpha(float k) const
        {
            float f = std::clamp(k, 0.0f, 1.0f);
            return {r, g, b, static_cast<uint8_t>(std::lround(a * f))};
        }
        bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
        bool operator!=(const Color &o) const { return !(*this == o); }
    };

    /** \brief Named color constants. */
    namespace Colors
    {
        inline constexpr Color White{255, 255, 255, 255};
        inline constexpr Color Black{0, 0, 0, 255};
        inline constexpr Color Blank{0, 0, 0, 0};
        inline constexpr Color Red{230, 41, 55, 255};
        inline constexpr Color Maroon{190, 33, 55, 255};
        inline constexpr Color Green{0, 228, 48, 255};
        inline constexpr Color DarkGreen{0, 117, 44, 255};
        inline constexpr Color Blue{0, 121, 241, 255};
        inline constexpr Color Yellow{253, 249, 0, 255};
        inline constexpr Color Gray{130, 130, 130, 255};
        inline constexpr Color DarkGray{80, 80, 80, 255};
        inline constexpr Color LightGray{200, 200, 200, 255};
    }

    /** \brief Opaque handle to a GPU/software texture owned by the asset provider. */
    struct TextureHandle
    {
        uint32_t id{0};
        int width{0}, height{0};
        bool valid() const { return id != 0; }
    };

    /** \brief Opaque handle to a font loaded by the asset provider. */
    struct FontHandle
    {
        uint32_t id{0};
    };

    /** \brief Opaque handle to a shader/effect owned by the asset provider. */
    struct EffectHandle
    {
        uint32_t id{0};
        bool valid() const { return id != 0; }
    };

    /**
     * \brief Opaque handle to an off-screen render target.
     *
     * `width` and `height` mirror the logical size of the target and are
     * cached on the handle to let callers avoid an extra query. They are
     * advisory: always check `valid()` before use.
     */
    struct TargetHandle
    {
        uint32_t id{0};
        int width{0}, height{0};
        bool valid() const { return id != 0; }
    };

    /** \brief Nine-slice inset values for scalable textures. */
    struct NineSlice
    {
        int left{0}, top{0}, right{0}, bottom{0};
    };

    /**
     * \brief 2D affine transform stored as pivot + translate + rotation + scale.
     *
     * Applying: translate the pivot to origin, scale, rotate, then add pivot
     * back and apply the translation. Matches the CSS `transform-origin`
     * convention used by the layout system.
     *
     * \sa applyTransform
     * \sa applyInverseTransform
     */
    struct Transform2D
    {
        Vec2 pivot;
        Vec2 translate;
        float rotationDeg{0.0f};
        float scale{1.0f};
    };

    /**
     * \brief Ordered list of font family names to try, first match wins.
     *
     * \sa IAssetProvider::getFont(const FontStack&)
     */
    struct FontStack : SmallVector<std::string, 3>
    {
        FontStack() = default;
        using SmallVector<std::string, 3>::SmallVector;

        FontStack(const char *s)
            : SmallVector<std::string, 3>{std::string(s)} {}
        FontStack(std::string s)
            : SmallVector<std::string, 3>{std::move(s)} {}
        FontStack(std::string_view s)
            : SmallVector<std::string, 3>{std::string(s)} {}
    };

    /** \brief Mouse buttons tracked by the input system. `Count` is the sentinel. */
    enum class MouseButton : uint8_t
    {
        Left,
        Right,
        Middle,
        Count
    };

    /** \brief Per-button state snapshot for the current frame. */
    struct ButtonState
    {
        bool down{false};      ///< Button is currently held.
        bool pressed{false};   ///< Button transitioned from up to down this frame.
        bool released{false};  ///< Button transitioned from down to up this frame.
    };

    /** \brief Pointer position, wheel delta, and per-button state for one frame. */
    struct PointerState
    {
        Vec2 pos;
        float wheelY{0.0f};

        std::array<ButtonState, (size_t)MouseButton::Count> buttons{};

        ButtonState &operator[](MouseButton b) { return buttons[(size_t)b]; }
        const ButtonState &operator[](MouseButton b) const { return buttons[(size_t)b]; }
    };

    /** \brief Keyboard modifier snapshot. */
    struct ModifierState
    {
        bool shift{false};
        bool ctrl{false};
        bool alt{false};
        bool super{false};
    };

    /** \brief Global viewport metrics, updated once per frame from the platform. */
    struct Metrics
    {
        static inline Vec2 viewport{1280.0f, 720.0f};
    };

    /**
     * \brief Platform-agnostic key constants.
     *
     * Printable characters are delivered through `InputEvents::chars`; these
     * constants cover navigation and modifier keys only. Values are stable
     * across platforms.
     */
    namespace Key
    {
        constexpr int Backspace = 1;
        constexpr int Delete = 2;
        constexpr int Enter = 3;
        constexpr int Escape = 4;
        constexpr int Left = 5;
        constexpr int Right = 6;
        constexpr int Home = 7;
        constexpr int End = 8;
        constexpr int Tab = 9;
        constexpr int Space = 10;

        constexpr int Up = 11;
        constexpr int Down = 12;

        constexpr int A = 65;
        constexpr int C = 67;
        constexpr int V = 86;
        constexpr int X = 88;

        constexpr int Y = 89;
        constexpr int Z = 90;
    }

    /** \brief One frame's worth of keyboard input. */
    struct InputEvents
    {
        std::vector<int> chars;  ///< Printable characters typed this frame.
        std::vector<int> keys;   ///< Keys pressed this frame (edge-triggered).
        std::vector<int> held;   ///< Keys currently held (level-triggered).
    };

    /** \brief Cross-axis alignment for flex items and content. */
    enum class Align
    {
        Auto,     ///< Defer to the parent's `alignItems`.
        Start,    ///< Pack at the leading edge.
        Center,   ///< Center within the available space.
        End,      ///< Pack at the trailing edge.
        Stretch   ///< Stretch to fill the cross axis.
    };

    /** \brief Main-axis distribution for flex containers. */
    enum class Justify
    {
        Start,         ///< Pack at the leading edge.
        Center,        ///< Center the whole set.
        End,           ///< Pack at the trailing edge.
        SpaceBetween   ///< Distribute leftover space between items.
    };

    /**
     * \brief Apply a `Transform2D` to a point.
     * \param t The transform.
     * \param p Point in the transform's local space.
     * \return Point in the transform's destination space.
     */
    inline Vec2 applyTransform(const Transform2D &t, Vec2 p)
    {
        float dx = p.x - t.pivot.x;
        float dy = p.y - t.pivot.y;
        dx *= t.scale;
        dy *= t.scale;

        float rad = t.rotationDeg * 3.14159265358979323846f / 180.0f;
        float c = std::cos(rad), s = std::sin(rad);
        float rx = dx * c - dy * s;
        float ry = dx * s + dy * c;

        return {rx + t.pivot.x + t.translate.x, ry + t.pivot.y + t.translate.y};
    }

    /**
     * \brief Invert a `Transform2D` and apply it to a point.
     *
     * Used by hit-testing: given the pointer position in destination space,
     * recover the position in the transform's local space.
     *
     * \pre `t.scale != 0` (division by scale).
     */
    inline Vec2 applyInverseTransform(const Transform2D &t, Vec2 p)
    {
        p.x -= t.translate.x;
        p.y -= t.translate.y;
        float dx = p.x - t.pivot.x, dy = p.y - t.pivot.y;
        float rad = -t.rotationDeg * 3.14159265358979323846f / 180.0f;
        float c = std::cos(rad), s = std::sin(rad);
        float rx = dx * c - dy * s;
        float ry = dx * s + dy * c;
        rx /= t.scale;
        ry /= t.scale;
        return {rx + t.pivot.x, ry + t.pivot.y};
    }
}