#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  INPUT
//  Macchina a stati (Idle/Hover/Pressed/Disabled), callback di click,
//  gestione tastiera (Enter/Space per attivare), focus sui discendenti.
// =========================================================================

void Layout::updateInteractionFlags(bool selfBlocked)
{
	auto &ctx = UIContext::get();
	auto &pointer = ctx.pointer;

	isHovered = false;
	if (!selfBlocked && rect.width > 0 && rect.height > 0)
		isHovered = rect.contains(pointer.pos);

	isPressed = false;
	if (pointer.down && ctx.pressTarget)
	{
		Layout *n = ctx.pressTarget;
		while (n)
		{
			if (n == this)
			{
				isPressed = true;
				break;
			}
			if (!n->getPassThrough())
				break;
			n = n->getParent().get();
		}
	}

	bool focusNow = ctx.hasFocus(this);
	if (focusNow != isFocused)
	{
		isFocused = focusNow;
		pendingTransition = true;
	}
}

UIState Layout::computeNextState() const
{
	if (!isEnabled)
		return UIState::Disabled;
	if (isPressed)
		return UIState::Pressed;
	if (isHovered)
		return UIState::Hover;
	return UIState::Idle;
}

void Layout::fireInteractionCallbacks(UIState prevState, UIState nextState, bool stateChanged)
{
	auto &ctx = UIContext::get();
	auto &pointer = ctx.pointer;

	if (stateChanged)
	{
		if (nextState == UIState::Hover && prevState != UIState::Hover && onHoverEnter)
			onHoverEnter();
		else if (prevState == UIState::Hover && nextState != UIState::Hover && onHoverExit)
			onHoverExit();
	}

	if (pointer.pressed && !ctx.clickConsumed)
	{
		bool inPressPath = (ctx.pressTarget == this) || isAncestorOf(ctx.pressTarget);
		if (inPressPath)
		{
			if (onPress)
				onPress();
			if (!passThrough_)
				ctx.consumeClick();
		}
	}

	if (pointer.released)
	{
		bool pressedHere = (ctx.pressTarget == this) || isAncestorOf(ctx.pressTarget);
		bool releasedHere = (ctx.releaseTarget == this) || isAncestorOf(ctx.releaseTarget);
		bool validClick = pressedHere && releasedHere;

		if (validClick)
		{
			if (onRelease)
				onRelease();

			if (onClick && !ctx.clickConsumed)
			{
				onClick();
				if (!passThrough_)
					ctx.consumeClick();
			}
		}
	}

	if (pointer.rightPressed && isHovered && onRightClick)
	{
		if (!ctx.rightClickConsumed)
		{
			onRightClick();
			if (!passThrough_)
				ctx.consumeRightClick();
		}
	}
}

void Layout::handleFocusInput()
{
	if (!isFocused || !isEnabled || !keyboardActivates_)
		return;

	auto &ev = UIContext::get().inputEvents;
	for (int k : ev.keys)
	{
		if (k == Key::Enter || k == Key::Space)
		{
			if (onClick)
				onClick();
		}
	}
}

} // namespace ZenitUI