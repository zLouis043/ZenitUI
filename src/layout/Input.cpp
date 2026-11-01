#include "Layout.hpp"
#include "Debug.hpp"

namespace ZenitUI
{


	void InputController::updateFlags(Layout &node, bool selfBlocked, bool scrolling)
	{
		auto &ctx = UIContext::get();
		auto &pointer = ctx.pointer;

		if (scrolling)
		{
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
		if (pointer[MouseButton::Left].down && ctx.pressTarget)
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

		if (stateChanged)
		{
			bool wasHovered = (prevState == UIState::Hover || prevState == UIState::Pressed);
			bool isHovered = (nextState == UIState::Hover || nextState == UIState::Pressed);

			if (isHovered && !wasHovered && node.onHoverEnter)
				node.onHoverEnter();
			else if (!isHovered && wasHovered && node.onHoverExit)
				node.onHoverExit();
		}

		auto fireClickButton = [&](MouseButton button, bool isLeftLegacy)
		{
			const auto &btn = pointer[button];

			if (btn.pressed && !ctx.wasConsumed(button))
			{
				bool inPressPath = (ctx.pressTarget == &node) ||
								   node.isAncestorOf(ctx.pressTarget);
				if (inPressPath)
				{
					if (isLeftLegacy && node.onPress)
						node.onPress();
					if (node.onButtonPress)
						node.onButtonPress(button);
					if (!node.config.passThrough)
						ctx.consumeClick(button);
				}
			}

			if (btn.released)
			{
				bool pressedHere  = (ctx.pressTarget == &node)   ||
									node.isAncestorOf(ctx.pressTarget);
				bool releasedHere = (ctx.releaseTarget == &node) ||
									node.isAncestorOf(ctx.releaseTarget);
				bool validClick   = pressedHere && releasedHere;

				if (validClick)
				{
					if (isLeftLegacy && node.onRelease)
						node.onRelease();
					if (node.onButtonRelease)
						node.onButtonRelease(button);

					if (!ctx.wasConsumed(button))
					{
						if (isLeftLegacy && node.onClick)
							node.onClick();
						if (node.onButtonClick)
							node.onButtonClick(button);
						if (!node.config.passThrough)
							ctx.consumeClick(button);
					}
				}
			}
		};

		fireClickButton(MouseButton::Left,   /*isLeftLegacy=*/true);
		fireClickButton(MouseButton::Middle, /*isLeftLegacy=*/false);

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