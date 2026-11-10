#pragma once

/**
 * \file Dropdown.hpp
 * \brief A select-style dropdown with a popup list.
 *
 * The dropdown is a `Vertical` container with two children:
 *  1. A `Button` (class `"dropdown-trigger"`) whose label shows the currently
 *     selected option.
 *  2. A `ScrollView` (portal) that contains one `Label` (class
 *     `"dropdown-item"`) per option.
 *
 * The list is always a portal, so it draws above everything else and is not
 * clipped by ancestors. It is shown / hidden by toggling its `enabled` flag
 * and opacity, and its size is computed dynamically in `arrange` from the
 * available space below the trigger, with automatic flip above if there is
 * not enough room.
 *
 * Interaction:
 *  - Click on the trigger toggles the list.
 *  - Click on an item selects it and closes the list.
 *  - Click outside the trigger and list closes the list.
 *  - Escape closes the list when the dropdown has focus.
 *  - Up / Down navigate options when the dropdown has focus and the list is
 *    open; Up also opens the list from the closed state.
 *  - Enter / Space toggle the list.
 *
 * \note `setSelected` fires `onChange` **even when the index is unchanged**.
 *       This differs from the usual "only fire on change" convention but is
 *       relied upon by some tests (`Dropdown_basic.setSelected_same_value_fires_onChange`).
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "ScrollView.hpp"

namespace ZenitUI::UI
{
    /**
     * \class Dropdown
     * \brief A single-select dropdown.
     *
     * Style tag: `"Dropdown"`. Focusable, keyboard-activatable. The list is
     * a portal and stays out of the parent's layout flow.
     */
    class Dropdown : public TLayout<Dropdown>
    {
    public:
        /**
         * \brief Construct with options and an initial selection.
         * \param options  List of option strings.
         * \param selected Initially selected index. Values outside the valid
         *                 range are clamped to `0` on the first `setSelected`
         *                 call — the constructor itself does not validate.
         */
        Dropdown(std::vector<std::string> options, int selected = 0)
            : TLayout<Dropdown>(LayoutType::Vertical),
              options(std::move(options)), selected(selected) {}

        /** \brief Currently selected index. */
        int getSelected() const { return selected; }

        /**
         * \brief Select the option at \p idx and close the list.
         *
         * Out-of-range indices are ignored. Always fires `onChange` on
         * success, even if the index was already selected. Marks the style
         * as pending so the trigger label and item check state update on the
         * next frame.
         */
        void setSelected(int idx)
        {
            if (idx < 0 || idx >= (int)options.size())
                return;
            selected = idx;
            refreshButtonText();
            updateListStyles();
            if (onChange)
                onChange(selected, options[selected]);
            internal_.pendingTransition = 1;
        }

        /** \brief `true` while the option list is visible. */
        bool getOpen() const { return isOpen; }

        /**
         * \brief Open or close the option list.
         *
         * No-op if \p o matches the current state. Fires `onOpenChanged`.
         */
        void setOpen(bool o)
        {
            if (isOpen == o)
                return;
            isOpen = o;
            applyOpenState();
            if (onOpenChanged)
                onOpenChanged(isOpen);
        }

        /** \brief Fired when an item is selected. Second argument is the option text. */
        std::function<void(int, const std::string &)> onChange;
        /** \brief Fired when the list is opened or closed. */
        std::function<void(bool)> onOpenChanged;

    protected:
        /**
         * \brief Close the list when the dropdown is disabled.
         */
        void onEnabledChanged(bool nowEnabled) override
        {
            if (!nowEnabled && isOpen)
            {
                isOpen = false;
                applyOpenState();
                if (onOpenChanged)
                    onOpenChanged(false);
            }
        }

        /**
         * \brief Keyboard navigation and close-on-outside-click.
         *
         * Keyboard handling only runs when the dropdown has focus. The
         * outside-click handler checks both the trigger rect and the list
         * rect, and closes the dropdown on a press outside both.
         */
        void onUpdate(float) override
        {
            if (isOpen)
                internal_.pendingTransition = 1;

            auto &ctx = UIContext::get();

            if (ctx.hasFocus(this))
            {
                for (int k : ctx.inputEvents.keys)
                {
                    if (k == Key::Escape)
                    {
                        if (isOpen)
                            setOpen(false);
                    }
                    else if (k == Key::Down)
                    {
                        if (!isOpen)
                        {
                            setOpen(true);
                        }
                        else
                        {
                            int next = std::min(selected + 1, (int)options.size() - 1);
                            if (next != selected)
                            {
                                setSelected(next);
                                if (listContainer && next < (int)listContainer->children.size())
                                    listContainer->children[next]->scrollIntoView();
                            }
                        }
                    }
                    else if (k == Key::Up)
                    {
                        if (isOpen)
                        {
                            int prev = std::max(selected - 1, 0);
                            if (prev != selected)
                            {
                                setSelected(prev);
                                if (listContainer && prev < (int)listContainer->children.size())
                                    listContainer->children[prev]->scrollIntoView();
                            }
                        }
                    }
                    else if (k == Key::Enter || k == Key::Space)
                    {
                        setOpen(!isOpen);
                        if (isOpen && listContainer && selected < (int)listContainer->children.size())
                            listContainer->children[selected]->scrollIntoView();
                    }
                }
            }

            if (!isOpen)
                return;

            if (ctx.pointer[MouseButton::Left].pressed || ctx.pointer[MouseButton::Right].pressed)
            {
                if (!rect.contains(ctx.pointer.pos) &&
                    !listContainer->getRect().contains(ctx.pointer.pos))
                {
                    setOpen(false);
                }
            }
        }

        /**
         * \brief Build the trigger button and the option list.
         *
         * Wires:
         *  - the trigger's `onClick` to toggle the list;
         *  - each item's `onClick` to select the item and close the list.
         *
         * The list starts closed with `enabled = false` and `opacity = 0`.
         */
        void onBuild() override
        {
            setStyleTag("Dropdown");
            setInteractive(true);
            setFocusable(true);
            internal_.pendingTransition = 1;
            getInlineBase().position = Position::Relative;
            getInlineBase().zIndex = ZIndex(10);

            button = Btn("", [this]()
                         {
                UIContext::get().requestFocus(shared_from_this());
                toggleOpen(); });
            button->cls("dropdown-trigger");
            button->setFocusable(false);

            button->getInlineBase().alignH = Align::Stretch;
            button->getInlineDefaults().minWidth = Px(120.0f);
            button->getInlineDefaults().height = VH(3.0f);
            button->getInlineDefaults().background = Color{40, 40, 45, 255};
            button->getInlineDefaults().radius = Px(6.0f);
            button->getInlineDefaults().borderColor = Color{70, 70, 80, 255};
            button->getInlineDefaults().borderWidth = Px(1.0f);
            button->getInlineDefaults().alignItems = Align::Center;
            addChild(button);

            listContainer = std::make_shared<ScrollView>();
            listContainer->getInlineBase().position = Position::Absolute;
            listContainer->setPortal(true);

            listContainer->getInlineDefaults().width = VW(15.0f);
            int visibleRows = std::min((int)options.size(), 6);
            listContainer->getInlineDefaults().height = VH(2.0f + visibleRows * 4.0f);
            listContainer->getInlineDefaults().background = Color{30, 30, 36, 255};
            listContainer->getInlineDefaults().borderColor = Color{60, 60, 70, 255};
            listContainer->getInlineDefaults().borderWidth = Px(1.0f);
            listContainer->getInlineDefaults().alignItems = Align::Stretch;
            listContainer->getInlineDefaults().overflowX = Overflow::Auto;
            listContainer->getInlineDefaults().overflowY = Overflow::Auto;

            for (size_t i = 0; i < options.size(); ++i)
            {
                int idx = (int)i;
                auto item = Label(options[i]);
                item->cls("dropdown-item");
                item->getInlineBase().padding = Spacing(VH(1.0f), VW(1.0f));
                item->getInlineBase().verticalAlign = Align::Center;
                item->setInteractive(true);
                item->setFocusable(false);

                item->onClick = [this, idx]()
                {
                    UIContext::get().requestFocus(shared_from_this());
                    setSelected(idx);
                    setOpen(false);
                };
                listContainer->addChild(item);
            }
            addChild(listContainer);
            isOpen = false;
            applyOpenState();
            refreshButtonText();
            updateListStyles();
        }

        /**
         * \brief Position the option list under (or above) the trigger.
         *
         * The list is a portal: its position is computed from the trigger's
         * screen-space rect adjusted for any scrolling ancestor's scroll
         * offset. If the list does not fit below the trigger, it flips above;
         * if it does not fit above either, it is clamped to the larger of the
         * two spaces.
         *
         * \note Because the list is a portal, its coordinates are absolute
         *       (screen space). This is why the scroll adjustment is needed:
         *       the trigger's rect is in content space, and the ancestor's
         *       scroll offset must be subtracted to get screen space.
         */
        void arrange(Rect space) override
        {
            TLayout<Dropdown>::arrange(space);
            if (!button || !listContainer)
                return;

            Rect btnRect = button->getRect();

            Layout *clipAncestor = nullptr;
            Layout *p = getParent().get();
            while (p)
            {
                const auto &st = p->getStyle();
                if (st.overflowX != Overflow::Visible ||
                    st.overflowY != Overflow::Visible)
                {
                    clipAncestor = p;
                    break;
                }
                p = p->getParent().get();
            }

            Rect clip;
            float scrollX = 0.0f;
            float scrollY = 0.0f;
            if (clipAncestor)
            {
                clip = clipAncestor->getRect();
                scrollX = clipAncestor->getScrollX();
                scrollY = clipAncestor->getScrollY();
            }
            else
            {
                clip = {0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
            }

            const float btnScreenX = btnRect.x - scrollX;
            const float btnScreenY = btnRect.y - scrollY;

            float listH = listContainer->getMeasuredSize().y;
            float desiredY = btnScreenY + btnRect.height + 4.0f;

            const float spaceBelow = (clip.y + clip.height) - desiredY;
            const float spaceAbove = btnScreenY - clip.y;

            if (listH <= spaceBelow)
            {
            }
            else if (listH <= spaceAbove)
            {
                desiredY = btnScreenY - listH - 4.0f;
            }
            else
            {
                if (spaceBelow >= spaceAbove)
                {
                    listH = std::max(20.0f, spaceBelow);
                }
                else
                {
                    listH = std::max(20.0f, spaceAbove);
                    desiredY = btnScreenY - listH - 4.0f;
                }
            }

            listContainer->arrange({btnScreenX, desiredY, btnRect.width, listH});
        }

    private:
        std::vector<std::string> options;
        int selected{0};
        bool isOpen{false};
        std::shared_ptr<Button> button;
        std::shared_ptr<ScrollView> listContainer;

        /** \brief Toggle the list state. */
        void toggleOpen() { setOpen(!isOpen); }

        /**
         * \brief Apply the current open state to the list's visibility.
         *
         * Uses `enabled` (which also blocks input) plus `opacity` (which
         * enables the fade transition). Marks the style as pending.
         */
        void applyOpenState()
        {
            listContainer->setEnabled(isOpen);
            listContainer->setInteractive(isOpen);
            listContainer->getInlineBase().opacity = isOpen ? 1.0f : 0.0f;
            internal_.pendingTransition = 1;
        }

        /**
         * \brief Update the trigger label with the current selection.
         *
         * No-op if the selection is out of range or if the button has no
         * children (e.g. before `onBuild`).
         */
        void refreshButtonText()
        {
            if (selected < 0 || selected >= (int)options.size())
                return;
            if (button->children.empty())
                return;
            auto t = std::dynamic_pointer_cast<Text>(button->children.front());
            if (t)
                t->setText(options[selected]);
        }

        /**
         * \brief Update each item's `:checked` state to match the selection.
         *
         * Used by themes to highlight the current option.
         */
        void updateListStyles()
        {
            if (!listContainer)
                return;
            for (size_t i = 0; i < listContainer->children.size(); ++i)
            {
                auto &item = listContainer->children[i];
                item->setChecked((int)i == selected);
            }
        }
    };
}