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
			if (focusNow != node.state.focused)
			{
				node.state.focused = focusNow;
				node.internal_.pendingTransition = 1;
			}
			return;
		}

		node.state.hovered = 0;
		if (!selfBlocked && node.rect.width > 0 && node.rect.height > 0)
			node.state.hovered = node.rect.contains(pointer.pos);

		node.state.pressed = 0;
		if (pointer.down && ctx.pressTarget)
		{
			Layout *n = ctx.pressTarget;
			while (n)
			{
				if (n == &node)
				{
					node.state.pressed = 1;
					break;
				}
				if (!n->getPassThrough())
					break;
				n = n->getParent().get();
			}
		}

		bool focusNow = ctx.hasFocus(&node);
		if (focusNow != node.state.focused)
		{
			node.state.focused = focusNow;
			node.internal_.pendingTransition = 1;
		}
	}

	UIState InputController::computeNextState(const Layout &node) const
	{
		if (!node.config.enabled)
			return UIState::Disabled;
		if (node.state.pressed)
			return UIState::Pressed;
		if (node.state.hovered)
			return UIState::Hover;
		return UIState::Idle;
	}

		void InputController::fireCallbacks(Layout &node, UIState prevState, UIState nextState, bool stateChanged)
	{
		auto &ctx = UIContext::get();
		auto &pointer = ctx.pointer;

		// ---- Hover transitions ----
		if (stateChanged)
		{
			bool wasHovered = (prevState == UIState::Hover || prevState == UIState::Pressed);
			bool isHovered = (nextState == UIState::Hover || nextState == UIState::Pressed);

			if (isHovered && !wasHovered && node.onHoverEnter)
				node.onHoverEnter();
			else if (!isHovered && wasHovered && node.onHoverExit)
				node.onHoverExit();
		}

		// ---- Left button: press / release / click ----
		//
		// The blocks below are per-button. Once the unified
		// onButtonClick(MouseButton) API lands (5.15d), these will be
		// wrapped in a loop over every button in the MouseButton enum.
		// For now they're spelled out for Left and Right only, which
		// matches the current public API (onClick / onPress / onRelease
		// for Left, onRightClick for Right).
		{
			const auto &btn = pointer[MouseButton::Left];

			if (btn.pressed && !ctx.wasConsumed(MouseButton::Left))
			{
				bool inPressPath = (ctx.pressTarget == &node) || node.isAncestorOf(ctx.pressTarget);
				if (inPressPath)
				{
					if (node.onPress)
						node.onPress();
					if (!node.config.passThrough)
						ctx.consumeClick(MouseButton::Left);
				}
			}

			if (btn.released)
			{
				bool pressedHere  = (ctx.pressTarget == &node)   || node.isAncestorOf(ctx.pressTarget);
				bool releasedHere = (ctx.releaseTarget == &node) || node.isAncestorOf(ctx.releaseTarget);
				bool validClick   = pressedHere && releasedHere;

				if (validClick)
				{
					if (node.onRelease)
						node.onRelease();
					if (node.onClick && !ctx.wasConsumed(MouseButton::Left))
					{
						node.onClick();
						if (!node.config.passThrough)
							ctx.consumeClick(MouseButton::Left);
					}
				}
			}
		}

		// ---- Right button: right-click ----
		{
			const auto &btn = pointer[MouseButton::Right];

			if (btn.pressed && node.state.hovered && node.onRightClick)
			{
				if (!ctx.wasConsumed(MouseButton::Right))
				{
					node.onRightClick();
					if (!node.config.passThrough)
						ctx.consumeClick(MouseButton::Right);
				}
			}
		}
	}

	void InputController::handleKeyInput(Layout &node)
	{
		if (!node.state.focused || !node.config.enabled || !node.config.keyboardActivates)
			return;

		auto &ev = UIContext::get().inputEvents;
		for (int k : ev.keys)
		{
			if (k == Key::Enter || k == Key::Space)
			{
				if (node.onClick)
					node.onClick();
			}
		}
	}

} // namespace ZenitUI