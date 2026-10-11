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

    // Returns the current label text, or "" if the button has no Text
    // child (e.g. created via Button::create(cb) and never given one).
    std::string text() const
    {
        auto t = labelChild();
        return t ? t->getText() : std::string{};
    }

    // Replaces the label. If the button has no Text child yet, one is
    // created with the same class `Btn()` uses ("btn-text"), so the
    // result is indistinguishable from a button built with `Btn(...)`.
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
    std::shared_ptr<Text> labelChild() const
    {
        if (children.empty())
            return nullptr;
        return std::dynamic_pointer_cast<Text>(children.front());
    }
};

inline std::shared_ptr<Button> Btn(std::string text, std::function<void()> cb = nullptr)
{
    auto b = Button::create(std::move(cb));
    b->addChild(Label(text)->cls("btn-text"));
    return b;
}

} // namespace ZenitUI::UI