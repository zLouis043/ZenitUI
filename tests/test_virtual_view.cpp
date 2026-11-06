#include "TestFramework.hpp"
#include "Mocks.hpp"
#include "ZenitUIComponents.hpp"

using namespace ZenitUI;
using namespace ZenitUI::Test;
using namespace ZenitUI::UI;

namespace {
    std::shared_ptr<Layout> makeUniformRow()
    {
        return std::make_shared<Layout>(LayoutType::Stack);
    }

    std::shared_ptr<Layout> makeVariableRow(int i)
    {
        auto row = std::make_shared<Layout>(LayoutType::Stack);
        row->size(Px(100), Px(i % 2 == 0 ? 24.0f : 48.0f));
        return row;
    }
}

TEST(VirtualView, item_count_get_set)
{
    Env env;
    auto v = VirtualView::create(makeUniformRow, 100);
    CHECK(v->getItemCount() == 100);
    v->setItemCount(50);
    CHECK(v->getItemCount() == 50);
    v->setItemCount(-5);
    CHECK(v->getItemCount() == 0);
}

TEST(VirtualView, uniform_height_get_set)
{
    Env env;
    auto v = VirtualView::create(makeUniformRow, 100);
    CHECK(v->getUniformHeight() == 0.0f);
    v->setUniformHeight(32.0f);
    CHECK(v->getUniformHeight() == 32.0f);
}

TEST(VirtualView, first_visible_uniform_at_top)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 100);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    CHECK(v->getFirstVisibleItem() == 0);
    CHECK(v->getLastVisibleItem() == 9);
}

TEST(VirtualView, first_visible_uniform_after_scroll)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 100);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    v->scrollToY(240.0f);
    env.frame(root);
    env.frame(root);

    CHECK(v->getFirstVisibleItem() == 10);
    CHECK(v->getLastVisibleItem() == 19);
}

TEST(VirtualView, zero_items)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 0);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    CHECK(v->getFirstVisibleItem() == -1);
    CHECK(v->getLastVisibleItem()  == -1);
}

TEST(VirtualView, content_size_uniform)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 100);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    CHECK_NEAR(v->getMaxScrollY(), 2160.0f, 1.0f);
}

TEST(VirtualView, scroll_to_item_uniform)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 1000);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    v->scrollToItem(500);
    env.frame(root);

    CHECK(v->getFirstVisibleItem() <= 500);
    CHECK(v->getLastVisibleItem()  >= 500);
}

TEST(VirtualView, auto_height_measure_variable)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeVariableRow, 100);
    v->size(Percent(100), Px(240));
    root->addChild(v);
    env.frame(root);

    // 50 righe da 24 + 50 da 48 = 1200 + 2400 = 3600
    CHECK_NEAR(v->getMaxScrollY(), 3600.0f - 240.0f, 1.0f);
}

TEST(VirtualView, auto_first_visible_at_top)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeVariableRow, 100);
    v->size(Percent(100), Px(240));
    root->addChild(v);
    env.frame(root);

    CHECK(v->getFirstVisibleItem() == 0);
    // Con 24/48 alternati, cumulative = [0, 24, 72, 96, 144, 168, 216, 240, ...]
    // itemAtY(239.999) = 6 (item 7 parte a 240)
    CHECK(v->getLastVisibleItem() == 6);
}

TEST(VirtualView, materialized_count_bounded)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 10000);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    // 10 visibili (0..9) + 0 buffer sopra + 2 buffer sotto = 12
    CHECK(v->children.size() == 12);
}

TEST(VirtualView, materialized_dropped_when_out_of_range)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    std::vector<int> createdBefore;
    std::vector<int> createdAfter;
    bool phase2 = false;

    auto v = VirtualView::create(makeUniformRow, 10000);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    v->onItemCreated = [&](Layout*, int idx) {
        if (phase2) createdAfter.push_back(idx);
        else        createdBefore.push_back(idx);
    };
    root->addChild(v);
    env.frame(root);

    CHECK(createdBefore.size() == 12);
    CHECK(std::find(createdBefore.begin(), createdBefore.end(), 0) != createdBefore.end());
    CHECK(std::find(createdBefore.begin(), createdBefore.end(), 9) != createdBefore.end());

    phase2 = true;
    v->scrollToY(1000.0f * 24.0f);
    env.frame(root);
    env.frame(root);

    for (int idx : createdAfter)
    {
        CHECK(idx >= 998);
        CHECK(idx <= 1011);
    }
    CHECK(!createdAfter.empty());
    CHECK(v->getFirstVisibleItem() >= 998);
    CHECK(v->getFirstVisibleItem() <= 1000);
}

TEST(VirtualView, on_item_created_fires_for_materialized)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    std::vector<int> created;
    auto v = VirtualView::create(makeUniformRow, 1000);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    v->onItemCreated = [&](Layout*, int idx) { created.push_back(idx); };
    root->addChild(v);
    env.frame(root);

    CHECK(created.size() == 12);
    CHECK(std::find(created.begin(), created.end(), 0) != created.end());
    CHECK(std::find(created.begin(), created.end(), 9) != created.end());
}

TEST(VirtualView, removeAllChildren_clears_materialized)
{
    Env env;
    auto root = std::make_shared<Layout>(LayoutType::Stack);
    root->size(Percent(100), Percent(100));

    auto v = VirtualView::create(makeUniformRow, 1000);
    v->size(Percent(100), Px(240));
    v->setUniformHeight(24.0f);
    root->addChild(v);
    env.frame(root);

    CHECK(v->children.size() == 12);

    v->removeAllChildren();
    CHECK(v->children.empty());

    env.frame(root);
    CHECK(v->children.size() == 12);
}

TEST(VirtualView, first_visible_before_measure_is_minus_one)
{
    Env env;
    auto v = VirtualView::create(makeUniformRow, 1000);

    CHECK(v->getFirstVisibleItem() == -1);
    CHECK(v->getLastVisibleItem()  == -1);
}
