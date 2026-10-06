#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{

// =========================================================================
//  INPUT (InputController)
//  Macchina a stati (Idle/Hover/Pressed/Disabled), callback di click,
//  gestione tastiera, focus. Nessuno stato proprio: opera su Layout.
// =========================================================================

void InputController::updateFlags(Layout &node, bool selfBlocked, bool scrolling)
{
	auto &ctx = UIContext::get();
	auto &pointer = ctx.pointer;

	if (scrolling)
	{
		// Durante lo scroll congeliamo l'hover: evita di marcare dirty
		// ogni bottone attraversato dal mouse mentre la lista scorre.
		bool focusNow = ctx.hasFocus(&node);
		if (focusNow != node.isFocused)
		{
			node.isFocused = focusNow;
			node.pendingTransition = true;
		}
		return;
	}

	node.isHovered = false;
	if (!selfBlocked && node.rect.width > 0 && node.rect.height > 0)
		node.isHovered = node.rect.contains(pointer.pos);

	node.isPressed = false;
	if (pointer.down && ctx.pressTarget)
	{
		Layout *n = ctx.pressTarget;
		while (n)
		{
			if (n == &node) { node.isPressed = true; break; }
			if (!n->getPassThrough()) break;
			n = n->getParent().get();
		}
	}

	bool focusNow = ctx.hasFocus(&node);
	if (focusNow != node.isFocused)
	{
		node.isFocused = focusNow;
		node.pendingTransition = true;
	}
}

UIState InputController::computeNextState(const Layout &node) const
{
	if (!node.isEnabled)  return UIState::Disabled;
	if (node.isPressed)   return UIState::Pressed;
	if (node.isHovered)   return UIState::Hover;
	return UIState::Idle;
}

void InputController::fireCallbacks(Layout &node, UIState prevState, UIState nextState, bool stateChanged)
{
	auto &ctx = UIContext::get();
	auto &pointer = ctx.pointer;

	if (stateChanged)
	{
		if (nextState == UIState::Hover && prevState != UIState::Hover && node.onHoverEnter)
			node.onHoverEnter();
		else if (prevState == UIState::Hover && nextState != UIState::Hover && node.onHoverExit)
			node.onHoverExit();
	}

	if (pointer.pressed && !ctx.clickConsumed)
	{
		bool inPressPath = (ctx.pressTarget == &node) || node.isAncestorOf(ctx.pressTarget);
		if (inPressPath)
		{
			if (node.onPress) node.onPress();
			if (!node.passThrough_) ctx.consumeClick();
		}
	}

	if (pointer.released)
	{
		bool pressedHere  = (ctx.pressTarget   == &node) || node.isAncestorOf(ctx.pressTarget);
		bool releasedHere = (ctx.releaseTarget == &node) || node.isAncestorOf(ctx.releaseTarget);
		bool validClick   = pressedHere && releasedHere;

		if (validClick)
		{
			if (node.onRelease) node.onRelease();
			if (node.onClick && !ctx.clickConsumed)
			{
				node.onClick();
				if (!node.passThrough_) ctx.consumeClick();
			}
		}
	}

	if (pointer.rightPressed && node.isHovered && node.onRightClick)
	{
		if (!ctx.rightClickConsumed)
		{
			node.onRightClick();
			if (!node.passThrough_) ctx.consumeRightClick();
		}
	}
}

void InputController::handleKeyInput(Layout &node)
{
	if (!node.isFocused || !node.isEnabled || !node.keyboardActivates_)
		return;

	auto &ev = UIContext::get().inputEvents;
	for (int k : ev.keys)
	{
		if (k == Key::Enter || k == Key::Space)
		{
			if (node.onClick) node.onClick();
		}
	}
}

} // namespace ZenitUI