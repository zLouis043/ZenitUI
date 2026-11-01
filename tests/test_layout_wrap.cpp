#include "TestFramework.hpp"
#include "Layout.hpp"
#include "Theme.hpp"

using namespace ZenitUI;

// Helper: reset del theme tra un test e l'altro
static void freshTheme() {
    Theme::get().clear();
}

// Helper: crea un blocco a dimensione fissa che non si restringe
static std::shared_ptr<Layout> makeBox(float w, float h) {
    auto n = std::make_shared<Layout>(LayoutType::Stack);
    n->size(Px(w), Px(h));
    n->getInlineBase().shrink = 0.0f; // Impedisce il restringimento per forzare l'a capo
    return n;
}

// ============================================================
//  Layout: wrap (HStack)
// ============================================================

TEST(Layout_wrap, no_wrap_overflows) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = false; // Default

    auto c1 = makeBox(60, 20);
    auto c2 = makeBox(60, 20);
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    // Senza wrap, c2 viene posizionato a x=60, sforando la larghezza di 100px.
    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    CHECK_NEAR(c2->getRect().x, 60.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 0.0f, 1e-4);
}

TEST(Layout_wrap, wrap_moves_to_next_line) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;

    auto c1 = makeBox(60, 20);
    auto c2 = makeBox(60, 20);
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    // Con wrap, c2 (60+60=120 > 100) deve andare a capo sulla riga successiva.
    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 20.0f, 1e-4); // Spostato in giù dall'altezza di c1
}

TEST(Layout_wrap, wrap_measures_correct_height) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    // FIX: Usiamo Auto() per larghezza e altezza, così measure restituisce
    // l'ingombro reale intrinseco calcolato dal wrap.
    root->size(Auto(), Auto()); 
    root->getInlineBase().wrap = true;
    root->getInlineBase().gap = Px(10);
    
    root->addChild(makeBox(60, 20)); // Riga 1
    root->addChild(makeBox(60, 30)); // Riga 2
    root->addChild(makeBox(60, 40)); // Riga 3
    
    // Passiamo 100 come constraint del parent
    Vec2 s = root->measure(100, 1000);
    
    // Width dev'essere il max degli elementi sulle righe: max(60, 60, 60) = 60
    CHECK_NEAR(s.x, 60.0f, 1e-4);
    // Height dev'essere la somma delle altezze di riga + gapY: 20 + 10 + 30 + 10 + 40 = 110
    CHECK_NEAR(s.y, 110.0f, 1e-4);
}

TEST(Layout_wrap, wrap_multiple_lines_with_gap) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().gap = Px(10); // gapX e gapY = 10

    auto c1 = makeBox(40, 20);
    auto c2 = makeBox(40, 20);
    auto c3 = makeBox(40, 20);
    
    root->addChild(c1);
    root->addChild(c2);
    root->addChild(c3);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    // Riga 1: c1 (40) + gap (10) + c2 (40) = 90 <= 100.
    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 50.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 0.0f, 1e-4);

    // Riga 2: c3 va a capo. Y = line1_height (20) + gapY (10) = 30.
    CHECK_NEAR(c3->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c3->getRect().y, 30.0f, 1e-4);
}

TEST(Layout_wrap, wrap_justify_center) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().justify = Justify::Center;

    // FIX: c1 (60) + c2 (80) = 140 > 100! Ora sono forzati ad andare a capo per forza.
    // Riga 1: box da 60px. Spazio libero = 40. Offset per Center = 20.
    auto c1 = makeBox(60, 20);
    // Riga 2: box da 80px. Spazio libero sulla riga = 20. Offset per Center = 10.
    auto c2 = makeBox(80, 20);
    
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    CHECK_NEAR(c1->getRect().x, 20.0f, 1e-4);
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    CHECK_NEAR(c2->getRect().x, 10.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 20.0f, 1e-4);
}

TEST(Layout_wrap, wrap_cross_axis_alignment) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(100), Auto());
    root->getInlineBase().wrap = true;
    root->getInlineBase().itemsV = Align::Center;

    // Riga 1: c1 è alto 40px, c2 è alto 20px. Entrambi sulla prima riga (40+40=80 < 100). 
    // Altezza riga massima = 40.
    auto c1 = makeBox(40, 40);
    auto c2 = makeBox(40, 20);
    
    root->addChild(c1);
    root->addChild(c2);

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    // c1 detta l'altezza di riga (40). Y deve essere 0.
    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    
    // c2 è alto 20px. Centrato nella riga di 40px -> (40 - 20) / 2 = 10.
    CHECK_NEAR(c2->getRect().y, 10.0f, 1e-4);
}