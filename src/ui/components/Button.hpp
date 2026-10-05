#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "Text.hpp"
#include "Layouts.hpp"

namespace ZenitUI::UI {

class Button : public TLayout<Button, Panel>
{
public:
    Button(std::function<void()> cb)
    {
        onClick = std::move(cb);
        setInteractive(true);
        setFocusable(true);
        setKeyboardActivates(true);
        setStyleTag("Button");
        getInlineBase().itemsH = Align::Center;
        getInlineBase().itemsV = Align::Center;
    }
};

inline std::shared_ptr<Button> Btn(std::string text, std::function<void()> cb = nullptr)
{
    auto b = Button::create(std::move(cb));
    b->addChild(Label(text)->cls("btn-text"));
    return b;
}

} // namespace ZenitUI::UI