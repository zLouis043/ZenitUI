#pragma once

#include "UI.hpp"
#include <cstdio>
#include <string>

using namespace ZenitUI;
using namespace ZenitUI::UI;

class DemoWindow : public TLayout<DemoWindow> {
public:
    DemoWindow(std::function<void()> onCloseCb = nullptr)
        : TLayout<DemoWindow>(LayoutType::Stack), onCloseCb(std::move(onCloseCb)) {}

    void close() {
        if (onCloseCb) onCloseCb();
        removeFromParent();
    }

protected:
    void onBuild() override {
        addClass("demo-overlay");
        setFocusScope(true);

        auto layoutRoot = HStack()->cls("demo-panel");
        auto sidebar    = VStack()->cls("demo-sidebar");
        contentArea     = ScrollView::create(LayoutType::Vertical)->cls("demo-content");

        sidebar->addChild(Label("ZENIT UI DEMO")->cls("title-text"));
        sidebar->addChild(std::make_shared<Layout>()->size(Auto(), VH(2.0f)));

        auto addTab = [&](const std::string& name, std::function<void(std::shared_ptr<Layout>)> builder) {
            auto btn = Btn(name, [this, builder]() {
                this->contentArea->children.clear();
                this->contentArea->scrollToTop();
                builder(this->contentArea);
            })->cls("tab-btn");
            sidebar->addChild(btn);
        };

        addTab("1. Widget Base",      [this](auto p) { buildBasicWidgets(p); });
        addTab("2. Layout & Flexbox", [this](auto p) { buildFlexboxTest(p); });
        addTab("3. Z-Index & Portal", [this](auto p) { buildPortalTest(p); });
        addTab("4. Markup Layout",    [this](auto p) { buildMarkupTest(p); });
        addTab("5. Animazioni",       [this](auto p) { buildAnimationsTest(p); });
        addTab("6. Event Bubbling",   [this](auto p) { buildBubblingTest(p); });
        addTab("7. Stress & Culling", [this](auto p) { buildCullingTest(p); });

        sidebar->addChild(std::make_shared<Layout>()->cls("spacer-grow"));
        sidebar->addChild(Btn("CHIUDI DEMO", [this]() { close(); })->cls("btn-danger"));

        layoutRoot->addChild(sidebar);
        layoutRoot->addChild(contentArea);
        addChild(layoutRoot);

        buildBasicWidgets(contentArea);
    }

private:
    std::function<void()> onCloseCb;
    std::shared_ptr<ScrollView> contentArea;

    static void addHeader(std::shared_ptr<Layout> parent,
                          const std::string& title, const std::string& desc) {
        parent->addChild(Label(title)->cls("title-text"));
        auto d = Label(desc);
        d->setWrap(true);
        d->cls("card-desc");
        parent->addChild(d);
    }

    // ---------- TAB 1: WIDGET BASE ----------
    void buildBasicWidgets(std::shared_ptr<Layout> parent) {
        addHeader(parent, "1. Widget di Base",
            "Panoramica dei componenti UI standard: input testuale, slider, progress, toggle, checkbox e bottoni.");

        auto card = VStack()->cls("card");

        auto textInput = std::make_shared<TextInput>("Testo editabile");
        textInput->getInlineBase().width = Percent(50);
        card->addChild(HStack({ Label("TextInput: ")->cls("normal-text")->cls("setting-label"), textInput })->cls("setting-row"));

        auto sfxSlider = std::make_shared<Slider>(0.5f);
        sfxSlider->getInlineBase().width  = Percent(50);
        sfxSlider->getInlineBase().height = VH(3.0f);

        auto pb = std::make_shared<ProgressBar>(0.5f);
        pb->getInlineBase().width  = Percent(50);
        pb->getInlineBase().height = VH(2.0f);
        sfxSlider->onValueChanged = [pb](float v) { pb->setValue(v); };

        card->addChild(HStack({ Label("Slider → ProgressBar: ")->cls("normal-text")->cls("setting-label"), sfxSlider })->cls("setting-row"));
        card->addChild(HStack({ Label("ProgressBar: ")->cls("normal-text")->cls("setting-label"), pb })->cls("setting-row"));

        auto toggle = std::make_shared<Toggle>(true);
        toggle->size(VW(4.0f), VH(4.0f));
        auto check  = std::make_shared<Checkbox>(false);
        check->size(VH(4.0f), VH(4.0f));

        card->addChild(HStack({ Label("Toggle: ")->cls("normal-text")->cls("setting-label"),   toggle })->cls("setting-row"));
        card->addChild(HStack({ Label("Checkbox: ")->cls("normal-text")->cls("setting-label"), check  })->cls("setting-row"));

        auto btnRow = HStack({
            Btn("Primario", []() { std::printf("Primario\n"); })->cls("btn-primary"),
            Btn("Danger",   []() { std::printf("Danger\n");   })->cls("btn-danger"),
            Btn("Disabilitato")->cls("btn-primary")
        })->cls("button-row");
        btnRow->children.back()->setEnabled(false);

        card->addChild(btnRow);

        Tooltip::attach(btnRow->children[0], "Esegue l'azione principale", 0.3f);
        Tooltip::attach(btnRow->children[1], "Azione distruttiva, richiede conferma", 0.3f);
        Tooltip::attach(toggle, "Abilita o disabilita questa opzione", 0.3f);
        Tooltip::attach(check,  "Salva i progressi sul cloud", 0.3f);

        parent->addChild(card);
    }

