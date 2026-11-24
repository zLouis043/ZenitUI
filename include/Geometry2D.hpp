#pragma once

/**
 * \file Geometry2D.hpp
 * \brief Shared zero-allocation 2D tessellation helpers for `IRenderer` backends.
 */

#include "CoreTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace ZenitUI::Geometry2D
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	inline constexpr int kRoundedCornerSegments = 12;
	inline constexpr int kRoundedRingPoints = 4 * (kRoundedCornerSegments + 1);

	/**
	 * \struct RoundedBorderRing
	 * \brief Outer and inner vertex loops of a CSS border-box rounded stroke.
	 */
	struct RoundedBorderRing
	{
		std::array<Vec2, kRoundedRingPoints> outer{};
		std::array<Vec2, kRoundedRingPoints> inner{};
		bool empty{true};       ///< Degenerate rect or zero thickness — skip drawing.
		bool isSolidFill{false}; ///< Thickness covers the entire rect — delegate to `fillRoundedRect`.
	};

	/**
	 * \brief Build the inward-extruded rounded ring for `IRenderer::strokeRoundedRect`.
	 *
	 * Generates `kRoundedRingPoints` pairs in clockwise screen order (TR, BR, BL, TL).
	 * For each index `i` with `next = (i + 1) % kRoundedRingPoints`, emit the two
	 * counter-clockwise (Y-down screen space) triangles:
	 * `(outer[i], inner[next], outer[next])` and `(outer[i], inner[i], inner[next])`.
	 */
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

	/** @} */
} // namespace ZenitUI::Geometry2D