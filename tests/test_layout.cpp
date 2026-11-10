#include "TestFramework.hpp"
#include "Layout.hpp"
#include "Theme.hpp"
#include "ZMarkup.hpp"

using namespace ZenitUI;

static void freshTheme() {
    Theme::get().clear();
}


TEST(Layout_measure, empty_stack_zero) {
    freshTheme();
    auto l = std::make_shared<Layout>(LayoutType::Stack);
    Vec2 s = l->measure(1000, 500);
    CHECK_NEAR(s.x, 0.0f, 1e-4);
    CHECK_NEAR(s.y, 0.0f, 1e-4);
}

TEST(Layout_measure, fixed_size) {
    freshTheme();
    auto l = std::make_shared<Layout>(LayoutType::Stack);
    l->size(Px(100), Px(50));
    Vec2 s = l->measure(1000, 500);
    CHECK_NEAR(s.x, 100.0f, 1e-4);
    CHECK_NEAR(s.y, 50.0f, 1e-4);
}

TEST(Layout_measure, percent_size) {
    freshTheme();
    auto l = std::make_shared<Layout>(LayoutType::Stack);
    l->size(Percent(50), Percent(25));
    Vec2 s = l->measure(1000, 400);
    CHECK_NEAR(s.x, 500.0f, 1e-4);
    CHECK_NEAR(s.y, 100.0f, 1e-4);
}

TEST(Layout_measure, stack_from_children) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(100), Px(50));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(80),  Px(70));
    root->addChild(c1);
    root->addChild(c2);
    Vec2 s = root->measure(1000, 500);
    CHECK_NEAR(s.x, 100.0f, 1e-4);
    CHECK_NEAR(s.y, 70.0f,  1e-4);
}

TEST(Layout_measure, vertical_sums_heights) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(50), Px(20));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(50), Px(30));
    root->addChild(c1);
    root->addChild(c2);
    Vec2 s = root->measure(1000, 500);
    CHECK_NEAR(s.y, 50.0f, 1e-4);
    CHECK_NEAR(s.x, 50.0f, 1e-4);
}

TEST(Layout_measure, horizontal_sums_widths) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(30), Px(20));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(50), Px(20));
    root->addChild(c1);
    root->addChild(c2);
    Vec2 s = root->measure(1000, 500);
    CHECK_NEAR(s.x, 80.0f, 1e-4);
}


TEST(Layout_arrange, root_fills_space) {
    freshTheme();
    auto l = std::make_shared<Layout>(LayoutType::Stack);
    l->size(Percent(100), Percent(100));
    l->measure(800, 600);
    l->arrange({0, 0, 800, 600});
    auto r = l->getRect();
    CHECK_NEAR(r.x, 0.0f, 1e-4);
    CHECK_NEAR(r.y, 0.0f, 1e-4);
    CHECK_NEAR(r.width, 800.0f, 1e-4);
    CHECK_NEAR(r.height, 600.0f, 1e-4);
}

TEST(Layout_arrange, vertical_stacks_children) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Vertical);
    root->size(Px(200), Auto());
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(100), Px(30));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(100), Px(40));
    root->addChild(c1);
    root->addChild(c2);

    root->measure(1000, 1000);
    root->arrange({0, 0, 200, 100});

    CHECK_NEAR(c1->getRect().y, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().height, 30.0f, 1e-4);
    CHECK_NEAR(c2->getRect().y, 30.0f, 1e-4);
    CHECK_NEAR(c2->getRect().height, 40.0f, 1e-4);
}

TEST(Layout_arrange, horizontal_places_side_by_side) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(200), Auto());
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(50), Px(30));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(70), Px(30));
    root->addChild(c1);
    root->addChild(c2);

    root->measure(1000, 1000);
    root->arrange({0, 0, 200, 100});

    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c1->getRect().width, 50.0f, 1e-4);
    CHECK_NEAR(c2->getRect().x, 50.0f, 1e-4);
    CHECK_NEAR(c2->getRect().width, 70.0f, 1e-4);
}

