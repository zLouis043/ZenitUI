#pragma once

/**
 * \file Button.hpp
 * \brief A clickable button with an optional text label.
 *
 * The visual is delegated to the base `Panel` (background, border, radius,
 * shadow all come from `Panel` / `Button` theme rules). The button only adds:
 *  - interactive + focusable + keyboard-activatable configuration;
 *  - a single `Text` child whose content can be queried / set via
 *    `text()` / `setText()`.
 *
 * \note The text child is the *first* child if it is a `Text` instance.
 *       `setText` will not touch a non-Text first child; it will add a new
 *       `Label` at the end instead. This means adding an icon as the first
 *       child will silently break `setText` updates — set the label first
 *       (as `Btn` does), or use the `Button::setText` return value to check
 *       for success.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Text.hpp"
#include "Layouts.hpp"

namespace ZenitUI::UI {

/**
 * \class Button
 * \brief A push button.
 *
 * Style tag: `"Button"`. Inherits from `Panel` so it shares its raycast and
 * chrome behaviour. Emitted `:pressed` / `:hover` / `:focused` states match
 * the underlying `InputController` lifecycle.
 *
 * The button is keyboard-activatable: `Enter` and `Space` fire `onClick`
 * when the button has focus.
 */
class Button : public TLayout<Button, Panel>
{
public:
    /**
     * \brief Construct a button with the given click handler.
     * \param cb Handler fired on click. May be empty; `setText` can still be
     *           used to add a label later.
     */
    Button(std::function<void()> cb)
    {
        onClick = std::move(cb);
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(true);
        setStyleTag("Button");
        getInlineDefaults().alignItems = Align::Center;
    }

    /**
     * \brief Return the current label text, or `""` if there is none.
     *
     * Looks at the first child only. Returns empty when the first child is
     * not a `Text` (e.g. after `setText` was blocked by a non-Text child).
     */
    std::string text() const
    {
        auto t = labelChild();
        return t ? t->getText() : std::string{};
    }

    /**
     * \brief Update the label text.
     *
     * If the first child is a `Text`, replaces its content. Otherwise, adds a
     * new `Text` labeled with the given string as the last child. Note that
     * adding a label after a non-Text child will make `text()` (which only
     * inspects the first child) return empty — this is a known wart of the
     * current implementation; `Btn` sets the label first so it is not
     * usually a problem in practice.
     */
    void setText(const std::string& s)
    {
        if (auto t = labelChild())
        {
            t->setText(s);
            return;
        }
        addChild(Label(s)->cls("btn-text"));
    }

private:
    /** \brief The first child, if it is a `Text`; `nullptr` otherwise. */
    std::shared_ptr<Text> labelChild() const
    {
        if (children.empty())
            return nullptr;
        return std::dynamic_pointer_cast<Text>(children.front());
    }
};

/**
 * \brief Convenience factory: a `Button` with a `Text` label and a click handler.
 *
 * \param text Label to display. Stored as the first child with class
 *             `"btn-text"` so theme rules can target it.
 * \param cb   Optional click handler.
 * \return The constructed button.
 */
inline std::shared_ptr<Button> Btn(std::string text, std::function<void()> cb = nullptr)
{
    auto b = Button::create(std::move(cb));
    b->addChild(Label(text)->cls("btn-text"));
    return b;
}

} // namespace ZenitUI::UI