#pragma once

/**
 * \file Toggle.hpp
 * \brief A boolean on/off switch.
 *
 * The visual is a `Layout` container with a single child `Layout` whose
 * class is `"toggle-knob"`. The knob's position is driven entirely by CSS
 * through the `Toggle:checked .toggle-knob` selector; there is no
 * C++-side layout logic for the knob.
 *
 * \note Unlike `Checkbox`, `Toggle` calls `setChecked` in the constructor and
 *       on click. This makes the `:checked` pseudo-class fire correctly for
 *       theme rules like `Toggle:checked .toggle-knob { left: 60px; }`.
 */

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

/**
 * \class Toggle
 * \brief A toggle switch.
 *
 * Style tag: `"Toggle"`. Emitted checked state: `:checked` when `isChecked`.
 * Keyboard: activated by Enter/Space (`setKeyboardActivates(true)`).
 */
class Toggle : public TLayout<Toggle>
{
public:
    /**
     * \brief Construct with an initial state.
     * \param state Initial checked state.
     */
    Toggle(bool state = false)
        : TLayout<Toggle>(LayoutType::Stack), isChecked(state)
    {
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(true);
        setStyleTag("Toggle");

        setChecked(state);

        onClick = [this]()
        {
            isChecked = !isChecked;
            setChecked(isChecked);
            if (onToggle)
                onToggle(isChecked);
        };
    }

    /** \brief Fired after the checked state changes, with the new value. */
    std::function<void(bool)> onToggle = nullptr;
    /** \brief Current checked state. Prefer `getChecked()` for consistency. */
    bool isChecked;

protected:
    /**
     * \brief Create the knob child.
     *
     * The knob is not interactive and does not block raycasts, so clicks on
     * the knob reach the Toggle and toggle it. This is what allows
     * `Toggle:checked .toggle-knob` to be a pure CSS effect.
     */
    void onBuild() override
    {
        knob = std::make_shared<Layout>(LayoutType::Stack);
        knob->cls("toggle-knob");
        knob->setInteractive(false);
        knob->setBlocksRaycast(false);
        addChild(knob);
    }

private:
    std::shared_ptr<Layout> knob;
};

} // namespace ZenitUI::UI