TEST(Layout_arrange, gap_separates_children) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(200), Auto());
    root->getInlineBase().gap = Px(10);
    auto c1 = std::make_shared<Layout>(LayoutType::Stack); c1->size(Px(50), Px(30));
    auto c2 = std::make_shared<Layout>(LayoutType::Stack); c2->size(Px(50), Px(30));
    root->addChild(c1);
    root->addChild(c2);

    root->measure(1000, 1000);
    root->arrange({0, 0, 200, 100});

    CHECK_NEAR(c1->getRect().x, 0.0f, 1e-4);
    CHECK_NEAR(c2->getRect().x, 60.0f, 1e-4);
}


TEST(Layout_hitTest, point_inside_root) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(100), Px(100));
    root->measure(1000, 1000);
    root->arrange({0, 0, 100, 100});

    CHECK(root->hitTest({50, 50}, false) == root.get());
}

TEST(Layout_hitTest, point_outside_root) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(100), Px(100));
    root->measure(1000, 1000);
    root->arrange({0, 0, 100, 100});

    CHECK(root->hitTest({200, 200}, false) == nullptr);
}

TEST(Layout_hitTest, child_wins_over_parent) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(200), Px(200));
    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    root->addChild(child);

    root->measure(1000, 1000);
    root->arrange({0, 0, 200, 200});

    CHECK(root->hitTest({25, 25}, false) == child.get());
    CHECK(root->hitTest({100, 100}, false) == root.get());
}

TEST(Layout_hitTest, disabled_node_not_hit) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(100), Px(100));
    root->measure(1000, 1000);
    root->arrange({0, 0, 100, 100});
    root->setEnabled(false);

    CHECK(root->hitTest({50, 50}, false) == nullptr);
}

TEST(Layout_dirtyTracking, measure_returns_cached_when_nothing_changed) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(200), Px(100));
    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    root->addChild(child);

    Vec2 s1 = root->measure(1000, 1000);
    CHECK_NEAR(s1.x, 200.0f, 1e-4);

    Vec2 s2 = root->measure(1000, 1000);
    CHECK_NEAR(s2.x, s1.x, 1e-6);
    CHECK_NEAR(s2.y, s1.y, 1e-6);
}

TEST(Layout_dirtyTracking, size_change_triggers_remeasure) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(200), Px(100));
    root->measure(1000, 1000);

    root->size(Px(300), Px(150));

    Vec2 s = root->measure(1000, 1000);
    CHECK_NEAR(s.x, 300.0f, 1e-4);
}

TEST(Layout_dirtyTracking, size_change_invalidates_cache) {
    freshTheme();
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(200), Px(100));

    Vec2 s1 = root->measure(1000, 1000);
    CHECK_NEAR(s1.x, 200.0f, 1e-4);

    root->size(Px(300), Px(150));
    Vec2 s2 = root->measure(1000, 1000);
    CHECK_NEAR(s2.x, 300.0f, 1e-4);
    CHECK_NEAR(s2.y, 150.0f, 1e-4);
}

TEST(Layout_hitTest, clipped_area_not_hit) {
    freshTheme();

    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Px(500), Px(200));

    auto clipper = std::make_shared<Layout>(LayoutType::Stack);
    clipper->size(Px(200), Px(100));
    clipper->getInlineBase().overflowX = Overflow::Hidden;
    clipper->getInlineBase().overflowY = Overflow::Hidden;

    auto inner = std::make_shared<Layout>(LayoutType::Stack);
    inner->size(Px(400), Px(100));
    inner->setInteractive(true);
    inner->setBlocksRaycast(true);
    clipper->addChild(inner);
    root->addChild(clipper);

    root->measure(1000, 1000);
    root->arrange({0, 0, 500, 200});
    clipper->measure(500, 200);
    clipper->arrange({0, 0, 200, 100});
    inner->measure(200, 100);
    inner->arrange({0, 0, 400, 100});

    CHECK(root->hitTest({50, 50}, false) == inner.get());

    CHECK(root->hitTest({300, 50}, false) != inner.get());
}

