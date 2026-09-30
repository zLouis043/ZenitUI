#pragma once

#include "UI.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;

class SettingsDialog : public TLayout<SettingsDialog> {
public:
    SettingsDialog(std::function<void()> onCloseCb = nullptr)
        : TLayout<SettingsDialog>(LayoutType::Stack), onCloseCb(std::move(onCloseCb))
    {
        getInlineBase().width  = Percent(100.0f);
        getInlineBase().height = Percent(100.0f);
        getInlineBase().background = Colors::Black.withAlpha(0.0f);
        getInlineBase().itemsH = Align::Center;
        getInlineBase().itemsV = Align::Center;

        auto sfxSlider = std::make_shared<Slider>(0.8f);
        sfxSlider->getInlineBase().width  = VW(15.0f);
        sfxSlider->getInlineBase().height = VH(3.0f);
        sfxSlider->onValueChanged = [](float) {};

        auto fsToggle = std::make_shared<Toggle>(true);
        fsToggle->getInlineBase().width  = VW(4.0f);
        fsToggle->getInlineBase().height = VH(4.0f);

        auto cancelBtn = Btn("ANNULLA", [this]() { close(); });
        auto saveBtn   = Btn("SALVA",   [this]() { close(); });
        cancelBtn->cls("btn-danger");
        saveBtn  ->cls("btn-primary");

        // TEST STEP 4: animazione CSS sul bottone SALVA (pulse infinito)
        saveBtn->cls("btn-pulse");

        panel = VStack({
            Label("IMPOSTAZIONI")->cls("title-text"),
            HStack({ Label("Volume Effetti")->cls("normal-text"), sfxSlider })->cls("setting-row"),
            HStack({ Label("Schermo Intero")->cls("normal-text"), fsToggle })->cls("setting-row"),
            std::make_shared<Layout>()->size(Percent(100), VH(2.0f)),
            HStack({ cancelBtn, saveBtn })->cls("setting-row")->size(Percent(100), Auto())
        });

        panel->cls("settings-panel");
        panel->getInlineBase().translateY = VH(150.0f);

        panel->children.back()->getInlineBase().justify = Justify::Center;
        panel->children.back()->getInlineBase().gap     = VW(2.0f);

        addChild(panel);

        auto introAnim = std::make_shared<UIAnimation>(0.5f);
        introAnim->addTrack<ZenitUI::Color>(
            Colors::Black.withAlpha(0.0f),
            Colors::Black.withAlpha(0.7f),
            [](Layout* l, ZenitUI::Color c) { l->getInlineBase().background = c; },
            TransitionFunction::Linear
        );
        introAnim->addTrack<Value>(
            VH(150.0f), Value(0.0f),
            [this](Layout*, Value v) { this->panel->getInlineBase().translateY = v; },
            TransitionFunction::EaseOutBack
        );
        addAnimation("Intro", introAnim);
        playAnimation("Intro");
    }

    void close() {
        if (onCloseCb) onCloseCb();
        removeFromParent();
    }

private:
    std::function<void()> onCloseCb;
    std::shared_ptr<Layout> panel;
};