#pragma once

#include "Common.hpp"
#include "Layout.hpp"
#include "ScrollView.hpp"
#include "Logger.hpp"

#include <functional>
#include <algorithm>
#include <cmath>

namespace ZenitUI::UI
{

    class VirtualView : public TLayout<VirtualView, ScrollView>
    {
    public:
        using ItemFactory = std::function<std::shared_ptr<Layout>(int index)>;

        VirtualView(ItemFactory factory, int itemCount)
            : TLayout<VirtualView, ScrollView>(LayoutType::Vertical),
              itemFactory(std::move(factory)),
              itemCount(std::max(0, itemCount))
        {
        }

        static std::shared_ptr<VirtualView> create(ItemFactory factory, int count)
        {
            return TLayout<VirtualView, ScrollView>::create(
                std::move(factory), count);
        }

        static std::shared_ptr<VirtualView> create(
            std::function<std::shared_ptr<Layout>()> factory,
            int count)
        {
            return create([factory](int)
                          { return factory(); }, count);
        }

        int getItemCount() const noexcept { return itemCount; }
        void setItemCount(int count);

        float getUniformHeight() const noexcept { return uniformHeight; }
        void setUniformHeight(float px);
        int getFirstVisibleItem() const;
        int getLastVisibleItem() const;
        void scrollToItem(int index);

        void removeAllChildren() override;

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

        struct Entry
        {
            std::shared_ptr<Layout> node;
            int index{-1};
        };
        std::vector<Entry> materialized;

        void rebuildCumulative(float availW);
        int itemAtY(float y) const;
        void updateMaterialization();
        bool hasMaterialized(int index) const;
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

    inline void VirtualView::arrange(Rect space)
    {
        rect = space;

        updateMaterialization();

        internal_.subtreeDirty = 1;
        TLayout<VirtualView, ScrollView>::arrange(space);
    }

} // namespace ZenitUI::UI