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

				// ---- Click-based buttons: Left and Middle ----
		//
		// These follow the standard click model: press, then release on
		// the same target. The legacy onPress/onRelease/onClick callbacks
		// fire only for Left (they're the historical API). The unified
		// onButtonPress/onButtonRelease/onButtonClick fire for every
		// button that uses this model, currently Left and Middle.
		//
		// Both legacy and unified callbacks can be set at the same time.
		// If they are, both fire — the user gets two callbacks for the
		// same event, which is what they asked for by setting both.
		auto fireClickButton = [&](MouseButton button, bool isLeftLegacy)
		{
			const auto &btn = pointer[button];

			// --- Press ---
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

			// --- Release + Click ---
			if (btn.released)
			{
				bool pressedHere  = (ctx.pressTarget == &node)   ||
									node.isAncestorOf(ctx.pressTarget);
				bool releasedHere = (ctx.releaseTarget == &node) ||
									node.isAncestorOf(ctx.releaseTarget);
				bool validClick   = pressedHere && releasedHere;

				if (validClick)
				{
					// onRelease fires unconditionally when the click is
					// valid (matches the legacy behavior: release is a
					// lower-level event than click).
					if (isLeftLegacy && node.onRelease)
						node.onRelease();
					if (node.onButtonRelease)
						node.onButtonRelease(button);

					// onClick / onButtonClick are gated by consumption:
					// if a descendant already consumed the event, they
					// don't fire here.
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

		// ---- Right button: onRightClick ----
		//
		// Semantic is intentionally different from Left/Middle: fires
		// on press if the node is hovered, without validating a full
		// click pair. This matches the historical "right-click" model
		// (one-shot, typically for context menus).
		//
		// onButton* callbacks do NOT fire for Right. If a future design
		// wants to unify right-click under the same model, that's a
		// separate decision — Right's "one-shot on press" semantic has
		// legitimate uses that would be lost by adopting the click-pair
		// model.
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