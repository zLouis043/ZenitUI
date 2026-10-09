#pragma once

#include "UI.hpp"

using namespace ZenitUI;
using namespace ZenitUI::UI;

class SettingsScreen : public TLayout<SettingsScreen>
{
public:
    SettingsScreen(std::function<void()> onCloseCb = nullptr)
        : TLayout<SettingsScreen>(LayoutType::Stack),
          onCloseCb(std::move(onCloseCb)) {}

    void close()
    {
        if (onCloseCb)
            onCloseCb();
        removeFromParent();
    }

protected:
    void onBuild() override
    {
        addClass("settings-overlay");
        setFocusScope(true);
        setInteractive(true);
        setBlocksRaycast(true);

        // ---- Panel centrale ----
        auto panel = VStack()->cls("settings-panel");
        panel->setInteractive(true);
        panel->setBlocksRaycast(true);
        // Un click sul panel (o figli) non deve risalire all'overlay.
        panel->onClick = []() { /* consuma il click */ };

        // ---------- Header ----------
        auto header = HStack()->cls("settings-header");
        header->addChild(Label("Impostazioni")->cls("settings-title"));
        header->addChild(std::make_shared<Layout>()->cls("spacer-grow"));

        auto closeBtn = Btn("X", [this]()
                            { close(); })
                            ->cls("settings-close");
        header->addChild(closeBtn);
        panel->addChild(header);

        // ---------- Body (placeholder) ----------
        auto body = ScrollView::create(LayoutType::Vertical)->cls("settings-body");
        buildAudioSection(body);
        buildVideoSection(body);
        buildGameplaySection(body);
        panel->addChild(body);

        // ---------- Footer ----------
        auto footer = HStack()->cls("settings-footer");

        auto resetBtn = Btn("Ripristina default", [this]()
                            {
                                // TODO Step 5
                            })
                            ->cls("btn-secondary");

        auto cancelBtn = Btn("Annulla", [this]()
                             { close(); })
                             ->cls("btn-secondary");

        auto saveBtn = Btn("Salva", [this]()
                           {
            // TODO Step 5
            close(); })
                           ->cls("btn-primary");
        saveBtn->setEnabled(false); // finché non dirty

        footer->addChild(resetBtn);
        footer->addChild(std::make_shared<Layout>()->cls("spacer-grow"));
        footer->addChild(cancelBtn);
        footer->addChild(saveBtn);
        panel->addChild(footer);

        addChild(panel);
    }

    void onUpdate(float) override
    {
        // Click sull'overlay ma fuori dal panel → chiudi.
        auto &ctx = UIContext::get();
        if (ctx.pointer.pressed && ctx.topmostConsumer == this)
        {
            close();
            return;
        }

        // ESC chiude.
        for (int k : ctx.inputEvents.keys)
        {
            if (k == Key::Escape)
            {
                close();
                return;
            }
        }
    }

private:
    std::function<void()> onCloseCb;

    // ============================================================
    //  Sezione Audio — markup ZMarkup + binding in C++
    // ============================================================
    void buildAudioSection(std::shared_ptr<Layout> parent)
    {
        static const char *markup = R"(
            VStack.settings-section {
                Text.settings-section-title "Audio"

                HStack.settings-row {
                    Text.setting-label "Musica"
                    Slider#music-slider value=0.55 width="55%"
                    Text.setting-value#music-value "55%"
                }

                HStack.settings-row {
                    Text.setting-label "Effetti"
                    Slider#sfx-slider value=0.80 width="55%"
                    Text.setting-value#sfx-value "80%"
                }

                HStack.settings-row {
                    Text.setting-label "Muto globale"
                    Toggle#mute-toggle checked=false
                }
            }
        )";

        auto ui = ZMarkup::build(markup);
        if (!ui.root())
            return;

        parent->addChild(ui.root());

        // Binding percentuali live
        bindPercent(ui, "music-slider", "music-value");
        bindPercent(ui, "sfx-slider", "sfx-value");
    }

    // ============================================================
    //  Sezione Video
    // ============================================================
    void buildVideoSection(std::shared_ptr<Layout> parent)
    {
        static const char *markup = R"(
        VStack.settings-section {
            Text.settings-section-title "Video"

            HStack.settings-row {
                Text.setting-label "Schermo intero"
                Toggle#fs-toggle checked=false
            }

            HStack.settings-row {
                Text.setting-label "Limite FPS"
                Dropdown#fps-dropdown options="30,60,120,Illimitato"
                Text.setting-value#fps-value "60 FPS"
            }
        }
    )";

        auto ui = ZMarkup::build(markup);
        if (!ui.root())
            return;
        parent->addChild(ui.root());

        if (auto fps = ui.find<Dropdown>("fps-dropdown"))
        {
            auto label = ui.find<Text>("fps-value");
            fps->setSelected(1); // default: 60

            // refresh iniziale della label
            if (label)
                label->setText("60 FPS");

            fps->onChange = [label](int /*idx*/, const std::string &name)
            {
                if (!label)
                    return;
                if (name == "Illimitato")
                    label->setText("Unlimited");
                else
                    label->setText(name + " FPS");
            };
        }
    }

    // ============================================================
    //  Sezione Gameplay
    // ============================================================
    void buildGameplaySection(std::shared_ptr<Layout> parent)
    {
        static const char *markup = R"(
        VStack.settings-section {
            Text.settings-section-title "Gameplay"

            HStack.settings-row {
                Text.setting-label "Difficoltà"
                Dropdown#diff-dropdown options="Facile,Normale,Difficile"
                Text.setting-value#diff-value "Normale"
            }

            HStack.settings-row {
                Text.setting-label "Aim assist"
                Toggle#aim-toggle checked=true
            }

            HStack.settings-row {
                Text.setting-label "Inverti Y"
                Toggle#invert-toggle checked=false
            }
        }
    )";

        auto ui = ZMarkup::build(markup);
        if (!ui.root())
            return;
        parent->addChild(ui.root());

        if (auto diff = ui.find<Dropdown>("diff-dropdown"))
        {
            auto label = ui.find<Text>("diff-value");
            diff->setSelected(1); // default: Normale
            if (label)
                label->setText("Normale");

            diff->onChange = [label](int /*idx*/, const std::string &name)
            {
                if (label)
                    label->setText(name);
            };
        }
    }

    static void bindPercent(ZMarkup::UINode &ui,
                            const char *sliderId,
                            const char *labelId)
    {
        auto slider = ui.find<Slider>(sliderId);
        auto label = ui.find<Text>(labelId);
        if (!slider || !label)
            return;

        // Settiamo subito il testo iniziale
        label->setText(std::to_string((int)std::lround(slider->getValue() * 100.0f)) + "%");

        slider->onValueChanged = [label](float v)
        {
            int pct = (int)std::lround(v * 100.0f);
            label->setText(std::to_string(pct) + "%");
        };
    }
};