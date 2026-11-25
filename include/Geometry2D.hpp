#pragma once

/**
 * \file Geometry2D.hpp
 * \brief Shared zero-allocation 2D tessellation helpers for `IRenderer` backends.
 *
 * Every builder emits counter-clockwise triangles in NDC (Y-down logical
 * screen space) so backends with OpenGL back-face culling enabled render
 * consistently without winding mismatches.
 */

#include "CoreTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace ZenitUI::Geometry2D
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	inline constexpr int kRoundedCornerSegments = 12;
	inline constexpr int kRoundedRingPoints = 4 * (kRoundedCornerSegments + 1);
	inline constexpr int kCircleMaxSegments = 48;

	/**
	 * \brief Fixed-capacity stack mesh returned by `Geometry2D` builders.
	 *
	 * Avoids heap allocations during per-frame tessellation while exposing
	 * `std::span` views directly consumable by `IRenderer::drawGeometry`.
	 */
	template <size_t MaxVertices, size_t MaxIndices>
	struct StaticMesh
	{
		std::array<Vertex2D, MaxVertices> vertices{};
		std::array<uint32_t, MaxIndices> indices{};
		uint32_t vertexCount{0};
		uint32_t indexCount{0};

		std::span<const Vertex2D> vertexSpan() const noexcept { return {vertices.data(), vertexCount}; }
		std::span<const uint32_t> indexSpan() const noexcept { return {indices.data(), indexCount}; }
		bool empty() const noexcept { return indexCount == 0; }

		void pushVertex(Vec2 pos, Vec2 uv, Color c) noexcept
		{
			vertices[vertexCount++] = Vertex2D{pos, uv, c};
		}

		void pushTri(uint32_t i0, uint32_t i1, uint32_t i2) noexcept
		{
			indices[indexCount++] = i0;
			indices[indexCount++] = i1;
			indices[indexCount++] = i2;
		}

		void pushQuadIndices(uint32_t tl, uint32_t tr, uint32_t br, uint32_t bl) noexcept
		{
			// CCW in NDC (Y-down screen space)
			pushTri(tl, bl, br);
			pushTri(tl, br, tr);
		}
	};

	using QuadMesh = StaticMesh<4, 6>;
	using RectStrokeMesh = StaticMesh<8, 24>;
	using BorderMesh = StaticMesh<16, 24>;
	using NineSliceMesh = StaticMesh<16, 54>;
	using CircleMesh = StaticMesh<kCircleMaxSegments + 1, kCircleMaxSegments * 3>;
	using RoundedFillMesh = StaticMesh<kRoundedRingPoints + 1, kRoundedRingPoints * 3>;
	using RoundedStrokeMesh = StaticMesh<kRoundedRingPoints * 2, kRoundedRingPoints * 6>;

	/**
	 * \struct RoundedBorderRing
	 * \brief Outer and inner vertex loops of a CSS border-box rounded stroke.
	 */
	struct RoundedBorderRing
	{
		std::array<Vec2, kRoundedRingPoints> outer{};
		std::array<Vec2, kRoundedRingPoints> inner{};
		bool empty{true};
		bool isSolidFill{false};
	};

	/** \brief Build the inward-extruded rounded ring for `IRenderer::strokeRoundedRect`. */
	inline RoundedBorderRing buildRoundedBorderRing(Rect r, float radiusPx, float thickness)
	{
		RoundedBorderRing ring;
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f)
			return ring;

		const float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return ring;

		ring.empty = false;
		const float t = std::min(thickness, maxR);
		if (t >= maxR - 0.01f)
		{
			ring.isSolidFill = true;
			return ring;
		}

		const float outerR = std::clamp(radiusPx, 0.0f, maxR);
		const float innerR = std::max(0.0f, outerR - t);

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);
		constexpr float kStartAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};

		const Vec2 outerCenters[4] = {
			{r.x + r.width - outerR, r.y + outerR},
			{r.x + r.width - outerR, r.y + r.height - outerR},
			{r.x + outerR,           r.y + r.height - outerR},
			{r.x + outerR,           r.y + outerR}};

		const Vec2 innerCenters[4] = {
			{r.x + r.width - t - innerR, r.y + t + innerR},
			{r.x + r.width - t - innerR, r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + t + innerR}};

		int idx = 0;
		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				const float a = kStartAngles[corner] + kStep * static_cast<float>(s);
				const float ca = std::cos(a);
				const float sa = std::sin(a);
				ring.outer[idx] = {outerCenters[corner].x + ca * outerR,
								   outerCenters[corner].y + sa * outerR};
				ring.inner[idx] = {innerCenters[corner].x + ca * innerR,
								   innerCenters[corner].y + sa * innerR};
				++idx;
			}
		}
		return ring;
	}

	/** \brief Build a solid-color axis-aligned quad. */
	inline QuadMesh buildQuad(Rect r, Color c)
	{
		QuadMesh m;
		if (c.a == 0 || r.width <= 0.0f || r.height <= 0.0f)
			return m;

		constexpr Vec2 uv{0.5f, 0.5f};
		m.pushVertex({r.x, r.y}, uv, c);
		m.pushVertex({r.x + r.width, r.y}, uv, c);
		m.pushVertex({r.x + r.width, r.y + r.height}, uv, c);
		m.pushVertex({r.x, r.y + r.height}, uv, c);
		m.pushQuadIndices(0, 1, 2, 3);
		return m;
	}

	/** \brief Build a textured quad mapping pixel sub-rect `src` onto logical `dst`. */
	inline QuadMesh buildTexturedQuad(TextureHandle t, Rect src, Rect dst, Color tint)
	{
		QuadMesh m;
		if (!t.valid() || t.width <= 0 || t.height <= 0 ||
			tint.a == 0 || dst.width <= 0.0f || dst.height <= 0.0f)
			return m;

		const float invW = 1.0f / static_cast<float>(t.width);
		const float invH = 1.0f / static_cast<float>(t.height);
		const float u0 = src.x * invW;
		const float v0 = src.y * invH;
		const float u1 = (src.x + src.width) * invW;
		const float v1 = (src.y + src.height) * invH;

		m.pushVertex({dst.x, dst.y}, {u0, v0}, tint);
		m.pushVertex({dst.x + dst.width, dst.y}, {u1, v0}, tint);
		m.pushVertex({dst.x + dst.width, dst.y + dst.height}, {u1, v1}, tint);
		m.pushVertex({dst.x, dst.y + dst.height}, {u0, v1}, tint);
		m.pushQuadIndices(0, 1, 2, 3);
		return m;
	}

	/** \brief Build a 9-slice mesh (up to 9 quads, single indexed draw call). */
	inline NineSliceMesh buildNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint)
	{
		NineSliceMesh m;
		if (!t.valid() || t.width <= 0 || t.height <= 0 ||
			tint.a == 0 || dst.width <= 0.0f || dst.height <= 0.0f)
			return m;

		const float tw = static_cast<float>(t.width);
		const float th = static_cast<float>(t.height);

		const float sl = std::clamp(static_cast<float>(s.left), 0.0f, tw);
		const float sr = std::clamp(static_cast<float>(s.right), 0.0f, tw - sl);
		const float st = std::clamp(static_cast<float>(s.top), 0.0f, th);
		const float sb = std::clamp(static_cast<float>(s.bottom), 0.0f, th - st);

		float dl = sl, dr = sr;
		if (dl + dr > dst.width && (dl + dr) > 0.0f)
		{
			const float k = dst.width / (dl + dr);
			dl *= k;
			dr *= k;
		}
		float dt = st, db = sb;
		if (dt + db > dst.height && (dt + db) > 0.0f)
		{
			const float k = dst.height / (dt + db);
			dt *= k;
			db *= k;
		}

		const float xs[4] = {dst.x, dst.x + dl, dst.x + dst.width - dr, dst.x + dst.width};
		const float ys[4] = {dst.y, dst.y + dt, dst.y + dst.height - db, dst.y + dst.height};
		const float us[4] = {0.0f, sl / tw, (tw - sr) / tw, 1.0f};
		const float vs[4] = {0.0f, st / th, (th - sb) / th, 1.0f};

		for (int row = 0; row < 4; ++row)
			for (int col = 0; col < 4; ++col)
				m.pushVertex({xs[col], ys[row]}, {us[col], vs[row]}, tint);

		for (int row = 0; row < 3; ++row)
		{
			if (ys[row + 1] <= ys[row] || vs[row + 1] <= vs[row])
				continue;
			for (int col = 0; col < 3; ++col)
			{
				if (xs[col + 1] <= xs[col] || us[col + 1] <= us[col])
					continue;
				const uint32_t tl = static_cast<uint32_t>(row * 4 + col);
				m.pushQuadIndices(tl, tl + 1, tl + 5, tl + 4);
			}
		}
		return m;
	}

	/** \brief Build a CSS border-box rectangular stroke strictly inside `r` without corner overdraw. */
	inline RectStrokeMesh buildRectStroke(Rect r, float thickness, Color c)
	{
		RectStrokeMesh m;
		if (c.a == 0 || thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f)
			return m;

		constexpr Vec2 uv{0.5f, 0.5f};
		const float maxT = std::min(r.width, r.height) * 0.5f;
		const float t = std::min(thickness, maxT);

		if (t >= maxT - 0.01f)
		{
			m.pushVertex({r.x, r.y}, uv, c);
			m.pushVertex({r.x + r.width, r.y}, uv, c);
			m.pushVertex({r.x + r.width, r.y + r.height}, uv, c);
			m.pushVertex({r.x, r.y + r.height}, uv, c);
			m.pushQuadIndices(0, 1, 2, 3);
			return m;
		}

		// Outer loop: 0=TL, 1=TR, 2=BR, 3=BL
		m.pushVertex({r.x, r.y}, uv, c);
		m.pushVertex({r.x + r.width, r.y}, uv, c);
		m.pushVertex({r.x + r.width, r.y + r.height}, uv, c);
		m.pushVertex({r.x, r.y + r.height}, uv, c);

		// Inner loop: 4=iTL, 5=iTR, 6=iBR, 7=iBL
		m.pushVertex({r.x + t, r.y + t}, uv, c);
		m.pushVertex({r.x + r.width - t, r.y + t}, uv, c);
		m.pushVertex({r.x + r.width - t, r.y + r.height - t}, uv, c);
		m.pushVertex({r.x + t, r.y + r.height - t}, uv, c);

		for (uint32_t k = 0; k < 4; ++k)
		{
			const uint32_t next = (k + 1) & 3u;
			m.pushTri(k, 4u + k, 4u + next);
			m.pushTri(k, 4u + next, next);
		}
		return m;
	}

	/** \brief Build per-edge rectangular borders in a single indexed draw call. */
	inline BorderMesh buildBorder(Rect r, EdgeInsets sides, Color c)
	{
		BorderMesh m;
		if (c.a == 0 || r.width <= 0.0f || r.height <= 0.0f)
			return m;

		constexpr Vec2 uv{0.5f, 0.5f};
		auto addRect = [&](float x, float y, float w, float h)
		{
			if (w <= 0.0f || h <= 0.0f)
				return;
			const uint32_t base = m.vertexCount;
			m.pushVertex({x, y}, uv, c);
			m.pushVertex({x + w, y}, uv, c);
			m.pushVertex({x + w, y + h}, uv, c);
			m.pushVertex({x, y + h}, uv, c);
			m.pushQuadIndices(base, base + 1, base + 2, base + 3);
		};

		if (sides.top > 0.0f)
			addRect(r.x, r.y, r.width, std::min(sides.top, r.height));
		if (sides.bottom > 0.0f)
		{
			const float bh = std::min(sides.bottom, r.height);
			addRect(r.x, r.y + r.height - bh, r.width, bh);
		}
		const float innerH = r.height - sides.top - sides.bottom;
		if (innerH > 0.0f)
		{
			if (sides.left > 0.0f)
				addRect(r.x, r.y + sides.top, std::min(sides.left, r.width), innerH);
			if (sides.right > 0.0f)
			{
				const float rw = std::min(sides.right, r.width);
				addRect(r.x + r.width - rw, r.y + sides.top, rw, innerH);
			}
		}
		return m;
	}

	/** \brief Build a filled rounded rectangle (fast-paths to a 4-vertex quad when `radiusPx <= 0.01`). */
	inline RoundedFillMesh buildRoundedRectFill(Rect r, float radiusPx, Color c)
	{
		RoundedFillMesh m;
		if (c.a == 0 || r.width <= 0.0f || r.height <= 0.0f)
			return m;

		constexpr Vec2 uv{0.5f, 0.5f};
		const float maxR = std::min(r.width, r.height) * 0.5f;
		const float rad = std::clamp(radiusPx, 0.0f, maxR);

		if (rad <= 0.01f)
		{
			m.pushVertex({r.x, r.y}, uv, c);
			m.pushVertex({r.x + r.width, r.y}, uv, c);
			m.pushVertex({r.x + r.width, r.y + r.height}, uv, c);
			m.pushVertex({r.x, r.y + r.height}, uv, c);
			m.pushQuadIndices(0, 1, 2, 3);
			return m;
		}

		// Vertex 0: center of fan
		m.pushVertex({r.x + r.width * 0.5f, r.y + r.height * 0.5f}, uv, c);

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);
		constexpr float kStartAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};

		const Vec2 centers[4] = {
			{r.x + r.width - rad, r.y + rad},
			{r.x + r.width - rad, r.y + r.height - rad},
			{r.x + rad,           r.y + r.height - rad},
			{r.x + rad,           r.y + rad}};

		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				const float a = kStartAngles[corner] + kStep * static_cast<float>(s);
				m.pushVertex({centers[corner].x + std::cos(a) * rad,
							  centers[corner].y + std::sin(a) * rad},
							 uv, c);
			}
		}

		constexpr uint32_t ringPts = static_cast<uint32_t>(kRoundedRingPoints);
		for (uint32_t i = 0; i < ringPts; ++i)
		{
			const uint32_t cur = 1u + i;
			const uint32_t nxt = 1u + ((i + 1u) % ringPts);
			m.pushTri(0u, nxt, cur);
		}
		return m;
	}

	/** \brief Build a CSS border-box rounded stroke mesh strictly inside `r`. */
	inline RoundedStrokeMesh buildRoundedRectStroke(Rect r, float radiusPx, float thickness, Color c)
	{
		RoundedStrokeMesh m;
		if (c.a == 0)
			return m;

		const auto ring = buildRoundedBorderRing(r, radiusPx, thickness);
		if (ring.empty)
			return m;

		if (ring.isSolidFill)
		{
			const auto fill = buildRoundedRectFill(r, radiusPx, c);
			std::copy_n(fill.vertices.begin(), fill.vertexCount, m.vertices.begin());
			std::copy_n(fill.indices.begin(), fill.indexCount, m.indices.begin());
			m.vertexCount = fill.vertexCount;
			m.indexCount = fill.indexCount;
			return m;
		}

		constexpr Vec2 uv{0.5f, 0.5f};
		constexpr uint32_t N = static_cast<uint32_t>(kRoundedRingPoints);

		for (uint32_t i = 0; i < N; ++i)
			m.pushVertex(ring.outer[i], uv, c);
		for (uint32_t i = 0; i < N; ++i)
			m.pushVertex(ring.inner[i], uv, c);

		for (uint32_t i = 0; i < N; ++i)
		{
			const uint32_t next = (i + 1u) % N;
			m.pushTri(i, N + next, next);
			m.pushTri(i, N + i, N + next);
		}
		return m;
	}

	/** \brief Build a filled circle with adaptive segment count (`16..kCircleMaxSegments`). */
	inline CircleMesh buildCircle(Vec2 center, float radius, Color c)
	{
		CircleMesh m;
		if (c.a == 0 || radius <= 0.0f)
			return m;

		constexpr Vec2 uv{0.5f, 0.5f};
		constexpr float kTwoPi = 6.2831853071795865f;
		const int segs = std::clamp(static_cast<int>(std::ceil(radius * 0.75f)) + 12,
									16, kCircleMaxSegments);
		const float step = kTwoPi / static_cast<float>(segs);

		m.pushVertex(center, uv, c);
		for (int i = 0; i < segs; ++i)
		{
			const float a = step * static_cast<float>(i);
			m.pushVertex({center.x + std::cos(a) * radius,
						  center.y + std::sin(a) * radius},
						 uv, c);
		}

		const uint32_t uSegs = static_cast<uint32_t>(segs);
		for (uint32_t i = 0; i < uSegs; ++i)
		{
			const uint32_t cur = 1u + i;
			const uint32_t nxt = 1u + ((i + 1u) % uSegs);
			m.pushTri(0u, nxt, cur);
		}
		return m;
	}

	/** @} */
} // namespace ZenitUI::Geometry2D