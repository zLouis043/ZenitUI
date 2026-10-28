#pragma once
#include "Common.hpp"
#include "Layout.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "ScrollView.hpp"

namespace ZenitUI::UI
{
    class Dropdown : public TLayout<Dropdown>
    {
    public:
        Dropdown(std::vector<std::string> options, int selected = 0)
            : TLayout<Dropdown>(LayoutType::Vertical),
              options(std::move(options)), selected(selected) {}

        int getSelected() const { return selected; }

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

        bool getOpen() const { return isOpen; }

        void setOpen(bool o)
        {
            if (isOpen == o)
                return;
            isOpen = o;
            applyOpenState();
            if (onOpenChanged)
                onOpenChanged(isOpen);
        }

        std::function<void(int, const std::string &)> onChange;
        std::function<void(bool)> onOpenChanged;

    protected:
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

        void onUpdate(float) override
        {
            if (isOpen)
                internal_.pendingTransition = 1;

            auto &ctx = UIContext::get();

            // --- GESTIONE TASTIERA ---
            if (ctx.hasFocus(this))
            {
                for (int k : ctx.inputEvents.keys)
                {
                    if (k == Key::Escape)
                    {
                        if (isOpen)
                            setOpen(false); // Escape chiude il menù
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
                        setOpen(!isOpen); // Toggle apertura
                        if (isOpen && listContainer && selected < (int)listContainer->children.size())
                            listContainer->children[selected]->scrollIntoView();
                    }
                }
            }

            if (!isOpen)
                return;

            // --- Click fuori per chiudere ---
            if (ctx.pointer[MouseButton::Left].pressed || ctx.pointer[MouseButton::Right].pressed)
            {
                if (!rect.contains(ctx.pointer.pos) &&
                    !listContainer->getRect().contains(ctx.pointer.pos))
                {
                    setOpen(false);
                }
            }
        }

        void onBuild() override
        {
            setStyleTag("Dropdown");
            setInteractive(true);
            setFocusable(true); // Il Dropdown stesso riceve il focus
            internal_.pendingTransition = 1;
            getInlineBase().position = Position::Relative;
            getInlineBase().zIndex = ZIndex(10);

            // Il bottone interno propaga il focus al Dropdown
            button = Btn("", [this]()
                         { 
                UIContext::get().requestFocus(shared_from_this());
                toggleOpen(); });
            button->cls("dropdown-trigger");
            button->setFocusable(false); // Disabilita tab stop sul bottone figlio

            button->getInlineBase().width = Percent(100);
            button->getInlineDefaults().height = VH(3.0f);
            button->getInlineDefaults().background = Color{40, 40, 45, 255};
            button->getInlineDefaults().radius = Px(6.0f);
            button->getInlineDefaults().borderColor = Color{70, 70, 80, 255};
            button->getInlineDefaults().borderWidth = Px(1.0f);
            button->getInlineDefaults().itemsH = Align::Center;
            button->getInlineDefaults().itemsV = Align::Center;
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
            listContainer->getInlineDefaults().itemsH = Align::Stretch;
            listContainer->getInlineDefaults().overflowX = Overflow::Auto;
            listContainer->getInlineDefaults().overflowY = Overflow::Auto;

            for (size_t i = 0; i < options.size(); ++i)
            {
                int idx = (int)i;
                auto item = Label(options[i]);
                item->cls("dropdown-item");
                item->getInlineBase().padding = Spacing(VH(1.0f), VW(1.0f));
                item->getInlineBase().itemsV = Align::Center;
                item->setInteractive(true);
                item->setFocusable(false); // Niente tab stop per le voci

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
                // Sta sotto
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

        void toggleOpen() { setOpen(!isOpen); }

        void applyOpenState()
        {
            listContainer->setEnabled(isOpen);
            listContainer->setInteractive(isOpen);
            listContainer->getInlineBase().opacity = isOpen ? 1.0f : 0.0f;
            internal_.pendingTransition = 1;
        }

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

        void updateListStyles()
        {
            if (!listContainer)
                return;
            for (size_t i = 0; i < listContainer->children.size(); ++i)
            {
                auto &item = listContainer->children[i];
                // Affidiamo tutto allo stato CSS!
                item->setChecked((int)i == selected);
            }
        }
    };
} // namespace ZenitUI::UI