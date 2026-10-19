// ZenitUI — Example 11: ref and refAll
//
// @ref is a logic-only identifier. Unlike #id, it does NOT
// participate in CSS specificity, and it can be shared by multiple
// nodes. This example shows:
//
//   1. `refAll<Button>("delete")` — wire every delete button in a
//      list with one loop, without giving each one a unique #id.
//   2. `ref<Text>("counter")` — update a single label via ref.
//   3. Dynamic add/delete — refs keep working when the tree changes.
//
// The contrast with #id is the point:
//   - #list, #add, #reset are style ids. They are unique elements
//     that need to be styled specifically.
//   - @delete is a logic ref. Three (or three hundred) buttons share
//     it: same style via .btn-danger, same C++ handle via refAll.
//
// Build:   make examples
// Run:     make run-11_ref_list

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>
#include <string>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// Initial tree. Every delete button has @delete, every item shares
// the same .item style, the counter has @counter, and the two
// control buttons use #id because they're unique.
static const char *kInitialMarkup = R"(
    VStack#root {
        VStack.card {
            HStack.header {
                Text "Items:"
                Text.counter@counter "3"
            }
            VStack#list {
                HStack.item {
                    Text.item-label "Item 1"
                    Button.btn-danger@delete "X"
                }
                HStack.item {
                    Text.item-label "Item 2"
                    Button.btn-danger@delete "X"
                }
                HStack.item {
                    Text.item-label "Item 3"
                    Button.btn-danger@delete "X"
                }
            }
            HStack.footer {
                Button#add "Add item"
                Button#reset "Reset"
            }
        }
    }
)";

// Tiny wrapper that keeps the @counter label in sync with the
// number of rows in #list. A subclass is required because onUpdate
// is a virtual method, not an assignable field. In a real app this
// could be replaced by Bindable<T> (Proposal 2), which would make
// the whole wrapper unnecessary.
class CounterSync : public TLayout<CounterSync>
{
public:
    CounterSync(std::shared_ptr<Layout> list, std::shared_ptr<Text> counter)
        : TLayout<CounterSync>(LayoutType::Stack),
          list_(std::move(list)), counter_(std::move(counter))
    {
        setStyleTag("CounterSync");
        setSize(Percent(100), Percent(100));
    }

protected:
    void onUpdate(float) override
    {
        if (!list_ || !counter_)
            return;
        auto n = std::to_string(list_->children.size());
        if (counter_->getText() != n)
            counter_->setText(n);
    }

private:
    std::shared_ptr<Layout> list_;
    std::shared_ptr<Text> counter_;
};

int main()
{
    InitWindow(560, 480, "ZenitUI — 11 ref");
    SetTargetFPS(60);

    RaylibRenderer renderer;
    RaylibPlatform platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets = &assets;

    ZMarkup::loadStyleString(R"(
        #root {
            width: 100%;
            height: 100%;
            background: #1a1a1e;
            items-h: center;
            justify: center;
        }
        .card {
            background: #2a2a32;
            radius: 8px;
            padding: 16px;
            gap: 12px;
            min-width: 320px;
        }
        .header {
            gap: 8px;
            items-v: center;
        }
        .counter {
            color: #4a9eff;
            font-size: 18px;
        }
        #list {
            gap: 6px;
        }
        .item {
            background: #333;
            radius: 4px;
            padding: 8px 12px;
            gap: 12px;
            items-v: center;
        }
        .item-label {
            grow: 1;
        }
        .footer {
            gap: 8px;
            justify: end;
        }
        Button {
            background: #4a9eff;
            radius: 4px;
            padding: 6px 12px;
        }
        Button:hover { background: #7bc0ff; }
        .btn-danger { background: #be2137; }
        .btn-danger:hover { background: #e62937; }
    )");

    // Build the initial tree.
    auto ui = ZMarkup::build(kInitialMarkup);
    if (!ui.root())
    {
        std::fprintf(stderr, "Failed to build initial markup\n");
        CloseWindow();
        return 1;
    }

    auto list = ui.find<Layout>("list");      // by #id
    auto counter = ui.ref<Text>("counter");   // by @ref
    auto addBtn = ui.find<Button>("add");     // by #id
    auto resetBtn = ui.find<Button>("reset"); // by #id

    if (!list || !counter || !addBtn || !resetBtn)
    {
        std::fprintf(stderr, "Missing node handles — check the markup\n");
        CloseWindow();
        return 1;
    }

    // Wire a delete button: capture the row (the button's parent) as
    // a weak_ptr. The callback runs on click, and the row is removed
    // from its parent. The weak_ptr avoids a cycle between the row
    // and its own button.
    auto wireDelete = [](std::shared_ptr<Button> btn)
    {
        if (!btn)
            return;
        std::weak_ptr<Button> weakBtn = btn;
        btn->onClick = [weakBtn]()
        {
            if (auto b = weakBtn.lock())
                if (auto row = b->getParent())
                    row->removeFromParent();
        };
    };

    // ---- The core of the example ----
    // Wire EVERY button with @delete in one loop. No unique id per
    // button, no per-row callback. Compare with the id-based
    // alternative: #delete_1, #delete_2, #delete_3, plus three
    // separate `find` calls.
    for (auto &btn : ui.refAll<Button>("delete"))
        wireDelete(btn);

    // Build one item from a small markup fragment and wire its
    // delete button. Used by both "Add" and "Reset". Without
    // UIPrefab, each call re-parses the fragment — acceptable for
    // 1–3 items, not for a 1000-item list.
    auto makeItem = [wireDelete](int n) -> std::shared_ptr<Layout>
    {
        std::string markup =
            "HStack.item {"
            "  Text.item-label \"Item " +
            std::to_string(n) + "\""
                                "  Button.btn-danger@delete \"X\""
                                "}";
        auto frag = ZMarkup::build(markup);
        auto itemRoot = frag.root();
        if (auto btn = frag.ref<Button>("delete"))
            wireDelete(btn);
        return itemRoot;
    };

    addBtn->onClick = [list, makeItem]()
    {
        int n = (int)list->children.size() + 1;
        list->addChild(makeItem(n));
    };

    resetBtn->onClick = [list, makeItem]()
    {
        list->removeAllChildren();
        for (int i = 1; i <= 3; ++i)
            list->addChild(makeItem(i));
    };

    // Wrap the ZMarkup tree in the counter-sync node. The wrapper
    // runs the frame loop; the ZMarkup tree is its child.
    auto wrapper = CounterSync::create(list, counter);
    wrapper->addChild(ui.root());

    while (!WindowShouldClose())
    {
        wrapper->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(::BLACK);
        wrapper->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}