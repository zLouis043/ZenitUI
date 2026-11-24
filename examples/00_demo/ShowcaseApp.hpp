#pragma once

#include "ZenitUI.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace ZenitUI;
using namespace ZenitUI::UI;

// ============================================================================
//  ShowcaseApp — Flagship ZenitUI multi-view showcase
//
//  Demonstrates how ZenitUI's subsystems work together in a single app:
//    - App shell with live Dark/Light + Accent theme switching (CSS variables)
//    - View 1 (Dashboard): ZMarkup Prefabs + <Slot>, If, keyed Each, Bindable<T>
//    - View 2 (Editor):    Form controls, UTF-8 TextArea, live dirty state, Modal
//    - View 3 (Explorer):  10,000-row VirtualView, live filter, ContextMenu, 9-slice
//    - View 4 (Effects):   @keyframes, transitions, UIAnimation, filter:, CanvasLayout
// ============================================================================
class ShowcaseApp
{
public:
    struct TaskItem
    {
        int id{0};
        std::string tag;
        std::string title;
        bool operator==(const TaskItem &) const = default;
    };

    struct CatalogEntry
    {
        int id{0};
        std::string category;
        std::string message;
    };

    ShowcaseApp()
    {
        seedCatalog();
        registerPrefabs();
        buildShell();
        buildDashboardView();
        buildEditorView();
        buildExplorerView();
        buildEffectsView();
        buildModal();
        switchView(0);
    }

    std::shared_ptr<Layout> root() const { return appRoot_; }

    // Called once per frame from main() to refresh the live FPS/RenderStats box
    // and the VirtualView visible-range readout.
    void tickStats(int fps, float dtMs)
    {
        statsTimer_ += dtMs;
        if (statsTimer_ < 250.0f)
            return;
        statsTimer_ = 0.0f;

        const auto &st = UIContext::get().renderStats;
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "FPS: %d (%.1f ms)\nTargets live: %u  Passes: %u",
                      fps, dtMs, st.liveTargets, st.filterPasses);
        if (shellUi_)
            shellUi_->setTextRef("stats-label", buf);

        updateExplorerRange();
    }