TEST(Layout_hasId, true_when_matching)
{
    freshTheme();
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->setId("save-button");

    CHECK(node->hasId("save-button"));
    CHECK(!node->hasId("cancel-button"));
}

TEST(Layout_hasId, false_when_no_id)
{
    freshTheme();
    auto node = std::make_shared<Layout>(LayoutType::Stack);

    CHECK(!node->hasId("anything"));
    CHECK(!node->hasId(""));
}

TEST(Layout_hasId, empty_input_always_false)
{
    freshTheme();
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->setId("");

    CHECK(!node->hasId(""));
}


TEST(Layout_ref, setRef_updates_field)
{
    freshTheme();
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    CHECK(node->ref.empty());
    node->setRef("delete");
    CHECK(node->ref == "delete");
}

TEST(Layout_ref, id_and_ref_are_independent)
{
    freshTheme();
    auto node = std::make_shared<Layout>(LayoutType::Stack);
    node->setId("unique");
    node->setRef("delete");
    CHECK(node->nodeId == "unique");
    CHECK(node->ref == "delete");
    node->setId("other");
    CHECK(node->ref == "delete");
    node->setRef("other_ref");
    CHECK(node->nodeId == "other");
}

TEST(Layout_ref, withRef_returns_fluent_shared_ptr)
{
    freshTheme();
    auto btn = UI::Button::create(nullptr);
    CHECK(btn->ref.empty());

    auto result = btn->withRef("save");
    CHECK(result.get() == btn.get());
    CHECK(btn->ref == "save");
}

TEST(Layout_ref, withRef_is_chainable)
{
    freshTheme();
    auto btn = UI::Button::create(nullptr);
    auto chained = btn->withRef("save")->cls("btn-primary");
    CHECK(btn->ref == "save");
    CHECK(btn->hasClass("btn-primary"));
    CHECK(chained.get() == btn.get());
}

TEST(Layout_arrange, border_subtracted_from_content_box)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->getInlineBase().width      = Px(200);
    parent->getInlineBase().height     = Px(200);
    parent->getInlineBase().padding    = Spacing(5);
    parent->getInlineBase().borderWidth = Px(10);

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->getInlineBase().width  = Px(50);
    child->getInlineBase().height = Px(50);
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // inner = 200 - 5 - 5 - 10 - 10 = 170, offset 5 + 10 = 15
    CHECK_NEAR(child->getRect().x, 15.0f, 1e-4);
    CHECK_NEAR(child->getRect().y, 15.0f, 1e-4);
}

TEST(Layout_measure, border_added_to_auto_width)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->getInlineBase().padding     = Spacing(5);
    parent->getInlineBase().borderWidth = Px(10);

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->getInlineBase().width  = Px(100);
    child->getInlineBase().height = Px(100);
    parent->addChild(child);

    Vec2 sz = parent->measure(1000, 1000);

    // content = 100, padding 10, border 20 → 130
    CHECK_NEAR(sz.x, 130.0f, 1e-4);
    CHECK_NEAR(sz.y, 130.0f, 1e-4);
}

TEST(Layout_align, align_items_explicit_start_on_vstack)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));
    parent->getInlineBase().alignItems = Align::Start;

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    CHECK_NEAR(child->getRect().x, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().width, 50.0f, 0.5f);
}

TEST(Layout_align, align_items_center_on_vstack)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));
    parent->getInlineBase().alignItems = Align::Center;

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Child 50 wide, parent 200: centered at x = 75.
    CHECK_NEAR(child->getRect().x, 75.0f, 0.5f);
}

TEST(Layout_align, deprecated_items_h_on_hstack_is_noop)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Horizontal);
    parent->size(Px(200), Px(200));
    parent->getInlineBase().alignItems = Align::Start;   // explicit

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    child->getInlineBase().itemsH = Align::Center;   // wrong parent type
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // items-h on child of HStack: no derivation, no-op. With explicit
    // Start, child stays at its natural size.
    CHECK_NEAR(child->getRect().y, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().height, 50.0f, 0.5f);
}

