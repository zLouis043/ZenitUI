#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  SCROLL
//  Comportamento per-asse: overflow-x/y indipendenti.
//   - scrollbar verticale sul bordo destro (se overflowY == Scroll/Auto)
//   - scrollbar orizzontale sul bordo inferiore (se overflowX == Scroll/Auto)
//   - drag, click sul track, wheel (shift = orizzontale), inerzia
//  Con Auto la scrollbar appare solo se maxScroll > 0.
// =========================================================================

static bool isBarVisible(Overflow o, float maxScroll)
{
	if (o == Overflow::Scroll) return true;   // sempre
	if (o == Overflow::Auto)   return maxScroll > 0.0f;
	return false;
}

void Layout::resetScrollIfOverflowChanged()
{
	if (style_.currentStyle.overflowX != style_.lastInherited.overflowX ||
		style_.currentStyle.overflowY != style_.lastInherited.overflowY)
	{
		scroll_.reset();
	}
}

void Layout::tickScrollInput()
{
	if (!acceptsScrollInput() || type == LayoutType::Stack)
		return;

	auto &ctx = UIContext::get();
	auto &pointer = ctx.pointer;

	const bool canScrollY = scroll_.maxScroll.y > 0.0f
	                     && style_.currentStyle.overflowY != Overflow::Hidden;
	const bool canScrollX = scroll_.maxScroll.x > 0.0f
	                     && style_.currentStyle.overflowX != Overflow::Hidden;

	// --- Inizio drag o salto alla posizione (click sul track) ---
	if (!hasPointerCapture())
	{
		if (pointer.pressed && ctx.topmostConsumer == this)
		{
			bool handled = false;

			if (canScrollY)
			{
				Rect thumb = getThumbRectImpl();
				if (thumb.contains(pointer.pos))
				{
					capturePointer();
					scroll_.dragStartMouseY  = pointer.pos.y;
					scroll_.dragStartOffsetY = scroll_.offset.y;
					scroll_.velocity = {0.0f, 0.0f};
					handled = true;
				}
			}
			if (!handled && canScrollX)
			{
				Rect thumb = getHThumbRectImpl();
				if (thumb.contains(pointer.pos))
				{
					capturePointer();
					scroll_.dragStartMouseX  = pointer.pos.x;
					scroll_.dragStartOffsetX = scroll_.offset.x;
					scroll_.velocity = {0.0f, 0.0f};
					handled = true;
				}
			}

			if (!handled && canScrollY && !canScrollX)
			{
				// click sul track verticale (a destra della thumb)
				float ratio = (rect.height > 0.0f)
								  ? ((pointer.pos.y - rect.y) / rect.height)
								  : 0.0f;
				scroll_.offset.y = std::clamp(
					ratio * scrollContentSize.y - rect.height * 0.5f,
					0.0f, scroll_.maxScroll.y);
			}
			else if (!handled && canScrollX && !canScrollY)
			{
				// click sul track orizzontale (sotto la thumb)
				float ratio = (rect.width > 0.0f)
								  ? ((pointer.pos.x - rect.x) / rect.width)
								  : 0.0f;
				scroll_.offset.x = std::clamp(
					ratio * scrollContentSize.x - rect.width * 0.5f,
					0.0f, scroll_.maxScroll.x);
			}
		}
	}
	// --- Continuazione del drag ---
	else if (pointer.down)
	{
		if (canScrollY)
		{
			float dy = pointer.pos.y - scroll_.dragStartMouseY;
			float ratio = (rect.height > 0.0f)
							  ? (scrollContentSize.y / rect.height)
							  : 1.0f;
			scroll_.offset.y = std::clamp(
				scroll_.dragStartOffsetY + dy * ratio,
				0.0f, scroll_.maxScroll.y);
		}
		if (canScrollX)
		{
			float dx = pointer.pos.x - scroll_.dragStartMouseX;
			float ratio = (rect.width > 0.0f)
							  ? (scrollContentSize.x / rect.width)
							  : 1.0f;
			scroll_.offset.x = std::clamp(
				scroll_.dragStartOffsetX + dx * ratio,
				0.0f, scroll_.maxScroll.x);
		}
	}

	// --- Wheel: shift = orizzontale, altrimenti verticale ---
	if (!ctx.wheelConsumedThisFrame && isInteractive &&
		!hasPointerCapture() && rect.contains(pointer.pos))
	{
		if (pointer.wheelY != 0.0f)
		{
			if (ctx.shiftHeld && canScrollX)
			{
				scroll_.velocity.x -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
				ctx.wheelConsumedThisFrame = true;
			}
			else if (!ctx.shiftHeld && canScrollY)
			{
				scroll_.velocity.y -= pointer.wheelY * ScrollState::WHEEL_IMPULSE;
				ctx.wheelConsumedThisFrame = true;
			}
		}
	}

	scroll_.tickInertia(UIContext::get().dt);
}

