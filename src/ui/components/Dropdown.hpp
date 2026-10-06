#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "ScrollView.hpp"

namespace ZenitUI::UI {

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
        if (onChange)
            onChange(selected, options[selected]);
        pendingTransition = true;
    }

    bool getOpen() const { return isOpen; }
    void setOpen(bool o)
    {
        if (isOpen == o)
            return;
        isOpen = o;
        applyOpenState();
    }

    std::function<void(int, const std::string &)> onChange;

protected:
    void onEnabledChanged(bool nowEnabled) override
    {
        if (!nowEnabled && isOpen)
        {
            isOpen = false;
            applyOpenState();
        }
    }

    void onUpdate(float) override
    {
        if (!isOpen)
            return;
        auto &ctx = UIContext::get();
        if (!ctx.pointer.pressed)
            return;
        if (!rect.contains(ctx.pointer.pos) &&
            !listContainer->getRect().contains(ctx.pointer.pos))
        {
            setOpen(false);
        }
    }

    void onBuild() override
    {
        setStyleTag("Dropdown");
        setInteractive(true);
        setFocusable(true);
        pendingTransition = true;

        getInlineBase().position = Position::Relative;
        getInlineBase().zIndex = ZIndex(10);

        button = Btn("", [this]() { toggleOpen(); });
        button->cls("dropdown-trigger");
        button->getInlineBase().width = Percent(100);
        button->getInlineBase().height = VH(3.0f);
        button->getInlineBase().background = Color{40, 40, 45, 255};
        button->getInlineBase().radius = Px(6.0f);
        button->getInlineBase().borderColor = Color{70, 70, 80, 255};
        button->getInlineBase().borderWidth = Px(1.0f);
        button->getInlineBase().itemsH = Align::Center;
        button->getInlineBase().itemsV = Align::Center;
        addChild(button);

        listContainer = std::make_shared<ScrollView>();
        listContainer->getInlineBase().position = Position::Absolute;
        listContainer->setPortal(true);
        listContainer->getInlineBase().width = VW(15.0f);
        int visibleRows = std::min((int)options.size(), 6);
        listContainer->getInlineBase().height = VH(2.0f + visibleRows * 4.0f);
        listContainer->getInlineBase().background = Color{30, 30, 36, 255};
        listContainer->getInlineBase().borderColor = Color{60, 60, 70, 255};
        listContainer->getInlineBase().borderWidth = Px(1.0f);
        listContainer->getInlineBase().itemsH = Align::Stretch;
        for (size_t i = 0; i < options.size(); ++i)
        {
            int idx = (int)i;
            auto item = Label(options[i]);
            item->getInlineBase().padding = Spacing(VH(1.0f), VW(1.0f));
            item->getInlineBase().itemsV = Align::Center;
            item->getInlineBase().background = Color{40, 40, 50, 255};
            item->getInlineBase().color = Colors::White;
            item->setInteractive(true);
            item->setFocusable(true);
            item->onClick = [this, idx]()
            {
                setSelected(idx);
                setOpen(false);
            };
            listContainer->addChild(item);
        }
        addChild(listContainer);
        isOpen = false;
        applyOpenState();
        refreshButtonText();
    }

    void arrange(Rect space) override
    {
        TLayout<Dropdown>::arrange(space);

        if (button && listContainer)
        {
            Rect btnRect = button->getRect();
            listContainer->arrange({btnRect.x,
                                    btnRect.y + btnRect.height + 4.0f,
                                    btnRect.width,
                                    listContainer->getMeasuredSize().y});
        }
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
        pendingTransition = true;
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
};

} // namespace ZenitUI::UI