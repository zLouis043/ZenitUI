// ZenitUI — Example 12: Bindable
//
// A model value can be shared by many widgets without callback glue.
//
//   - Bindable<float>       volume       → N ProgressBars
//   - Bindable<std::string> trackName    → Label
//   - Bindable<bool>        muted        → Label
//
// The example also demonstrates lifetime safety: "Remove" destroys the
// last ProgressBar, but the Subscription that bound it to `volume` is
// dropped together with it (or, if it survived, it would be a no-op
// because its weak_ptr expired). "Add" creates a fresh bar with
// fireImmediately, so it reflects the current volume right away.
//
// Compare with the "before" pattern, which would look like:
//
//   slider->onValueChanged = [progress, label, audio](float v) {
//       progress->setValue(v);
//       label->setText(...);
//       audio.setMasterVolume(v);
//   };
//
// Adding a fourth consumer means editing the callback. Destroying
// `progress` first means the callback dereferences freed memory.
// With Bindable, each consumer is an independent, RAII subscription.
//
// Build:   make examples
// Run:     make run-12_bindable

#include <raylib.h>

#include "RaylibBackend.hpp"
#include "UI.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::UI;

static const char* kMarkup = R"(
    VStack#root {
        VStack.card {
            Text.title "Audio"
            HStack.row {
                Text.label "Volume"
                Slider#volumeSlider value=0.5 width="70%"
            }
            HStack.row {
                Text.label "Track"
                TextInput#track value="Track A" width="40%"
                Text.value#trackLabel wrap "Now playing: Track A"
            }
            HStack.row {
                Text.label "Muted"
                Toggle#mute checked=false
                Text.value#muteLabel "Unmuted"
            }
            Text.desc "Bars (add / remove from the panel below):"
            VStack#bars {
            }
        }
        VStack.card {
            Text.title "Lifetime demo"
            Text.desc wrap "Click 'Remove' to destroy the last bar. Click 'Add' to create a new one — it reflects the current volume immediately. The slider keeps working regardless of how many bars exist."
            HStack.actions {
                Button#remove "Remove last bar"
                Button#add    "Add bar"
            }
        }
    }
)";

int main()
{
    InitWindow(820, 640, "ZenitUI — 12 Bindable");
    SetTargetFPS(60);

    RaylibRenderer      renderer;
    RaylibPlatform      platform;
    RaylibAssetProvider assets(renderer);

    UIContext::get().renderer = &renderer;
    UIContext::get().platform = &platform;
    UIContext::get().assets   = &assets;

    ZMarkup::loadStyleString(R"(
        #root {
            width: 100%;
            height: 100%;
            background: #1a1a1e;
            items-h: center;
            justify: center;
            gap: 16px;
        }
        .card {
            background: #2a2a32;
            radius: 8px;
            padding: 16px;
            gap: 10px;
            min-width: 700px;
        }
        .title {
            color: #4a9eff;
            font-size: 18px;
        }
        .desc {
            color: #9090a0;
            font-size: 12px;
        }
        .row {
            gap: 10px;
            items-v: center;
            width: 100%;
        }
        .label {
            color: #9090a0;
            font-size: 14px;
            min-width: 70px;
        }
        .value {
            color: #4a9eff;
            font-size: 14px;
            grow: 1;
        }
        .actions {
            gap: 8px;
            items-v: center;
        }
        Button {
            background: #4a9eff;
            color: #ffffff;
            radius: 4px;
            padding: 6px 12px;
        }
        Button:hover { background: #7bc0ff; }

        /* Vertical list of bars. Scrolls when more than ~4 fit. */
        #bars {
            width: 100%;
            max-height: 160px;
            overflow-y: auto;
            overflow-x: hidden;
            gap: 4px;
        }

        /* Toggle needs explicit sizing: its intrinsic size is 0×0. */
        Toggle {
            width: 50px;
            height: 26px;
            background: #505050;
            radius: 13px;
            transition: background 0.15s linear;
        }
        Toggle:checked { background: #4a9eff; }
        Toggle .toggle-knob {
            position: absolute;
            width: 22px;
            height: 22px;
            top: 2px;
            left: 2px;
            radius: 11px;
            background: #ffffff;
            transition: left 0.18s ease-out-cubic;
        }
        Toggle:checked .toggle-knob { left: 26px; }
    )");

    // ---- Model ----
    Bindable<float>       volume{0.5f};
    Bindable<std::string> trackName{std::string("Track A")};
    Bindable<bool>        muted{false};

    // ---- Build the tree ----
    auto ui = ZMarkup::build(kMarkup);
    if (!ui.root()) {
        std::fprintf(stderr, "Failed to build markup\n");
        CloseWindow();
        return 1;
    }

    auto root          = ui.root();
    auto slider        = ui.find<Slider>("volumeSlider");
    auto trackInput    = ui.find<TextInput>("track");
    auto muteToggle    = ui.find<Toggle>("mute");
    auto trackLabel    = ui.find<Text>("trackLabel");
    auto muteLabel     = ui.find<Text>("muteLabel");
    auto removeBtn     = ui.find<Button>("remove");
    auto addBtn        = ui.find<Button>("add");
    auto barsContainer = ui.find<Layout>("bars");

    if (!slider || !trackInput || !muteToggle ||
        !trackLabel || !muteLabel || !removeBtn || !addBtn || !barsContainer) {
        std::fprintf(stderr, "Missing node handles — check the markup\n");
        CloseWindow();
        return 1;
    }

    // ---- Write side ----
    slider->onValueChanged    = [&volume](float v)                { volume.set(v); };
    trackInput->onTextChanged = [&trackName](const std::string& s){ trackName.set(s); };
    muteToggle->onToggle      = [&muted](bool m)                  { muted.set(m); };

    // ---- Read side ----
    auto subTrack = trackName.onChanged(
        [trackLabel](const std::string& s) {
            trackLabel->setText("Now playing: " + s);
        },
        /*fireImmediately=*/true);

    auto subMute = muted.onChanged(
        [muteLabel](const bool& m) {
            muteLabel->setText(m ? "Muted" : "Unmuted");
        },
        /*fireImmediately=*/true);

    // ---- Bars: one Subscription per bar, paired with its weak_ptr ----
    struct BarEntry {
        std::weak_ptr<ProgressBar> bar;
        Bindable<float>::Subscription sub;
    };
    std::vector<BarEntry> bars;

    auto makeBar = [&volume, &bars, barsContainer]() {
        auto pb = ProgressBar::create(0.0f);
        pb->size(Percent(100), Px(20));
        barsContainer->addChild(pb);

        BarEntry entry;
        entry.bar = pb;
        entry.sub = volume.bindTo(pb, &ProgressBar::setValue,
                                  /*fireImmediately=*/true);
        bars.push_back(std::move(entry));
    };

    // Seed with one initial bar so the demo doesn't open empty.
    makeBar();

    // ---- Lifetime demo ----
    addBtn->onClick = [makeBar]() { makeBar(); };

    removeBtn->onClick = [&bars]() {
        while (!bars.empty()) {
            auto p = bars.back().bar.lock();
            bars.pop_back();          // destroys Subscription (no-op if widget gone)
            if (p) {
                p->removeFromParent();
                return;
            }
        }
    };

    // ---- Frame loop ----
    while (!WindowShouldClose()) {
        root->runFrame(GetFrameTime());

        BeginDrawing();
        ClearBackground(::BLACK);
        root->renderFrame();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}