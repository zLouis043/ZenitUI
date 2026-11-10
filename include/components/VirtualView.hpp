#pragma once

/**
 * \file VirtualView.hpp
 * \brief A virtualized scrolling list with lazy materialization.
 *
 * `VirtualView` renders only the items currently in (or near) the viewport,
 * so it can present lists with tens of thousands of items at 60 fps. It is a
 * `ScrollView` subclass (`Vertical` flow), so it inherits scrolling, clipping
 * and scrollbar rendering from the base.
 *
 * Three data structures drive the virtualization:
 *
 *  - **Cumulative heights** (`cumulativeHeights`): prefix-sum of item
 *    heights, so that "which item is at y?" is an `upper_bound` lookup.
 *    Length is `itemCount + 1`; the last entry is the total content height.
 *    Invalidated by `setItemCount`, `setUniformHeight`, and by width changes.
 *  - **Materialized** (`materialized`): the actual child `Layout` nodes, one
 *    per visible (plus a small overscan) item. Each entry stores both the
 *    node and its index, so insertion / removal is O(visible), not O(n).
 *  - **Uniform height** (`uniformHeight`): optional optimization. When set
 *    to a positive value, `rebuildCumulative` skips measuring — the full
 *    prefix sum is `i * uniformHeight`. This makes 100k-item lists O(1) per
 *    frame for height computation, at the cost of not supporting
 *    variable-height items.
 *
 * The public API (`getFirstVisibleItem`, `getLastVisibleItem`,
 * `scrollToItem`, `setItemCount`, `setUniformHeight`) uses the cumulative
 * heights, which must have been built at least once. Before the first
 * `measure`, they are empty and those methods return `-1` / no-op.
 */

#include "Common.hpp"
#include "Layout.hpp"
#include "ScrollView.hpp"
#include "Logger.hpp"

#include <functional>
#include <algorithm>
#include <cmath>

namespace ZenitUI::UI
{

    /**
     * \class VirtualView
     * \brief A virtualized list.
     *
     * Style tag: `"VirtualView"`. Extends `ScrollView` (`Vertical`). Item
     * nodes are added as absolutely-positioned children of the view; the view
     * itself does not lay them out — their `top` and `height` come from
     * `cumulativeHeights` at materialization time.
     */
    class VirtualView : public TLayout<VirtualView, ScrollView>
    {
    public:
        /** \brief Signature for an item factory: index → node. */
        using ItemFactory = std::function<std::shared_ptr<Layout>(int index)>;

        /**
         * \brief Construct with a factory and an item count.
         * \param factory Called on demand for each item that enters the
         *                overscan window. Should be cheap; if it is expensive,
         *                cache externally.
         * \param itemCount Total number of items. Negative is clamped to `0`.
         */
        VirtualView(ItemFactory factory, int itemCount)
            : TLayout<VirtualView, ScrollView>(LayoutType::Vertical),
              itemFactory(std::move(factory)),
              itemCount(std::max(0, itemCount))
        {
        }

        /**
         * \brief Construct with an indexed factory.
         * \note This overload exists to satisfy `TLayout::create`'s
         *       forwarding: it is not intended to be called directly. Use
         *       `create(factory, count)` instead.
         */
        static std::shared_ptr<VirtualView> create(ItemFactory factory, int count)
        {
            return TLayout<VirtualView, ScrollView>::create(
                std::move(factory), count);
        }

        /**
         * \brief Construct with an index-agnostic factory.
         *
         * All items share the same factory (no index dispatch). Useful for
         * placeholder skeletons or uniform rows.
         */
        static std::shared_ptr<VirtualView> create(
            std::function<std::shared_ptr<Layout>()> factory,
            int count)
        {
            return create([factory](int)
                          { return factory(); }, count);
        }

        /** \brief Number of items in the list. */
        int getItemCount() const noexcept { return itemCount; }
        /**
         * \brief Replace the item count.
         *
         * Invalidates the cumulative heights; they are rebuilt on the next
         * `measure`. If the count is unchanged, no-op.
         */
        void setItemCount(int count);

        /**
         * \brief Return the uniform height, or `0` if variable heights are
         *        being measured.
         */
        float getUniformHeight() const noexcept { return uniformHeight; }
        /**
         * \brief Set a uniform height for all items.
         *
         * When positive, disables measurement — this makes huge lists cheap
         * (O(1) height computation), at the cost of assuming all items have
         * the same height. `0` restores measurement mode.
         */
        void setUniformHeight(float px);
        /**
         * \brief First item whose range overlaps `[scrollY, scrollY+height)`.
         * \return `-1` if there are no items or the heights are not built.
         */
        int getFirstVisibleItem() const;
        /** \brief Last visible item, inclusive. `-1` if none. */
        int getLastVisibleItem() const;
        /**
         * \brief Scroll so that the given item is the first visible.
         *
         * Does not account for the item's height: item \p index is placed at
         * the top of the viewport. `-1` and out-of-range indices are clamped
         * to the nearest valid value.
         */
        void scrollToItem(int index);

