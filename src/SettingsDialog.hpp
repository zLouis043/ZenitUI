#pragma once

#include "UI.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;

class SettingsDialog : public TLayout<SettingsDialog> {
public:
    SettingsDialog(std::function<void()> onCloseCb = nullptr)
        : TLayout<SettingsDialog>(LayoutType::Stack), onCloseCb(std::move(onCloseCb)) {}

    void close() {
        if (onCloseCb) onCloseCb();
        removeFromParent();
    }

protected:
    void onBuild() override {
        // --- corpo dell'ex costruttore, con addChild che ora usa weak_from_this valido ---
        addClass("dialog-overlay");
        setFocusScope(true);
        getInlineBase().background = Colors::Black.withAlpha(0.0f);

        auto sfxSlider = std::make_shared<Slider>(0.8f);
        sfxSlider->getInlineBase().width  = VW(15.0f);
        sfxSlider->getInlineBase().height = VH(3.0f);

        auto fsToggle = std::make_shared<Toggle>(true);
        fsToggle->getInlineBase().width  = VW(4.0f);
        fsToggle->getInlineBase().height = VH(4.0f);

        auto cancelBtn = Btn("ANNULLA", [this]() { close(); })->cls("btn-danger");
        auto saveBtn   = Btn("SALVA",   [this]() { close(); })
                            ->cls("btn-primary")
                            ->cls("btn-pulse");

        auto volumeBar = std::make_shared<ProgressBar>(0.8f);
        volumeBar->getInlineBase().width  = VW(15.0f);
        volumeBar->getInlineBase().height = VH(2.0f);

        sfxSlider->onValueChanged = [volumeBar](float v) { volumeBar->setValue(v); };

        auto vibCheckbox = std::make_shared<Checkbox>(true);
        vibCheckbox->getInlineBase().width  = VH(4.0f);
        vibCheckbox->getInlineBase().height = VH(4.0f);

        auto nameInput = std::make_shared<TextInput>("Player1");
        nameInput->getInlineBase().width = VW(20.0f);
        nameInput->onSubmit = [](const std::string& s) {
            std::printf("Submitted: %s\n", s.c_str());
        };

        auto languageDropdown = Dropdown::create(
            std::vector<std::string>{"Italiano","English","Deutsch","Français","Español"}, 0);
        languageDropdown->getInlineBase().width  = VW(15.0f);
        

        panel = VStack({
            Label("IMPOSTAZIONI")->cls("title-text"),
            HStack({ Label("Volume Effetti")->cls("normal-text"), sfxSlider })->cls("setting-row"),
            HStack({ Label("Barra Volume")->cls("normal-text"),   volumeBar })->cls("setting-row"),
            HStack({ Label("Schermo Intero")->cls("normal-text"), fsToggle })->cls("setting-row"),
            HStack({ Label("Vibrazione")->cls("normal-text"),     vibCheckbox })->cls("setting-row"),
            HStack({ Label("Nome Giocatore")->cls("normal-text"), nameInput })->cls("setting-row"),
            HStack({ Label("Lingua")->cls("normal-text"),         languageDropdown })->cls("setting-row"),
            std::make_shared<Layout>()->size(Percent(100), VH(2.0f)),
            HStack({
                std::make_shared<Layout>()->cls("spacer-grow"),
                cancelBtn, saveBtn,
                std::make_shared<Layout>()->cls("spacer-grow")
            })->cls("button-row")
        });

        panel->cls("settings-panel");
        panel->getInlineBase().translateY = VH(150.0f);
        addChild(panel);

        auto introAnim = std::make_shared<UIAnimation>(0.5f);
        introAnim->addTrack<ZenitUI::Color>(
            Colors::Black.withAlpha(0.0f), Colors::Black.withAlpha(0.7f),
            [](Layout* l, ZenitUI::Color c) { l->getInlineBase().background = c; },
            TransitionFunction::Linear);
        introAnim->addTrack<Value>(
            VH(150.0f), Value(0.0f),
            [this](Layout*, Value v) { this->panel->getInlineBase().translateY = v; },
            TransitionFunction::EaseOutBack);
        addAnimation("Intro", introAnim);
        playAnimation("Intro");
    }

    std::function<void()> onCloseCb;
    std::shared_ptr<Layout> panel;
};