TEST(Layout_align, default_is_stretch_on_vstack)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    // No explicit width: expect stretch to 200.
    child->size(Auto(), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    CHECK_NEAR(child->getRect().x, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().width, 200.0f, 0.5f);
}

TEST(Layout_align, default_is_stretch_on_hstack)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Horizontal);
    parent->size(Px(200), Px(200));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    // No explicit height: expect stretch to 200.
    child->size(Px(50), Auto());
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    CHECK_NEAR(child->getRect().y, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().height, 200.0f, 0.5f);
}

TEST(Layout_align, default_is_start_on_stack)
{
    freshTheme();
    // Stack children do NOT stretch: Stack is a free-positioning
    // container, not a flex container. CSS analogy: a position: relative
    // div does not stretch its children.
    auto parent = std::make_shared<Layout>(LayoutType::Stack);
    parent->size(Px(200), Px(200));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    CHECK_NEAR(child->getRect().x, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().y, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().width, 50.0f, 0.5f);
    CHECK_NEAR(child->getRect().height, 50.0f, 0.5f);
}

TEST(Layout_align, explicit_align_items_stretch_on_stack)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Stack);
    parent->size(Px(200), Px(200));
    parent->getInlineBase().alignItems = Align::Stretch;

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    CHECK_NEAR(child->getRect().width, 200.0f, 0.5f);
    CHECK_NEAR(child->getRect().height, 200.0f, 0.5f);
}

TEST(Layout_border, per_side_borders)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));
    parent->getInlineBase().borderWidth =
        Spacing(Px(5), Px(10), Px(15), Px(20));  // top, right, bottom, left

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // inner = {left=20, top=5, w=170, h=180}, child at (20, 5)
    CHECK_NEAR(child->getRect().x, 20.0f, 0.5f);
    CHECK_NEAR(child->getRect().y, 5.0f, 0.5f);
}

TEST(Layout_border, measure_with_per_side_borders)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->getInlineBase().borderWidth =
        Spacing(Px(5), Px(10), Px(15), Px(20));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->getInlineBase().width  = Px(100);
    child->getInlineBase().height = Px(100);
    parent->addChild(child);

    Vec2 sz = parent->measure(1000, 1000);

    // w = 100 + (left 20 + right 10) = 130
    // h = 100 + (top 5 + bottom 15)   = 120
    CHECK_NEAR(sz.x, 130.0f, 1e-4);
    CHECK_NEAR(sz.y, 120.0f, 1e-4);
}

TEST(Layout_border, uniform_border_renders_rounded)
{
    // Smoke test: no crash, no assert. Corner rounding is verified
    // visually in examples/07_effects.
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(100), Px(100));
    parent->getInlineBase().borderWidth = Px(2);
    parent->getInlineBase().radius      = Px(8);
    parent->getInlineBase().borderColor = Color{255, 0, 0, 255};
    parent->measure(100, 100);
    parent->arrange({0, 0, 100, 100});
    CHECK_NEAR(parent->getRect().width, 100.0f, 0.5f);
}

TEST(Layout_flex, min_height_auto_prevents_collapse)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(100));

    auto title = std::make_shared<Layout>(LayoutType::Stack);
    title->size(Px(200), Px(50));
    parent->addChild(title);

    auto content = std::make_shared<Layout>(LayoutType::Stack);
    content->size(Auto(), Px(200));
    parent->addChild(content);

    parent->measure(200, 100);
    parent->arrange({0, 0, 200, 100});

    // Total children = 250, parent = 100, freeSpace = -150.
    // Both items have shrink: 1, but overflow: visible floors each at
    // its content size, so neither collapses to 0.
    CHECK(title->getRect().height >= 50.0f - 0.5f);
    CHECK(content->getRect().height >= 200.0f - 0.5f);
}