        /** \brief Clear the materialized children *and* the materialization
         *         bookkeeping. Called automatically by `Layout::removeAllChildren`. */
        void removeAllChildren() override;

        /**
         * \brief Fired each time an item node is materialized.
         *
         * Useful for wiring per-item callbacks that need to see the node
         * identity. Not fired on re-materialization (when scrolling back to
         * an item that was previously unmaterialized, the callback fires
         * again).
         */
        std::function<void(Layout *, int)> onItemCreated;

    protected:
        void onBuild() override;
        void arrange(Rect space) override;
        Vec2 computeIntrinsicSize(float availW, float availH) override;

    private:
        ItemFactory itemFactory;
        int itemCount{0};
        float uniformHeight{0.0f};

        std::vector<float> cumulativeHeights; // size = itemCount + 1
        float lastMeasuredWidth{-1.0f};

        /** \brief One materialized item: the node and its index. */
        struct Entry
        {
            std::shared_ptr<Layout> node;
            int index{-1};
        };
        std::vector<Entry> materialized;

        /** \brief Recompute `cumulativeHeights` for the given available width. */
        void rebuildCumulative(float availW);
        /** \brief Return the index of the item that contains \p y, or `-1`. */
        int itemAtY(float y) const;
        /** \brief Ensure the materialized set matches the current viewport. */
        void updateMaterialization();
        /** \brief `true` if \p index has a live materialized node. */
        bool hasMaterialized(int index) const;
        /** \brief Materialize the item at \p index and add it as a child. */
        void addMaterialized(int index);
    };

    // ---- implementation ----

    inline void VirtualView::onBuild()
    {
        setStyleTag("VirtualView");
        setInteractive(true);
        setBlocksRaycast(true);
        TLayout<VirtualView, ScrollView>::onBuild();
    }

    inline void VirtualView::removeAllChildren()
    {
        materialized.clear();
        TLayout<VirtualView, ScrollView>::removeAllChildren();
    }

    inline void VirtualView::setItemCount(int count)
    {
        count = std::max(0, count);
        if (count == itemCount)
            return;
        itemCount = count;
        cumulativeHeights.clear();
        lastMeasuredWidth = -1.0f;
        internal_.subtreeDirty = 1;
        internal_.pendingTransition = 1;
    }

    inline void VirtualView::setUniformHeight(float px)
    {
        px = std::max(0.0f, px);
        if (px == uniformHeight)
            return;
        uniformHeight = px;
        cumulativeHeights.clear();
        lastMeasuredWidth = -1.0f;
        internal_.subtreeDirty = 1;
        internal_.pendingTransition = 1;
    }

    /**
     * \brief Rebuild `cumulativeHeights` (prefix sum of item heights).
     *
     * Uniform height mode: O(itemCount), no item construction.
     * Variable height mode: O(itemCount) but calls `itemFactory(i)` for every
     * item and measures it, so a huge list with variable heights is expensive
     * to rebuild. Rebuilds happen on width change or count change, so the
     * cost is amortized across frames.
     */
    inline void VirtualView::rebuildCumulative(float availW)
    {
        cumulativeHeights.assign((size_t)itemCount + 1, 0.0f);

        if (uniformHeight > 0.0f)
        {
            for (int i = 0; i <= itemCount; ++i)
                cumulativeHeights[i] = (float)i * uniformHeight;
            lastMeasuredWidth = availW;
            return;
        }

        float acc = 0.0f;
        for (int i = 0; i < itemCount; ++i)
        {
            std::shared_ptr<Layout> node;
            if (itemFactory)
                node = itemFactory(i);
            if (node)
            {
                Vec2 sz = node->measure(availW, 1e9f);
                acc += std::max(1.0f, sz.y);
            }
            else
            {
                acc += 1.0f;
            }
            cumulativeHeights[i + 1] = acc;
        }
        lastMeasuredWidth = availW;
    }

    /**
     * \brief Binary search for the item at the given local Y (relative to
     *        the top of the content, i.e. the scrollable range).
     *
     * Uses `upper_bound` so that Y exactly on a boundary resolves to the item
     * below (the one that starts there). Returns `-1` if the heights are not
     * built or the list is empty.
     */
    inline int VirtualView::itemAtY(float y) const
    {
        if (itemCount <= 0)
            return -1;
        if (cumulativeHeights.size() != (size_t)itemCount + 1)
            return -1;
        auto it = std::upper_bound(cumulativeHeights.begin(),
                                   cumulativeHeights.end(), y);
        int idx = (int)(it - cumulativeHeights.begin()) - 1;
        return std::clamp(idx, 0, itemCount - 1);
    }

