#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

class Toggle : public TLayout<Toggle>
{
public:
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

    std::function<void(bool)> onToggle = nullptr;
    bool isChecked;

protected:
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