TEST(Layout_flex, min_height_auto_respects_overflow_hidden)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(100));

    auto header = std::make_shared<Layout>(LayoutType::Stack);
    header->size(Px(200), Px(50));
    header->getInlineBase().overflowY = Overflow::Hidden;
    parent->addChild(header);

    auto body = std::make_shared<Layout>(LayoutType::Stack);
    body->size(Auto(), Px(200));
    parent->addChild(body);

    parent->measure(200, 100);
    parent->arrange({0, 0, 200, 100});

    // header: overflow hidden -> floor at 0, can shrink freely.
    // body: overflow visible -> floor at 200, does not collapse.
    CHECK(header->getRect().height < 50.0f - 0.5f);
    CHECK(body->getRect().height >= 200.0f - 0.5f);
}

TEST(Layout_flex, min_width_auto_prevents_collapse)
{
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Horizontal);
    parent->size(Px(100), Px(200));

    auto left = std::make_shared<Layout>(LayoutType::Stack);
    left->size(Px(80), Px(200));
    parent->addChild(left);

    auto right = std::make_shared<Layout>(LayoutType::Stack);
    right->size(Px(200), Px(200));
    parent->addChild(right);

    parent->measure(100, 200);
    parent->arrange({0, 0, 100, 200});

    // Total children = 280, parent = 100, freeSpace = -180.
    // Both items have overflow: visible -> floored at their content
    // width, neither collapses.
    CHECK(left->getRect().width >= 80.0f - 0.5f);
    CHECK(right->getRect().width >= 200.0f - 0.5f);
}

TEST(Layout_align, items_h_on_child_does_not_affect_self_alignment)
{
    // itemsH is a deprecated alias for align-items, which controls how
    // a node aligns its OWN children. It must NOT be interpreted as an
    // align-self hint on the node itself. This was the bug introduced
    // in §5.25 and fixed here.
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    child->getInlineBase().itemsH = Align::Center;
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Parent has no explicit align-items -> Stretch default.
    // Child does not derive alignH from itemsH -> stretched.
    CHECK_NEAR(child->getRect().x, 0.0f, 0.5f);
    CHECK_NEAR(child->getRect().width, 200.0f, 0.5f);
}

TEST(Layout_align, explicit_align_h_on_child_overrides_parent_stretch)
{
    // The correct way to align a single child is align-h / align-v
    // (align-self), not align-items / items-h on the child.
    freshTheme();
    auto parent = std::make_shared<Layout>(LayoutType::Vertical);
    parent->size(Px(200), Px(200));

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Px(50), Px(50));
    child->getInlineBase().alignH = Align::End;   // align-self
    parent->addChild(child);

    parent->measure(200, 200);
    parent->arrange({0, 0, 200, 200});

    // Child at x = 150 (200 - 50), End-aligned.
    CHECK_NEAR(child->getRect().x, 150.0f, 0.5f);
}

TEST(Layout_flex, flex_shorthand_distributes_space_and_preserves_child_percent)
{
    auto root = std::make_shared<Layout>(LayoutType::Horizontal);
    root->size(Px(1000.0f), Px(100.0f));

    auto fixed = std::make_shared<Layout>(LayoutType::Stack);
    fixed->size(Px(200.0f), Px(100.0f));

    auto flexNode = std::make_shared<Layout>(LayoutType::Stack);
    flexNode->getInlineBase().grow = 1.0f;
    flexNode->getInlineBase().shrink = 1.0f;
    flexNode->getInlineBase().flexBasis = Px(0.0f);

    auto child = std::make_shared<Layout>(LayoutType::Stack);
    child->size(Percent(100.0f), Percent(100.0f));
    flexNode->addChild(child);

    root->addChild(fixed);
    root->addChild(flexNode);

    root->measure(1000.0f, 100.0f);
    root->arrange({0.0f, 0.0f, 1000.0f, 100.0f});

    CHECK_NEAR(flexNode->getRect().width, 800.0f, 1e-3);
    
    CHECK_NEAR(child->getRect().width, 800.0f, 1e-3);
}