private:
    // ---- Reactive application state (Bindable<T>) -------------------------
    Bindable<bool> dirty_{false};
    Bindable<float> workload_{0.42f};
    Bindable<bool> highWorkload_{false};
    Bindable<std::vector<TaskItem>> tasks_{std::vector<TaskItem>{
        {1, "Layout", "Audit Flexbox wrap & responsive breakpoints"},
        {2, "Styling", "Verify Dark/Light custom property cascade"},
        {3, "Widgets", "Test UTF-8 grapheme editing in TextArea"},
    }};
    int nextTaskId_{4};

    // Editor model defaults
    Bindable<std::string> username_{std::string("zenit_dev")};
    Bindable<float> volume_{0.72f};
    Bindable<int> qualityIdx_{2};
    Bindable<bool> notifications_{true};
    Bindable<bool> cloudSync_{false};

    // Keep RAII subscriptions alive for the lifetime of ShowcaseApp
    std::vector<Bindable<float>::Subscription> floatSubs_;
    std::vector<Bindable<std::vector<TaskItem>>::Subscription> taskSubs_;
    std::vector<Bindable<std::string>::Subscription> strSubs_;

    // Explorer dataset (10,000 items) + active filter indices
    std::vector<CatalogEntry> catalog_;
    std::vector<int> filteredIndices_;
    std::string filterQuery_;
    int contextRowIndex_{-1};

    // Theme state
    bool lightTheme_{false};
    int accentIndex_{0}; // 0: Blue, 1: Violet, 2: Emerald
    float statsTimer_{250.0f};

    // Node handles
    std::shared_ptr<Layout> appRoot_;
    std::optional<ZMarkup::UINode> shellUi_;
    std::optional<ZMarkup::UINode> dashUi_;
    std::optional<ZMarkup::UINode> editorUi_;
    std::optional<ZMarkup::UINode> explorerUi_;
    std::optional<ZMarkup::UINode> effectsUi_;

    std::vector<std::shared_ptr<Layout>> views_;
    std::shared_ptr<TextArea> notesArea_;
    std::shared_ptr<VirtualView> virtualList_;
    std::shared_ptr<Popup> rowContextMenu_;
    std::shared_ptr<Modal> modal_;
    std::shared_ptr<Text> modalTitle_;
    std::shared_ptr<Text> modalBody_;
    std::function<void()> modalConfirmAction_;

    // -----------------------------------------------------------------------
    //  Seed 10,000 deterministic entries for the VirtualView explorer
    // -----------------------------------------------------------------------
    void seedCatalog()
    {
        static const char *kCats[] = {"Layout", "Style", "Input", "Render", "Markup", "Anim"};
        static const char *kMsgs[] = {
            "Resolved cascade with cached selector rules",
            "Reused off-screen layer target from TargetPool",
            "Materialized visible row inside VirtualView overscan",
            "Dispatched pointer capture across stacking context",
            "Reconciled keyed Each slice without rebuilding rows",
            "Completed ease-out-back transform transition",
        };

        catalog_.reserve(10000);
        filteredIndices_.reserve(10000);
        for (int i = 0; i < 10000; ++i)
        {
            catalog_.push_back({
                i + 1,
                kCats[i % 6],
                std::string(kMsgs[(i * 7) % 6]) + " (event #" + std::to_string(i + 1) + ")",
            });
            filteredIndices_.push_back(i);
        }
    }

    // -----------------------------------------------------------------------
    //  Register reusable ZMarkup prefabs (using <Slot> child injection)
    // -----------------------------------------------------------------------
    void registerPrefabs()
    {
        ZMarkup::registerPrefab("KpiCard", R"zm(
            VStack.kpi-card {
                Slot
            }
        )zm");

        ZMarkup::registerPrefab("TaskRow", R"zm(
            HStack.task-row {
                Text.task-tag@tag ""
                Text.task-title@title wrap="true" ""
                Button.btn-danger.btn-sm@done "Done"
            }
        )zm");
    }

    // -----------------------------------------------------------------------
    //  App shell: persistent sidebar + top bar + 4-view content host
    // -----------------------------------------------------------------------
    void buildShell()
    {
        static const char *kShellMarkup = R"zm(
            Stack#app {
                HStack.shell {
                    VStack.sidebar {
                        HStack.brand-row {
                            Text.brand-title "ZenitUI"
                            Text.version-pill "beta"
                        }

                        Text.nav-section "SHOWCASE"
                        Button.nav-btn@nav-0 "1. Dashboard"
                        Button.nav-btn@nav-1 "2. Editor & Forms"
                        Button.nav-btn@nav-2 "3. Virtual Explorer"
                        Button.nav-btn@nav-3 "4. Effects & Motion"

                        Layout.spacer

                        Text.nav-section "APPEARANCE"
                        VStack.sidebar-controls {
                            Button@theme-btn width="100%" "Theme: Dark"
                            Button@accent-btn width="100%" "Accent: Blue"
                        }

                        Text.stats-box@stats-label wrap "FPS: --"
                    }

                    VStack.main-col {
                        HStack.topbar {
                            Text.page-title@page-title wrap grow="1" min-width="0" "Dashboard"
                            If@dirty-badge {
                                Text.pill-warn "Unsaved changes"
                            }
                            Button@about-btn "About"
                        }

                        Stack.content-host@content-host {}
                    }
                }
            }
        )zm";

        shellUi_ = ZMarkup::build(kShellMarkup);
        appRoot_ = shellUi_->root();

        // Bind the "Unsaved changes" badge directly to the dirty_ Bindable<bool>.
        shellUi_->bindCondition("dirty-badge", dirty_, true);

        for (int i = 0; i < 4; ++i)
        {
            shellUi_->onClickRef("nav-" + std::to_string(i), [this, i]
                                 { switchView(i); });
        }

        // Live Dark/Light switch: toggling one class re-evaluates all CSS variables.
        shellUi_->onClickRef("theme-btn", [this]
                             {
            lightTheme_ = !lightTheme_;
            if (lightTheme_)
                appRoot_->addClass("theme-light");
            else
                appRoot_->removeClass("theme-light");

            shellUi_->setButtonTextRef("theme-btn", lightTheme_ ? "Theme: Light" : "Theme: Dark");
            if (dashUi_)
                dashUi_->setTextRef("kpi-theme", lightTheme_ ? "Light" : "Dark"); });

        // Live Accent palette switch: cycles between 3 token sets on #app.
        shellUi_->onClickRef("accent-btn", [this]
                             {
            accentIndex_ = (accentIndex_ + 1) % 3;
            appRoot_->removeClass("accent-violet");
            appRoot_->removeClass("accent-emerald");
            if (accentIndex_ == 1)
                appRoot_->addClass("accent-violet");
            else if (accentIndex_ == 2)
                appRoot_->addClass("accent-emerald");

            static const char *kNames[] = {"Accent: Blue", "Accent: Violet", "Accent: Emerald"};
            shellUi_->setButtonTextRef("accent-btn", kNames[accentIndex_]); });

        shellUi_->onClickRef("about-btn", [this]
                             { openModal(
                                   "ZenitUI Showcase",
                                   "Retained-mode C++20 UI framework featuring Flexbox layout, "
                                   "ZStyle cascade, ZMarkup prefabs & keyed Each, Bindable<T> "
                                   "reactivity, VirtualView, portals, and post-processing filters.",
                                   nullptr); });

        if (auto btn = shellUi_->ref("theme-btn"))
            Tooltip::attach(btn, "Toggles #app.theme-light to re-resolve all CSS variables", 0.35f);
        if (auto btn = shellUi_->ref("accent-btn"))
            Tooltip::attach(btn, "Cycles --accent tokens across every view in real time", 0.35f);
    }

    // -----------------------------------------------------------------------
    //  View 1: Dashboard — Prefabs + <Slot>, Bindable<T>, If, Keyed Each
    // -----------------------------------------------------------------------
    void buildDashboardView()
    {
        static const char *kDashMarkup = R"zm(
            ScrollView.view-scroll {
                HStack.kpi-grid {
                    KpiCard {
                        Text.kpi-label "ACTIVE WORKLOAD (Bindable<float>)"
                        Text.kpi-value@kpi-load "42%"
                        ProgressBar@kpi-bar value=0.42 width="100%"
                    }
                    KpiCard {
                        Text.kpi-label "OPEN TASKS (Keyed Each)"
                        Text.kpi-value@kpi-tasks "3"
                        Text.muted "Reconciled in place by ID"
                    }
                    KpiCard {
                        Text.kpi-label "ACTIVE THEME (CSS Variables)"
                        Text.kpi-value@kpi-theme "Dark"
                        Text.muted "Switch live from the sidebar"
                    }
                }

                VStack.card {
                    Text.card-title wrap="true" "Reactive Data Binding & Conditional UI"
                    Text.muted wrap "Drag the slider below: a single Bindable<float> updates the KPI card, progress bar, percentage readout, and triggers an If@overload-warn block above 80%."

                    HStack.row-wrap {
                        Text.form-label "Cluster load"
                        Slider@load-slider value=0.42 grow="1" min-width="0"
                        Text.value-readout@load-pct "42%"
                    }

                    If@overload-warn {
                        HStack.alert-banner {
                            Text.alert-text wrap="true" grow="1" min-width="0" "High workload (> 80%) — structural If block automatically mounted!"
                        }
                    }
                }

                VStack.card {
                    Text.card-title wrap="true" "Keyed Task Queue (Each + Prefab + Empty Fallback)"
                    Text.muted wrap "Add or complete items. Existing rows are preserved via bindEachKeyed(); clearing all items mounts the inline fallback block."

                    HStack.row {
                        TextInput@task-input min-width="180px" placeholder="Add a new task..." grow="1" min-width="0"
                        Button.btn-primary@task-add "Add Task"
                        Button@task-clear "Clear All"
                    }

                    VStack gap="6px" width="100%" {
                        Each@task-list prefab="TaskRow" {
                            VStack.empty-state {
                                Text.muted "All tasks completed! Add a new task above to repopulate."
                            }
                        }
                    }
                }
            }
        )zm";

        dashUi_ = ZMarkup::build(kDashMarkup);
        auto view = dashUi_->root();
        mountView(view);

        // Wire slider -> workload_ Bindable<float>
        dashUi_->onValueChangedRef("load-slider", [this](float v)
                                   {
            workload_.set(v);
            highWorkload_.set(v > 0.80f); });

        // Bind conditional warning banner
        dashUi_->bindCondition("overload-warn", highWorkload_, true);

        // Bind KPI bar and labels directly to workload_
        if (auto bar = dashUi_->ref<ProgressBar>("kpi-bar"))
            floatSubs_.push_back(workload_.bindTo(bar, &ProgressBar::setValue, true));

        ZMarkup::WeakUINode weakDash = dashUi_->weak();
        floatSubs_.push_back(workload_.onChanged(
            [weakDash](const float &v)
            {
                if (auto ui = weakDash.lock())
                {
                    std::string pct = std::to_string((int)std::lround(v * 100.0f)) + "%";
                    ui->setTextRef("kpi-load", pct);
                    ui->setTextRef("load-pct", pct);
                }
            },
            true));

        // Update KPI task counter whenever tasks_ changes
        taskSubs_.push_back(tasks_.onChanged(
            [weakDash](const std::vector<TaskItem> &vec)
            {
                if (auto ui = weakDash.lock())
                    ui->setTextRef("kpi-tasks", std::to_string(vec.size()));
            },
            true));

        // Keyed reconciliation for the task list
        dashUi_->bindEachKeyed(
            "task-list",
            tasks_,
            [](const TaskItem &t)
            { return t.id; },
            [this](ZMarkup::UINode &itemUi, const TaskItem &t)
            {
                int taskId = t.id;
                itemUi.onClickRef("done", [this, taskId]
                                  {
                    auto next = tasks_.get();
                    next.erase(std::remove_if(next.begin(), next.end(),
                                              [taskId](const TaskItem &it) { return it.id == taskId; }),
                               next.end());
                    tasks_.set(std::move(next)); });
            },
            [](ZMarkup::UINode &itemUi, const TaskItem &t)
            {
                itemUi.setTextRef("tag", t.tag);
                itemUi.setTextRef("title", t.title);
            },
            true);

        // Add / Clear actions
        auto addTask = [this]
        {
            auto input = dashUi_->ref<TextInput>("task-input");
            if (!input)
                return;
            std::string txt = input->getText();
            if (txt.empty())
                return;
            auto next = tasks_.get();
            next.push_back({nextTaskId_++, "Custom", txt});
            tasks_.set(std::move(next));
            input->setText("");
        };

        dashUi_->onClickRef("task-add", addTask);
        if (auto input = dashUi_->ref<TextInput>("task-input"))
        {
            input->onSubmit = [addTask](const std::string &)
            { addTask(); };
        }
        dashUi_->onClickRef("task-clear", [this]
                            { tasks_.set({}); });
    }

    // -----------------------------------------------------------------------
    //  View 2: Editor & Forms — Widgets, UTF-8 TextArea, Dirty State, Modal
    // -----------------------------------------------------------------------
    void buildEditorView()
    {
        static const char *kEditorMarkup = R"zm(
            ScrollView.view-scroll {
                HStack.split-row {
                    VStack.card.split-col {
                        Text.card-title wrap="true" "Workspace Settings"
                        Text.muted wrap "Modifying any control flags the top-bar unsaved badge via Bindable<bool>."

                        HStack.row {
                            Text.form-label "Display name"
                            TextInput@ed-name value="zenit_dev" grow="1" min-width="0"
                        }
                        HStack.row {
                            Text.form-label "Master volume"
                            Slider@ed-vol value=0.72 grow="1" min-width="0"
                            Text.value-readout@ed-vol-lbl "72%"
                        }
                        HStack.row {
                            Text.form-label "Render quality"
                            Dropdown@ed-qual options="Low,Medium,High,Ultra" grow="1" min-width="0"
                        }
                        HStack.row {
                            Text.form-label "Notifications"
                            Toggle@ed-notif checked=true
                        }
                        HStack.row {
                            Text.form-label "Cloud sync"
                            Checkbox@ed-cloud checked=false
                        }
                    }

                    VStack.card.split-col {
                        HStack.row-wrap justify="space-between" {
                            Text.card-title wrap "Multi-line UTF-8 TextArea"
                            Text.muted@notes-meta "0 lines"
                        }
                        Text.muted wrap "Supports soft word-wrap, UTF-8 grapheme cursor navigation, Ctrl+Z/Y undo/redo, and clipboard chords."
                        Stack@textarea-slot width="100%" {}
                    }
                }

                HStack.row {
                    Layout.spacer
                    Button@ed-reset "Reset Defaults"
                    Button.btn-primary@ed-save "Save Changes"
                }
            }
        )zm";

        editorUi_ = ZMarkup::build(kEditorMarkup);
        auto view = editorUi_->root();
        mountView(view);

        // Mount a multi-line UTF-8 TextArea inside the reserved slot.
        static const char *kDefaultNotes =
            "ZenitUI supports full UTF-8 editing:\n"
            "• Accented Latin: café, naïve, résumé, città\n"
            "• Symbols & arrows: → ← ↑ ↓ — € £ ©\n"
            "• Shortcuts: Ctrl+A, Ctrl+C/X/V, Ctrl+Z/Y\n\n"
            "Try selecting across wrapped lines or dragging with the mouse.";

        notesArea_ = TextArea::create(kDefaultNotes, 8);
        notesArea_->size(Percent(100), Px(210));
        notesArea_->setPlaceholder("Write release notes here...");

        auto updateNotesMeta = [this](const std::string &txt)
        {
            int lines = 1 + (int)std::count(txt.begin(), txt.end(), '\n');
            editorUi_->setTextRef(
                "notes-meta",
                std::to_string(lines) + " lines · " + std::to_string(txt.size()) + " bytes");
        };
        updateNotesMeta(notesArea_->getText());
        notesArea_->onTextChanged = [this, updateNotesMeta](const std::string &s)
        {
            updateNotesMeta(s);
            dirty_.set(true);
        };

        if (auto slot = editorUi_->ref("textarea-slot"))
            slot->addChild(notesArea_);

        // Wire form controls -> Bindable state + dirty flag
        if (auto qual = editorUi_->ref<Dropdown>("ed-qual"))
        {
            qual->setSelected(qualityIdx_.get());
            qual->onChange = [this](int idx, const std::string &)
            {
                qualityIdx_.set(idx);
                dirty_.set(true);
            };
        }

        editorUi_->onTextChangedRef("ed-name", [this](const std::string &s)
                                    {
            username_.set(s);
            dirty_.set(true); });

        editorUi_->onValueChangedRef("ed-vol", [this](float v)
                                     {
            volume_.set(v);
            dirty_.set(true); });

        ZMarkup::WeakUINode weakEd = editorUi_->weak();
        floatSubs_.push_back(volume_.onChanged(
            [weakEd](const float &v)
            {
                if (auto ui = weakEd.lock())
                    ui->setTextRef("ed-vol-lbl", std::to_string((int)std::lround(v * 100.0f)) + "%");
            },
            true));

        editorUi_->onToggleRef("ed-notif", [this](bool v)
                               {
            notifications_.set(v);
            dirty_.set(true); });

        if (auto cb = editorUi_->ref<Checkbox>("ed-cloud"))
        {
            cb->onToggle = [this](bool v)
            {
                cloudSync_.set(v);
                dirty_.set(true);
            };
        }

        // Working Reset button: restores model & all widgets
        editorUi_->onClickRef("ed-reset", [this, updateNotesMeta]
                              {
            username_.set("zenit_dev");
            volume_.set(0.72f);
            qualityIdx_.set(2);
            notifications_.set(true);
            cloudSync_.set(false);

            if (auto in = editorUi_->ref<TextInput>("ed-name"))
                in->setText("zenit_dev");
            if (auto sl = editorUi_->ref<Slider>("ed-vol"))
                sl->setValue(0.72f);
            if (auto dd = editorUi_->ref<Dropdown>("ed-qual"))
                dd->setSelected(2);
            if (auto tg = editorUi_->ref<Toggle>("ed-notif"))
                tg->setChecked(true);
            if (auto cb = editorUi_->ref<Checkbox>("ed-cloud"))
                cb->setChecked(false);
            if (notesArea_)
            {
                notesArea_->setText(kDefaultNotes);
                updateNotesMeta(notesArea_->getText());
            }
            dirty_.set(false); });

        // Working Save button: confirms via Modal and clears dirty flag
        editorUi_->onClickRef("ed-save", [this]
                              { openModal(
                                    "Save Workspace Settings?",
                                    "Profile \"" + username_.get() + "\" (volume " +
                                        std::to_string((int)std::lround(volume_.get() * 100.0f)) +
                                        "%) will be committed and the unsaved indicator cleared.",
                                    [this]
                                    { dirty_.set(false); }); });
    }

    // -----------------------------------------------------------------------
    //  View 3: Virtual Explorer — 10,000-row VirtualView + ContextMenu + 9-slice
    // -----------------------------------------------------------------------
    void buildExplorerView()
    {
        static const char *kExplorerMarkup = R"zm(
            VStack.view-fixed {
                HStack.row-wrap {
                    TextInput@exp-search placeholder="Filter 10,000 entries by category, text, or ID..." grow="1" min-width="150px"
                    Button@jump-top "Top"
                    Button@jump-mid "#5,000"
                    Button@jump-end "Bottom"
                }

                HStack.row-wrap justify="space-between" {
                    Text.muted@exp-range wrap "Visible: --"
                    Stack.nineslice-box@nineslice-badge {
                        Text "Nine-Slice Badge (npatches_y.png)"
                    }
                }

                Stack.virtual-host@virtual-host {}

                Text.muted@exp-status "Tip: Right-click any row to open a portal-backed ContextMenu."
            }
        )zm";

        explorerUi_ = ZMarkup::build(kExplorerMarkup);
        auto view = explorerUi_->root();
        mountView(view);

        // Apply the custom nine-slice texture asset to the preview badge.
        if (auto badge = explorerUi_->ref("nineslice-badge"))
        {
            Tooltip::attach(badge, "Rendered via ZStyle: background-texture: npatches 8 8 8 8", 0.35f);
        }

        // Single shared ContextMenu mounted on the Explorer view (escaped via portal).
        rowContextMenu_ = ContextMenu({
            {"Inspect entry", [this]
             {
                 if (contextRowIndex_ >= 0 && contextRowIndex_ < (int)filteredIndices_.size())
                 {
                     const auto &e = catalog_[filteredIndices_[contextRowIndex_]];
                     openModal(
                         "Entry #" + std::to_string(e.id) + " [" + e.category + "]",
                         e.message,
                         nullptr);
                 }
             }},
            {"Copy summary to clipboard", [this]
             {
                 if (contextRowIndex_ >= 0 && contextRowIndex_ < (int)filteredIndices_.size())
                 {
                     const auto &e = catalog_[filteredIndices_[contextRowIndex_]];
                     std::string text = "#" + std::to_string(e.id) + " [" + e.category + "] " + e.message;
                     if (UIContext::get().platform)
                         UIContext::get().platform->setClipboardText(text);
                     explorerUi_->setTextRef("exp-status", "Copied to clipboard: " + text);
                 }
             }},
            {"Scroll entry to top", [this]
             {
                 if (virtualList_ && contextRowIndex_ >= 0)
                     virtualList_->scrollToItem(contextRowIndex_);
             }},
        });
        view->addChild(rowContextMenu_);

        // Create the 10,000-row VirtualView with O(1) uniform row height.
        virtualList_ = VirtualView::create(
            [this](int idx) -> std::shared_ptr<Layout>
            {
                if (idx < 0 || idx >= (int)filteredIndices_.size())
                    return nullptr;
                const auto &e = catalog_[filteredIndices_[idx]];

                auto row = HStack()->cls("vrow");
                if (idx % 2 == 1)
                    row->addClass("alt");
                row->setInteractive(true);
                row->setBlocksRaycast(true);

                row->addChild(Label("#" + std::to_string(e.id))->cls("vrow-id"));
                row->addChild(Label(e.category)->cls("vrow-cat"));
                row->addChild(Label(e.message)->cls("vrow-msg"));

                row->onClick = [this, id = e.id, cat = e.category]
                {
                    explorerUi_->setTextRef(
                        "exp-status",
                        "Selected entry #" + std::to_string(id) + " [" + cat + "]");
                };
                row->onRightClick = [this, idx]
                {
                    contextRowIndex_ = idx;
                    if (rowContextMenu_)
                        rowContextMenu_->openAt(UIContext::get().pointer.pos);
                };
                return row;
            },
            (int)filteredIndices_.size());

        virtualList_->size(Percent(100), Percent(100));
        virtualList_->setUniformHeight(42.0f);

        if (auto host = explorerUi_->ref("virtual-host"))
            host->addChild(virtualList_);

        // Live case-insensitive filtering
        explorerUi_->onTextChangedRef("exp-search", [this](const std::string &q)
                                      { applyExplorerFilter(q); });

        explorerUi_->onClickRef("jump-top", [this]
                                {
            if (virtualList_)
                virtualList_->scrollToItem(0); });
        explorerUi_->onClickRef("jump-mid", [this]
                                {
            if (virtualList_)
                virtualList_->scrollToItem(std::min(4999, std::max(0, (int)filteredIndices_.size() - 1))); });
        explorerUi_->onClickRef("jump-end", [this]
                                {
            if (virtualList_ && !filteredIndices_.empty())
                virtualList_->scrollToItem((int)filteredIndices_.size() - 1); });
    }

    void applyExplorerFilter(const std::string &query)
    {
        filterQuery_ = query;
        std::string lowerQ = query;
        std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(),
                       [](unsigned char c)
                       { return (char)std::tolower(c); });

        filteredIndices_.clear();
        for (int i = 0; i < (int)catalog_.size(); ++i)
        {
            if (lowerQ.empty())
            {
                filteredIndices_.push_back(i);
                continue;
            }
            const auto &e = catalog_[i];
            std::string hay = std::to_string(e.id) + " " + e.category + " " + e.message;
            std::transform(hay.begin(), hay.end(), hay.begin(),
                           [](unsigned char c)
                           { return (char)std::tolower(c); });
            if (hay.find(lowerQ) != std::string::npos)
                filteredIndices_.push_back(i);
        }

        if (virtualList_)
        {
            virtualList_->removeAllChildren();
            virtualList_->scrollToTop();
            // Force height/materialization refresh even if count matched previous count.
            if (virtualList_->getItemCount() == (int)filteredIndices_.size())
                virtualList_->setItemCount(0);
            virtualList_->setItemCount((int)filteredIndices_.size());
        }
        updateExplorerRange();
    }

    void updateExplorerRange()
    {
        if (!explorerUi_ || !virtualList_)
            return;
        int total = (int)filteredIndices_.size();
        int first = virtualList_->getFirstVisibleItem();
        int last = virtualList_->getLastVisibleItem();
        if (total == 0 || first < 0)
        {
            explorerUi_->setTextRef("exp-range", "0 matching entries");
            return;
        }
        explorerUi_->setTextRef(
            "exp-range",
            "Materialized viewport: #" + std::to_string(first + 1) + " – #" +
                std::to_string(last + 1) + " of " + std::to_string(total) + " entries");
    }

    // -----------------------------------------------------------------------
    //  View 4: Effects & Motion — CSS Transitions, @keyframes, UIAnimation,
    //                             FilterRegistry, and CanvasLayout
    // -----------------------------------------------------------------------
    void buildEffectsView()
    {
        static const char *kEffectsMarkup = R"zm(
            ScrollView.view-scroll {
                VStack.card {
                    Text.card-title wrap="true" "Declarative Motion (ZStyle Transitions & @keyframes)"
                    HStack.row-wrap {
                        VStack.fx-tile.fx-hover {
                            Text "Hover / Press"
                            Text.muted color="#ffffffcc" "scale + color"
                        }
                        VStack.fx-tile.fx-pulse {
                            Text "Pulse @keyframes"
                            Text.muted "infinite loop"
                        }
                        VStack.fx-tile.card-elevated {
                            Text "Box Shadow"
                            Text.muted "soft elevation"
                        }
                    }
                }

                VStack.card {
                    Text.card-title wrap="true" "Imperative UIAnimation (Event-Driven Timelines)"
                    HStack.row-wrap {
                        VStack.fx-tile@anim-target min-width="180px" {
                            Text "Animated Card"
                            Text.muted "UIAnimation tracks"
                        }
                        Button.btn-primary@anim-slide "Slide In"
                        Button@anim-reverse "Reverse"
                        Button@anim-shake "Damped Shake"
                    }
                }

                VStack.card {
                    Text.card-title wrap="true" "Post-Processing Filters (FilterRegistry) & CanvasLayout"
                    Text.muted wrap "Off-screen targets are pooled automatically in UIContext::targetPool."
                    HStack.row-wrap {
                        VStack.fx-tile.fx-blur   { Text "blur(2px)" }
                        VStack.fx-tile.fx-sepia  { Text "sepia(0.85)" }
                        VStack.fx-tile.fx-shadow { Text "drop-shadow" }
                        VStack.fx-tile.fx-chain  { Text "blur + sepia" }
                    }

                    HStack.row-wrap {
                        Stack@canvas-slot {}
                        VStack grow="1" min-width="240px" items-h="start" gap="8px" {
                            Text.card-title wrap="true" "CanvasLayout Offscreen Target"
                            Text.muted wrap "Toggle a whole-canvas fragment shader (hue_shift.fs) on the composited subtree."
                            HStack.row {
                                Text.muted "Enable hueShift shader"
                                Toggle@canvas-shader-toggle checked=true
                            }
                        }
                    }
                }
            }
        )zm";

        effectsUi_ = ZMarkup::build(kEffectsMarkup);
        auto view = effectsUi_->root();
        mountView(view);

        // Register imperative UIAnimations on @anim-target
        if (auto target = effectsUi_->ref("anim-target"))
        {
            auto slide = std::make_shared<UIAnimation>(0.45f);
            slide->addTrack<Value>(
                Px(0.0f), Px(36.0f),
                [](Layout *l, Value v)
                { l->getInlineBase().translateX = v; },
                TransitionFunction::EaseOutBack);
            slide->addTrack<float>(
                1.0f, 1.06f,
                [](Layout *l, float s)
                { l->getInlineBase().scale = s; },
                TransitionFunction::EaseOutQuad);

            auto shake = std::make_shared<UIAnimation>(0.45f);
            shake->addTrack<float>(
                0.0f, 1.0f,
                [](Layout *l, float t)
                {
                    float amp = (1.0f - t) * 14.0f;
                    l->getInlineBase().translateX = Px(std::sin(t * 32.0f) * amp);
                },
                TransitionFunction::Linear);

            target->addAnimation("slide", slide);
            target->addAnimation("shake", shake);

            std::weak_ptr<Layout> weakTarget = target;
            effectsUi_->onClickRef("anim-slide", [weakTarget]
                                   {
                if (auto t = weakTarget.lock())
                    t->playAnimation("slide", false); });
            effectsUi_->onClickRef("anim-reverse", [weakTarget]
                                   {
                if (auto t = weakTarget.lock())
                    t->playAnimation("slide", true); });
            effectsUi_->onClickRef("anim-shake", [weakTarget]
                                   {
                if (auto t = weakTarget.lock())
                    t->playAnimation("shake", false); });
        }

        // CanvasLayout with toggleable hueShift shader
        auto canvas = CanvasLayout::create();
        canvas->size(Px(210), Px(110));

        auto scene = VStack();
        scene->getInlineBase().gap = Px(8);
        scene->getInlineBase().alignItems = Align::Center;

        auto swatches = HStack();
        swatches->getInlineBase().gap = Px(8);
        auto makeSwatch = [](ZenitUI::Color c)
        {
            auto s = std::make_shared<Layout>(LayoutType::Stack);
            s->cls("swatch");
            s->getInlineBase().background = c;
            return s;
        };
        swatches->addChild(makeSwatch({225, 70, 75, 255}));
        swatches->addChild(makeSwatch({65, 200, 110, 255}));
        swatches->addChild(makeSwatch({75, 135, 240, 255}));
        scene->addChild(swatches);
        scene->addChild(Label("Composited RGB Scene")->cls("muted"));
        canvas->addChild(scene);

        auto applyCanvasShader = [canvas](bool enabled)
        {
            if (!UIContext::get().assets)
                return;
            canvas->setShader(enabled ? UIContext::get().assets->getEffect("hueShift")
                                      : EffectHandle{});
        };
        applyCanvasShader(true);

        effectsUi_->onToggleRef("canvas-shader-toggle", applyCanvasShader);

        if (auto slot = effectsUi_->ref("canvas-slot"))
            slot->addChild(canvas);
    }

    // -----------------------------------------------------------------------
    //  Shared Modal overlay mounted directly on #app
    // -----------------------------------------------------------------------
    void buildModal()
    {
        modal_ = Modal::create();
        auto card = modal_->children[0];
        card->cls("modal-card");

        modalTitle_ = Label("")->cls("page-title");
        modalBody_ = Label("")->cls("muted");
        modalBody_->setWrap(true);

        auto actions = HStack();
        actions->getInlineBase().width = Percent(100);
        actions->getInlineBase().justify = Justify::End;
        actions->getInlineBase().gap = Px(10);

        actions->addChild(Btn("Close", [this]
                              { modal_->hide(); }));
        actions->addChild(Btn("Confirm", [this]
                              {
                              if (modalConfirmAction_)
                                  modalConfirmAction_();
                              modal_->hide(); })
                              ->cls("btn-primary"));

        card->addChild(modalTitle_);
        card->addChild(modalBody_);
        card->addChild(actions);

        appRoot_->addChild(modal_);
    }

    void openModal(const std::string &title,
                   const std::string &body,
                   std::function<void()> onConfirm)
    {
        if (!modal_)
            return;
        modalTitle_->setText(title);
        modalBody_->setText(body);
        modalConfirmAction_ = std::move(onConfirm);
        modal_->show();
    }

    // -----------------------------------------------------------------------
    //  View mounting & tab switching (preserves live state per view)
    // -----------------------------------------------------------------------
    void mountView(std::shared_ptr<Layout> view)
    {
        if (auto host = shellUi_->ref("content-host"))
            host->addChild(view);
        views_.push_back(std::move(view));
    }

    void switchView(int index)
    {
        static const char *kTitles[] = {
            "Dashboard — Prefabs, Bindable & Keyed Each",
            "Editor & Forms — Widgets & UTF-8 TextArea",
            "Virtual Explorer — 10,000 Rows & Portals",
            "Effects & Motion — Shaders & Animations",
        };

        for (int i = 0; i < (int)views_.size(); ++i)
        {
            views_[i]->setVisible(i == index);
            shellUi_->toggleClassRef("nav-" + std::to_string(i), "active", i == index);
        }
        shellUi_->setTextRef("page-title", kTitles[index]);
        if (index == 2)
            updateExplorerRange();
    }
};