    inline int VirtualView::getFirstVisibleItem() const
    {
        return itemAtY(getScrollY());
    }

    inline int VirtualView::getLastVisibleItem() const
    {
        if (itemCount <= 0)
            return -1;
        float bottom = getScrollY() + std::max(0.0f, getRect().height - 0.001f);
        return itemAtY(bottom);
    }

    inline void VirtualView::scrollToItem(int index)
    {
        if (itemCount <= 0)
            return;
        if (index < 0)
            index = 0;
        if (index >= itemCount)
            index = itemCount - 1;
        if (cumulativeHeights.size() != (size_t)itemCount + 1)
            return;
        scrollToY(cumulativeHeights[index]);
    }

    inline bool VirtualView::hasMaterialized(int index) const
    {
        for (const auto &e : materialized)
            if (e.index == index)
                return true;
        return false;
    }

    /**
     * \brief Create the item at \p index and add it to the tree.
     *
     * The item is:
     *  - positioned `absolute`;
     *  - `left: 0`, `width: 100%` (so it takes the view's width);
     *  - `top: cumulativeHeights[index]`;
     *  - `height: cumulativeHeights[index+1] - cumulativeHeights[index]`.
     *
     * The item is `enabled` on add — this is what restores input after a
     * potential lazy-disable. It is not tracked in `cumulativeHeights` for
     * further re-measuring; the height was already baked in.
     */
    inline void VirtualView::addMaterialized(int index)
    {
        if (hasMaterialized(index))
            return;
        if (!itemFactory)
            return;
        if (cumulativeHeights.size() != (size_t)itemCount + 1)
            return;

        auto node = itemFactory(index);
        if (!node)
            return;

        float top = cumulativeHeights[index];
        float bot = cumulativeHeights[index + 1];

        node->getInlineBase().position = Position::Absolute;
        node->getInlineBase().left = Px(0.0f);
        node->getInlineBase().width = Percent(100.0f);
        node->getInlineBase().top = Px(top);
        node->getInlineBase().height = Px(std::max(1.0f, bot - top));
        node->setEnabled(true);

        addChild(node);
        materialized.push_back({node, index});

        if (onItemCreated)
            onItemCreated(node.get(), index);
    }

    /**
     * \brief Reconcile `materialized` with the current viewport.
     *
     * The overscan window is `[first - 2, last + 2]`, clamped to the item
     * range. Items outside the window are marked for removal (deferred to
     * `cullRemovedChildren` at the end of the frame). Items inside the
     * window that are not yet materialized are added.
     *
     * The `+2` overscan keeps a small buffer of off-screen items so that
     * fast scrolling (e.g. inertial fling) does not show blank frames at
     * the leading edge.
     */
    inline void VirtualView::updateMaterialization()
    {
        if (itemCount <= 0)
        {
            for (auto &e : materialized)
                e.node->removeFromParent();
            materialized.clear();
            return;
        }

        if (cumulativeHeights.size() != (size_t)itemCount + 1)
            return;

        int first = getFirstVisibleItem();
        int last = getLastVisibleItem();
        if (first < 0 || last < 0)
            return;

        int start = std::max(0, first - 2);
        int end = std::min(itemCount - 1, last + 2);

        for (auto it = materialized.begin(); it != materialized.end();)
        {
            if (it->index < start || it->index > end)
            {
                it->node->removeFromParent();
                it = materialized.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (int i = start; i <= end; ++i)
            addMaterialized(i);
    }

    inline Vec2 VirtualView::computeIntrinsicSize(float availW, float /*availH*/)
    {
        if (cumulativeHeights.size() != (size_t)itemCount + 1 ||
            (uniformHeight == 0.0f && lastMeasuredWidth != availW))
        {
            rebuildCumulative(availW);
        }
        float totalH = cumulativeHeights.empty() ? 0.0f : cumulativeHeights.back();
        return {availW, totalH};
    }

    /**
     * \brief Arrange: update the materialized set, then delegate to base.
     *
     * Note that `internal_.subtreeDirty = 1` is set unconditionally — this
     * forces a re-measure next frame even when nothing changed. It is a
     * known wart (measure is cheap when nothing changed, but still) and
     * could be made conditional on `updateMaterialization` actually adding
     * or removing items.
     */
    inline void VirtualView::arrange(Rect space)
    {
        rect = space;

        updateMaterialization();

        internal_.subtreeDirty = 1;
        TLayout<VirtualView, ScrollView>::arrange(space);
    }

} // namespace ZenitUI::UI