void Layout::drawScrollbar(float parentOpacity)
{
	const bool wantV = isBarVisible(style_.currentStyle.overflowY, scroll_.maxScroll.y);
	const bool wantH = isBarVisible(style_.currentStyle.overflowX, scroll_.maxScroll.x);
	if (!wantV && !wantH)
		return;

	auto r = UIContext::get().renderer;
	if (!r)
		return;

	float op = style_.currentStyle.opacity * parentOpacity;
	if (op <= 0.001f)
		return;

	const float MARGIN  = 4.0f;
	const float TRACK_W = 6.0f;

	r->pushClip(rect);

	// Quanto lo spazio "occupato" da una barra va sottratto all'altra.
	const float reservedV = wantV ? (TRACK_W + MARGIN) : 0.0f;
	const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

	if (wantV)
	{
		Rect track = {
			rect.x + rect.width - TRACK_W - MARGIN,
			rect.y + MARGIN,
			TRACK_W,
			rect.height - MARGIN * 2 - reservedH};
		r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

		Rect thumb = getThumbRectImpl();
		Color thumbColor = hasPointerCapture()
							   ? Color{180, 180, 180, 255}
							   : Color{130, 130, 130, 220};
		r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));
	}

	if (wantH)
	{
		Rect track = {
			rect.x + MARGIN,
			rect.y + rect.height - TRACK_W - MARGIN,
			rect.width - MARGIN * 2 - reservedV,
			TRACK_W};
		r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

		Rect thumb = getHThumbRectImpl();
		Color thumbColor = hasPointerCapture()
							   ? Color{180, 180, 180, 255}
							   : Color{130, 130, 130, 220};
		r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));
	}

	r->popClip();
}

Rect Layout::getThumbRectImpl() const
{
	const float TRACK_W = 6.0f;
	const float MARGIN  = 4.0f;

	const bool wantH = isBarVisible(style_.currentStyle.overflowX, scroll_.maxScroll.x);
	const float reservedH = wantH ? (TRACK_W + MARGIN) : 0.0f;

	float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f - reservedH);
	float contentH = scrollContentSize.y;
	float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
	float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio));
	float scrollRatio = (scroll_.maxScroll.y > 0.0f)
							? (scroll_.offset.y / scroll_.maxScroll.y)
							: 0.0f;
	float available = std::max(0.0f, trackH - thumbH);
	float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

	return {rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH};
}

Rect Layout::getHThumbRectImpl() const
{
	const float TRACK_H = 6.0f;
	const float MARGIN  = 4.0f;

	const bool wantV = isBarVisible(style_.currentStyle.overflowY, scroll_.maxScroll.y);
	const float reservedV = wantV ? (TRACK_H + MARGIN) : 0.0f;

	float trackW = std::max(0.0f, rect.width - MARGIN * 2.0f - reservedV);
	float contentW = scrollContentSize.x;
	float viewportRatio = (contentW > 0.0f) ? (rect.width / contentW) : 1.0f;
	float thumbW = std::min(trackW, std::max(30.0f, trackW * viewportRatio));
	float scrollRatio = (scroll_.maxScroll.x > 0.0f)
							? (scroll_.offset.x / scroll_.maxScroll.x)
							: 0.0f;
	float available = std::max(0.0f, trackW - thumbW);
	float thumbX = rect.x + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

	return {thumbX, rect.y + rect.height - TRACK_H - MARGIN, thumbW, TRACK_H};
}

} // namespace ZenitUI