    // ---------- TAB 2: FLEXBOX ----------
    void buildFlexboxTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "2. Layout & Flexbox",
            "Justify, grow/shrink, gap, allineamento cross-axis.");

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Justify: SpaceBetween")->cls("card-title"));
            auto row = HStack({ Label("Sinistra"), Label("Centro"), Label("Destra") });
            row->getInlineBase().width = Percent(100);
            row->getInlineBase().justify = Justify::SpaceBetween;
            row->getInlineBase().background = ZenitUI::Color{50, 50, 60, 255};
            row->getInlineBase().padding = Spacing(VH(1.0f));
            card->addChild(row);
            parent->addChild(card);
        }

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Grow Property (1 : 2 : 1)")->cls("card-title"));
            auto b1 = Label("grow 1"); b1->getInlineBase().background = ZenitUI::Colors::Maroon;    b1->getInlineBase().grow = 1.0f;
            auto b2 = Label("grow 2"); b2->getInlineBase().background = ZenitUI::Colors::DarkGreen; b2->getInlineBase().grow = 2.0f;
            auto b3 = Label("grow 1"); b3->getInlineBase().background = ZenitUI::Colors::Maroon;    b3->getInlineBase().grow = 1.0f;
            auto row = HStack({ b1, b2, b3 });
            row->getInlineBase().width = Percent(100);
            row->getInlineBase().gap = VW(0.5f);
            card->addChild(row);
            parent->addChild(card);
        }

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("items-v: Center + justify: SpaceBetween")->cls("card-title"));
            auto row = HStack({ Label("sx"), Label("centro"), Label("dx") });
            row->getInlineBase().width = Percent(100);
            row->getInlineBase().height = VH(10.0f);
            row->getInlineBase().itemsV = Align::Center;
            row->getInlineBase().justify = Justify::SpaceBetween;
            row->getInlineBase().background = ZenitUI::Color{50, 50, 60, 255};
            row->getInlineBase().gap = VW(1.0f);
            card->addChild(row);
            parent->addChild(card);
        }
    }

    // ---------- TAB 3: PORTAL ----------
    void buildPortalTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "3. Z-Index & Portal",
            "Il Dropdown usa i Portal per scavalcare il clipping della ScrollView.");

        auto card = VStack()->cls("card");
        auto dropdown = Dropdown::create(
            std::vector<std::string>{"Opzione 1", "Opzione 2", "Opzione 3"}, 0);
        dropdown->getInlineBase().width = VW(20.0f);
        card->addChild(HStack({ Label("Valore: ")->cls("normal-text")->cls("setting-label"), dropdown })->cls("setting-row"));

        // Box contenitore con position: relative (implicito per absolute)
        auto stage = VStack()->cls("card");
        stage->getInlineBase().height = VH(30.0f);
        stage->getInlineBase().position = Position::Relative;

        // Badge in alto a destra: right=8, top=8
        auto badge = Label("BADGE")->cls("normal-text");
        badge->getInlineBase().position = Position::Absolute;
        badge->getInlineBase().top    = Px(8.0f);
        badge->getInlineBase().right  = Px(8.0f);
        badge->getInlineBase().background = Colors::Red;
        badge->getInlineBase().padding = Spacing(Px(4.0f), Px(8.0f));
        badge->getInlineBase().radius = Px(4.0f);
        stage->addChild(badge);

        // Label centrato: left=50% + right=50% → stretch a 0, quindi left+width esplicito
        auto center = Label("centrato")->cls("normal-text");
        center->getInlineBase().position = Position::Absolute;
        center->getInlineBase().top = Percent(50);
        center->getInlineBase().left = Percent(50);
        center->getInlineBase().width = Px(100.0f);
        // left 50% del parent, e poi translato a metà larghezza per centrare:
        center->getInlineBase().translateX = Percent(-50.0f);
        stage->addChild(center);

        // Stretch horizontal: left=0, right=0, width auto
        auto stretch = Label("stretch left→right")->cls("normal-text");
        stretch->getInlineBase().position = Position::Absolute;
        stretch->getInlineBase().left  = Px(0.0f);
        stretch->getInlineBase().right = Px(0.0f);
        stretch->getInlineBase().bottom = Px(8.0f);
        stage->getInlineBase().itemsH = Align::Center;
        stretch->getInlineBase().textAlign = Align::Center;
        stage->addChild(stretch);

        card->addChild(stage);
        parent->addChild(card);
    }

    // ---------- TAB 4: MARKUP ----------
    void buildMarkupTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "4. Markup Layout",
            "Layout costruito interamente da stringa ZMarkup. Supporta tag, classi, id, attributi inline e annidamento.");

        static const char* markup = R"(
            VStack.card {
                Text.card-title "Form di Configurazione"
                Text.card-desc wrap "Questo pannello è costruito da markup ZMarkup: ogni widget è creato tramite factory registrate, con classi CSS e attributi inline."
                HStack.setting-row {
                    Text.normal-text "Nome Utente:"
                    TextInput#mk-name value="Player1" width="55%"
                }
                HStack.setting-row {
                    Text.normal-text "Volume Master:"
                    Slider#mk-volume value=0.65 width="55%"
                }
                HStack.setting-row {
                    Text.normal-text "Audio Spaziale:"
                    Toggle#mk-spatial checked=true
                }
                HStack.setting-row {
                    Text.normal-text "Cloud Save:"
                    Checkbox#mk-cloud checked=false
                }
                HStack.setting-row {
                    Text.normal-text "Download:"
                    ProgressBar#mk-progress value=0.3 width="55%"
                }
                HStack.setting-row {
                    Text.normal-text "Lingua:"
                    Dropdown#mk-lang options="Italiano,English,Deutsch,Français" width="55%"
                }
                HStack.button-row {
                    Button.btn-primary#mk-save "Salva"
                    Button.btn-danger#mk-reset "Reset"
                }
            }
        )";

        auto ui = ZMarkup::build(markup);
        if (auto root = ui.root()) {
            parent->addChild(root);

            if (auto sl = ui.find<Slider>("mk-volume"))
                if (auto pb = ui.find<ProgressBar>("mk-progress"))
                    sl->onValueChanged = [pb](float v) { pb->setValue(v); };

            ui.onClick("mk-save",  []() { std::printf("MARKUP: Salva\n"); });
            ui.onClick("mk-reset", []() { std::printf("MARKUP: Reset\n"); });
        } else {
            parent->addChild(Label("Errore: markup non valido.")->cls("card-desc"));
        }

        static const char* markup2 = R"(
            HStack.card {
                VStack.side-panel {
                    Text.card-title "Annidato"
                    Text.card-desc wrap "Layout complesso costruito con markup annidato a più livelli. Questo secondo pannello dimostra classi annidate e layout orizzontali."
                }
                VStack.side-panel {
                    Text.card-title "Controlli"
                    HStack gap="8px" {
                        Text.normal-text "A"
                        Text.normal-text "B"
                        Text.normal-text "C"
                    }
                    Button.btn-primary "Pulsante"
                    Button.btn-danger "Pulsante 2"
                }
            }
        )";

        auto ui2 = ZMarkup::build(markup2);
        if (auto r2 = ui2.root()) parent->addChild(r2);
    }

    // ---------- TAB 5: ANIMAZIONI ----------
    void buildAnimationsTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "5. Animazioni",
            "Keyframes CSS e transizioni di stato. Ora la trasformazione animata viene applicata anche al draw.");

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Pulse (keyframes infinite)")->cls("card-title"));
            auto btn = Btn("Pulsazione", []() { std::printf("Pulse\n"); });
            btn->cls("btn-primary")->cls("btn-pulse");
            card->addChild(btn);
            parent->addChild(card);
        }

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Breathe (opacity + scale)")->cls("card-title"));
            auto box = HStack({ Label("Box che respira")->cls("normal-text")->cls("setting-label") });
            box->cls("breathing-box");
            card->addChild(box);
            parent->addChild(card);
        }

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Slide-In (ease-out-back)")->cls("card-title"));
            auto box = HStack({ Label("Questo box è entrato con slide-in")->cls("normal-text")->cls("setting-label") });
            box->cls("slide-in-box");
            card->addChild(box);
            parent->addChild(card);
        }

        {
            auto card = VStack()->cls("card");
            card->addChild(Label("Hover (transition su background + scale)")->cls("card-title"));
            auto box = HStack({ Label("Passa il mouse qui")->cls("normal-text")->cls("setting-label") });
            box->cls("hover-box");
            card->addChild(box);
            parent->addChild(card);
        }
    }

    // ---------- TAB 6: BUBBLING ----------
    void buildBubblingTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "6. Event Bubbling",
            "Propagazione bottom-up dei click e passThrough per lasciare che il click raggiunga il genitore.");

        auto card = VStack()->cls("card");

        auto parentBox = VStack()->cls("bubbling-box");
        parentBox->setInteractive(true);
        parentBox->onClick = []() { std::printf("CLICK CONSUMATO DAL BOX PADRE!\n"); };
        parentBox->addChild(Label("Clicca lo sfondo di questo box."));

        auto btnNormal = Btn("Bottone Normale", []() {
            std::printf("CLICK: Bottone Normale\n");
        })->cls("btn-primary");

        auto btnPass = Btn("Bottone Passthrough", []() {
            std::printf("CLICK: Bottone Passthrough\n");
        })->cls("btn-danger");
        btnPass->setPassThrough(true);

        parentBox->addChild(btnNormal);
        parentBox->addChild(btnPass);

        card->addChild(parentBox);
        parent->addChild(card);

        // --- Test Popup ---
        auto popupTest = VStack()->cls("card");
        popupTest->addChild(Label("Popup / ContextMenu")->cls("card-title"));
        popupTest->addChild(Label("Click sinistro o destro sul bottone per aprire un menu contestuale.")->cls("card-desc"));

        auto trigger = Btn("Apri Menu", nullptr)->cls("btn-primary");
        popupTest->addChild(trigger);

        auto menu = ContextMenu({
            { "Voce 1", []() { std::printf("Menu: Voce 1\n"); } },
            { "Voce 2", []() { std::printf("Menu: Voce 2\n"); } },
            { "Voce lunga di test", []() { std::printf("Menu: Voce lunga\n"); } },
        });

        // Il popup è figlio del trigger (per il lifetime), ma essendo portal
        // viene renderizzato sopra tutto.
        trigger->addChild(menu);
        
        trigger->onClick      = [trigger, menu]() { menu->openBelow(trigger); };
        trigger->onRightClick = [trigger, menu]() { menu->openBelow(trigger); };

        auto box = VStack()->cls("bubbling-box");
        box->addChild(Label("Click destro su questo box")->cls("normal-text"));

        auto boxMenu = ContextMenu({
            { "Azione A", []() { std::printf("Box: Azione A\n"); } },
            { "Azione B", []() { std::printf("Box: Azione B\n"); } },
        });
        box->addChild(boxMenu);
        box->onRightClick = [box, boxMenu]() { boxMenu->openAt(UIContext::get().pointer.pos); };

        popupTest->addChild(box);

        parent->addChild(popupTest);
    }

    // ---------- TAB 7: STRESS ----------
    void buildCullingTest(std::shared_ptr<Layout> parent) {
        addHeader(parent, "7. Stress & Culling",
            "200 elementi in una ScrollView. Il culling evita di disegnare quelli fuori dal viewport.");

        auto card = VStack()->cls("card");
        card->getInlineBase().gap = Px(4.0f);

        for (int i = 0; i < 200; ++i) {
            auto row = HStack({
                Label("Elemento #" + std::to_string(i))->cls("normal-text")->cls("setting-label"),
                Btn("Azione " + std::to_string(i))
            })->cls("setting-row");
            row->getInlineBase().background = (i % 2 == 0)
                ? ZenitUI::Color{35, 35, 40, 255}
                : ZenitUI::Color{45, 45, 50, 255};
            row->getInlineBase().padding = Spacing(Px(10.0f));
            row->getInlineBase().radius = Px(4.0f);
            card->addChild(row);
        }
        parent->addChild(card);
    }
};