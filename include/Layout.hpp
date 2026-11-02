#pragma once

#include "Common.hpp"

#include "Style.hpp"
#include "Theme.hpp"
#include "UIContext.hpp"
#include "AnimPrimitives.hpp"
#include "Logger.hpp"
#include "ScrollState.hpp"
#include "StyleResolver.hpp"
#include "AnimationPlayer.hpp"
#include "InputController.hpp"
#include "ScrollController.hpp"
#include "UIEnums.hpp"
#include "Media.hpp"

namespace ZenitUI
{

	struct NodeState
	{
		uint32_t hovered : 1;
		uint32_t pressed : 1;
		uint32_t focused : 1;
		uint32_t checked : 1;
	};

	struct NodeConfig
	{
		uint32_t interactive : 1;
		uint32_t blocksRaycast : 1;
		uint32_t enabled : 1;
		uint32_t focusable : 1;
		uint32_t focusScope : 1;
		uint32_t keyboardActivates : 1;
		uint32_t updateWhenDisabled : 1;
		uint32_t portal : 1;
		uint32_t passThrough : 1;
	};

	struct NodeInternal
	{
		uint32_t subtreeDirty : 1;
		uint32_t styleInitialized : 1;
		uint32_t pendingTransition : 1;
		uint32_t hadLocalAnimationsLast : 1;
		uint32_t wantsRemoval : 1;
		uint32_t pressedInChain : 1;
		uint32_t hadRemovedChildren : 1;
	};

	class Layout : public std::enable_shared_from_this<Layout>
	{
	public:
		Layout(LayoutType type = LayoutType::Stack) : type(type)
		{
			config.interactive = 1;
			config.enabled = 1;
			internal_.subtreeDirty = 1;
			internal_.pendingTransition = 1;
		}

		virtual ~Layout()
		{
			auto &ctx = UIContext::get();
			if (ctx.pressTarget == this)
				ctx.pressTarget = nullptr;
			if (ctx.releaseTarget == this)
				ctx.releaseTarget = nullptr;
			if (ctx.topmostConsumer == this)
				ctx.topmostConsumer = nullptr;
			if (ctx.hoverTarget == this)
				ctx.hoverTarget = nullptr;

			if (auto *r = ctx.renderer)
			{
				if (layerTarget_.valid())
					r->destroyTarget(layerTarget_);
				if (layerScratch_.valid())
					r->destroyTarget(layerScratch_);
				if (layerSeparable_.valid())
					r->destroyTarget(layerSeparable_);
			}
		}

		template <typename T = Layout>
		std::shared_ptr<T> as()
		{
			return std::static_pointer_cast<T>(shared_from_this());
		}

		friend struct StyleResolver;
		friend struct AnimationPlayer;
		friend struct InputController;
		friend struct ScrollController;

		void addClass(const std::string &className)
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return;
			styleClasses.push_back(className);
			invalidateStyleCacheRecursive();
			internal_.pendingTransition = 1;
		}

		void removeClass(const std::string &className)
		{
			auto it = std::remove(styleClasses.begin(),
								  styleClasses.end(),
								  className);
			if (it == styleClasses.end())
				return;
			styleClasses.erase(it, styleClasses.end());
			invalidateStyleCacheRecursive();
			internal_.pendingTransition = 1;
		}

		bool hasClass(const std::string &className) const
		{
			for (const auto &c : styleClasses)
				if (c == className)
					return true;
			return false;
		}

		bool hasId(const std::string &targetId) const
		{
			return !targetId.empty() && nodeId == targetId;
		}

		void clearStyleClasses()
		{
			if (styleClasses.empty())
				return;
			styleClasses.clear();
			internal_.pendingTransition = 1;
			invalidateStyleCacheRecursive();
		}

		void setId(const std::string &node_id)
		{
			nodeId = node_id;
		}

		void setRef(const std::string &r)
		{
			ref = r;
		}

		void setStyleTag(const std::string &t) { styleTag = t; invalidateStyleCacheRecursive(); }
		const std::string &getStyleTag() const { return styleTag; }
		const std::vector<std::string> &getStyleClasses() const { return styleClasses; }

		void setSize(Value w, Value h)
		{
			style_.inlineBase.width = w;
			style_.inlineBase.height = h;
			internal_.pendingTransition = 1;
		}

		std::shared_ptr<Layout> cls(const std::string &className)
		{
			addClass(className);
			return shared_from_this();
		}
		std::shared_ptr<Layout> id(const std::string &node_id)
		{
			setId(node_id);
			return shared_from_this();
		}
		std::shared_ptr<Layout> size(Value w, Value h)
		{
			setSize(w, h);
			return shared_from_this();
		}
		std::shared_ptr<Layout> with(std::shared_ptr<Layout> child)
		{
			addChild(std::move(child));
			return shared_from_this();
		}

		void addChild(std::shared_ptr<Layout> child)
		{
			if (!child)
				return;

			if (weak_from_this().expired())
			{
				logWarn("Layout", "", 0, 0,
						"addChild called on a node not handled with shared_ptr."
						"Use X::create() and put this logic in onBuild().");
				return;
			}

			child->parent = weak_from_this();
			children.push_back(std::move(child));
			internal_.pendingTransition = 1;
		}

		virtual Layout *contentTarget() { return this; }

		void removeFromParent() { internal_.wantsRemoval = 1; }
		void removeAllChildren()
		{
			for (auto &c : children)
				if (c)
					c->parent.reset();
			children.clear();
			internal_.pendingTransition = 1;
		}
		std::shared_ptr<Layout> getParent() const { return parent.lock(); }
		bool hasParent() const { return !parent.expired(); }

		bool isAncestorOf(const Layout *other) const
		{
			if (!other)
				return false;
			auto p = other->getParent();
			while (p)
			{
				if (p.get() == this)
					return true;
				p = p->getParent();
			}
			return false;
		}

		void setInteractive(bool interactive) { config.interactive = interactive ? 1 : 0; }
		void setBlocksRaycast(bool blocks) { config.blocksRaycast = blocks ? 1 : 0; }

		void setEnabled(bool e)
		{
			if ((config.enabled != 0) == e)
				return;
			config.enabled = e ? 1 : 0;
			internal_.pendingTransition = 1;

			if (!e)
			{
				auto &ctx = UIContext::get();
				if (ctx.focusedNode.lock().get() == this)
					ctx.focusedNode.reset();
				if (ctx.pointerCapture.lock().get() == this)
					ctx.pointerCapture.reset();
			}

			onEnabledChanged(e);
		}
		bool getEnabled() const { return config.enabled != 0; }

		void setChecked(bool c)
		{
			if ((state.checked != 0) == c)
				return;
			state.checked = c ? 1 : 0;
			internal_.pendingTransition = 1;
		}
		bool getChecked() const { return state.checked != 0; }

		bool isHoveredState() const { return state.hovered != 0; }
		bool isPressedState() const { return state.pressed != 0; }
		bool isFocusedState() const { return state.focused != 0; }
		bool isDisabledState() const { return config.enabled == 0; }
		bool isCheckedState() const { return state.checked != 0; }

		void setUpdateWhenDisabled(bool v) { config.updateWhenDisabled = v ? 1 : 0; }
		bool getUpdateWhenDisabled() const { return config.updateWhenDisabled != 0; }

		void beginTransition()
		{
			internal_.pendingTransition = 1;
			style_.transitionTimer = 0.0f;
			style_.transitionStartStyle = style_.currentStyle;
		}

		void setFocusable(bool f) { config.focusable = f ? 1 : 0; }
		bool isFocusable() const { return config.focusable != 0 && config.enabled != 0; }
		bool getFocused() const { return state.focused != 0; }

		void setFocusScope(bool s) { config.focusScope = s ? 1 : 0; }
		bool isFocusScope() const { return config.focusScope != 0; }

		void notifyDescendantFocused(Layout *descendant) { onDescendantFocused(descendant); }

		bool isStackingContext() const
		{
			return style_.currentStyle.position != Position::Static && !style_.currentStyle.zIndex.isAuto;
		}
		int getZIndex() const
		{
			return style_.currentStyle.zIndex.isAuto ? 0 : style_.currentStyle.zIndex.value;
		}

		bool isScrollContainer() const
		{
			return style_.currentStyle.overflowX != Overflow::Visible || style_.currentStyle.overflowY != Overflow::Visible;
		}

		void scrollTo(float x, float y)
		{
			if (!isScrollContainer())
				return;
			scroll_.state.offset.x = std::max(0.0f, x);
			scroll_.state.offset.y = std::max(0.0f, y);
			scroll_.state.clamp();
		}
		void scrollToX(float x) { scrollTo(x, scroll_.state.offset.y); }
		void scrollToY(float y) { scrollTo(scroll_.state.offset.x, y); }
		void scrollToTop()
		{
			if (isScrollContainer())
				scrollToY(0.0f);
		}
		void scrollToBottom()
		{
			if (isScrollContainer())
				scrollToY(scroll_.state.maxScroll.y);
		}
		void scrollToLeft()
		{
			if (isScrollContainer())
				scrollToX(0.0f);
		}
		void scrollToRight()
		{
			if (isScrollContainer())
				scrollToX(scroll_.state.maxScroll.x);
		}

		bool scrollIntoView(float margin = 0.0f);

		bool scrollIntoView(const Rect &subRect, float margin = 0.0f);

		float getScrollX() const { return scroll_.state.offset.x; }
		float getScrollY() const { return scroll_.state.offset.y; }
		float getMaxScrollX() const { return scroll_.state.maxScroll.x; }
		float getMaxScrollY() const { return scroll_.state.maxScroll.y; }

		void setPortal(bool p)
		{
			config.portal = p ? 1 : 0;
			internal_.pendingTransition = 1;
		}
		bool isPortal() const { return config.portal != 0; }

		void setPassThrough(bool p) { config.passThrough = p ? 1 : 0; }
		bool getPassThrough() const { return config.passThrough != 0; }

		void capturePointer()
		{
			UIContext::get().pointerCapture = shared_from_this();
		}
		void releasePointer()
		{
			auto &ctx = UIContext::get();
			auto sp = ctx.pointerCapture.lock();
			if (sp.get() == this)
				ctx.pointerCapture.reset();
		}
		bool hasPointerCapture() const
		{
			return UIContext::get().pointerCapture.lock().get() == this;
		}

		void setKeyboardActivates(bool v) { config.keyboardActivates = v ? 1 : 0; }
		bool getKeyboardActivates() const { return config.keyboardActivates != 0; }

		inline FontHandle resolveFont(const ComputedStyle &style, FontHandle fallback = {})
		{
			if (style.font.empty() || !UIContext::get().assets)
				return fallback;
			FontHandle h = UIContext::get().assets->getFont(style.font);
			return h.id != 0 ? h : fallback;
		}

		struct ResolvedBgTexture
		{
			TextureHandle tex;
			NineSlice slice;
			bool valid() const { return tex.valid(); }
		};

		inline ResolvedBgTexture resolveBgTexture(
			const ComputedStyle &style,
			TextureHandle fallbackTex = {},
			NineSlice fallbackSlice = {0, 0, 0, 0})
		{
			if (!style.backgroundTexture.name.empty())
			{
				auto *a = UIContext::get().assets;
				if (a)
				{
					TextureHandle h = a->getTexture(style.backgroundTexture.name);
					if (h.valid())
					{
						const auto &tr = style.backgroundTexture;
						return {h, NineSlice{tr.left, tr.top, tr.right, tr.bottom}};
					}
				}
			}
			return {fallbackTex, fallbackSlice};
		}

		void setInlineBase(const Style &s)
		{
			style_.inlineBase.overlay(s);
			internal_.pendingTransition = 1;
		}
		Style &getInlineBase()
		{
			internal_.pendingTransition = 1;
			return style_.inlineBase;
		}
		Style &getInlineDefaults()
		{
			internal_.pendingTransition = 1;
			return style_.inlineDefaults;
		}

		void addAnimation(const std::string &name, std::shared_ptr<UIAnimation> anim)
		{
			anim_.imperative[name] = {anim, 0.0f, 0.0f, false, false};
		}
		void playAnimation(const std::string &name, bool playReverse = false)
		{
			anim_.play(*this, name, playReverse);
		}
		bool hasActiveAnimations() const
		{
			return anim_.hasActive(*this);
		}

		void addCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					return;
			anim_.css.push_back({name, 0.0f, false});
		}
		void stopCssAnimation(const std::string &name)
		{
			for (auto &a : anim_.css)
				if (a.name == name)
					a.finished = true;
		}

		virtual Vec2 measure(float parent_w, float parent_h);
		virtual void arrange(Rect space);
		virtual void update(float dt, bool ancestorBlocked = false, bool scrolling = false);
		virtual void draw(float parentOpacity = 1.0f);

		Layout *hitTest(Vec2 p,
						bool ancestorBlocked = false,
						const Rect *clipLocal = nullptr);
		void updateTree(float dt);

		void runFrame(float dt);

		void renderFrame();

		Rect getRect() const { return rect; }
		Vec2 getMeasuredSize() const { return measuredSize; }
		const ComputedStyle &getStyle() const { return style_.currentStyle; }
		std::vector<const ThemeRule *> getMatchingRules() const;

		std::function<void()> onHoverEnter, onHoverExit, onPress, onRelease, onClick;

		std::function<void()> onRightClick;

		std::function<void(MouseButton)> onButtonPress;
		std::function<void(MouseButton)> onButtonRelease;
		std::function<void(MouseButton)> onButtonClick;

		std::function<void()> onAnimationsFinished;
		std::vector<std::shared_ptr<Layout>> children;

		std::string nodeId;

		std::string ref;

		void setBackgroundTexture(TextureHandle tex, NineSlice np = {0, 0, 0, 0})
		{
			bgTexture = tex;
			bgPatchInfo = np;
		}
		void setShader(EffectHandle shader)
		{
			customEffect = shader;
			hasShader = true;
		}

	protected:
		LayoutType type;
		uint32_t lastViewportGen_{0};
		Rect rect{0, 0, 0, 0};
		Vec2 measuredSize{0, 0};
		float lastMeasureW_{-1.0f};
		float lastMeasureH_{-1.0f};
		ScrollController scroll_;

		NodeState state{};
		NodeConfig config{};
		NodeInternal internal_{};
		std::string styleTag;
		std::vector<std::string> styleClasses;
		StyleResolver style_;
		InputController input_;

		TextureHandle bgTexture;
		NineSlice bgPatchInfo;
		TargetHandle layerTarget_;
		TargetHandle layerScratch_;
		TargetHandle layerSeparable_;
		EffectHandle customEffect;
		std::string effectName_;
		EffectHandle currentEffect_;
		bool hasShader{false};

		AnimationPlayer anim_;

		std::vector<Layout *> drawNegZ_, drawNormal_, drawPosZ_;

		std::weak_ptr<Layout> parent;

		void markInheritanceDirty()
		{
			internal_.pendingTransition = 1;
			internal_.subtreeDirty = 1;
			for (auto &c : children)
				c->markInheritanceDirty();
		}

		void invalidateStyleCacheRecursive()
		{
			style_.invalidateCache();
			internal_.pendingTransition = 1;
			for (auto &c : children)
				c->invalidateStyleCacheRecursive();
		}

		Style partStyle(const std::string &partName)
		{
			return style_.partFor(*this, partName);
		}

		virtual Vec2 computeIntrinsicSize(float /*availW*/, float /*availH*/) { return {0, 0}; }

		virtual void onLayout() {}

		virtual void onUpdate(float /*dt*/) {}

		virtual void onEnabledChanged(bool /*nowEnabled*/) {}

		EffectHandle resolveActiveEffect()
		{
			resolveEffectIfNeeded(style_.currentStyle);
			if (currentEffect_.valid())
				return currentEffect_;
			if (hasShader)
				return customEffect;
			return {};
		}

		virtual void renderContent(float /*op*/, const ComputedStyle & /*style*/) {}

		virtual void renderChrome(float op, const ComputedStyle &style)
		{
			auto r = UIContext::get().renderer;
			if (!r)
				return;
			if (rect.width <= 0 || rect.height <= 0)
				return;

			float maxRadius = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolveH(maxRadius * 2.0f, maxRadius * 2.0f), 0.0f, maxRadius);

			if (style.boxShadow.enabled)
			{
				const auto &sh = style.boxShadow;
				float sx = sh.x.resolveSelfH(rect.width, rect.height);
				float sy = sh.y.resolveSelfV(rect.width, rect.height);
				float blur = sh.blur.resolveSelfH(rect.width, rect.height);

				Rect shadowRect = {
					rect.x + sx,
					rect.y + sy,
					rect.width,
					rect.height};

				Color baseColor = sh.color.withAlpha(op);

				if (blur <= 0.5f)
				{
					r->fillRoundedRect(shadowRect, rPx, baseColor);
				}
				else
				{
					constexpr int LAYERS = 5;
					for (int i = 0; i < LAYERS; ++i)
					{
						float expand = blur * (float)(i + 1) / (float)LAYERS;
						float alpha = (1.0f - (float)i / (float)LAYERS) * 0.35f;
						Rect r2 = {
							shadowRect.x - expand,
							shadowRect.y - expand,
							shadowRect.width + expand * 2.0f,
							shadowRect.height + expand * 2.0f};
						float rPx2 = std::clamp(rPx + expand, 0.0f, expand + maxRadius);
						r->fillRoundedRect(r2, rPx2, sh.color.withAlpha(op * alpha));
					}
				}
			}

			Color bg = style.background.withAlpha(op);
			if (bg.a > 0)
			{
				if (rPx > 0.0f && maxRadius > 0.0f)
					r->fillRoundedRect(rect, rPx, bg);
				else
					r->fillRect(rect, bg);
			}

			auto resolved = resolveBgTexture(style, bgTexture, bgPatchInfo);
			if (resolved.valid())
			{
				Color tint = style.tint.withAlpha(op);
				if (resolved.slice.left || resolved.slice.top ||
					resolved.slice.right || resolved.slice.bottom)
				{
					r->drawNineSlice(resolved.tex, resolved.slice, rect, tint);
				}
				else
				{
					Rect src{0, 0, (float)resolved.tex.width, (float)resolved.tex.height};
					r->drawTexture(resolved.tex, src, rect, tint);
				}
			}

			Color bc = style.borderColor.withAlpha(op);
			float bw = style.borderWidth.resolveH(maxRadius * 2.0f, maxRadius * 2.0f);
			if (bc.a > 0 && bw > 0.0f)
			{
				if (rPx > 0.0f && maxRadius > 0.0f)
					r->strokeRoundedRect(rect, rPx, bw, bc);
				else
					r->strokeRect(rect, bw, bc);
			}
		}

		virtual void onDescendantFocused(Layout * /*descendant*/) {}

		void arrangeInto(Rect space);

		Transform2D currentTransform(const ComputedStyle &style) const
		{
			Transform2D tr;
			tr.pivot = rect.center();
			tr.translate = {
				style.translateX.resolveSelfH(rect.width, rect.height),
				style.translateY.resolveSelfV(rect.width, rect.height)};
			tr.rotationDeg = style.rotation;
			tr.scale = style.scale;
			return tr;
		}

	private:
		void initStyleIfNeeded();
		void cullRemovedChildren();
		void recomputeDirty(bool wasPending, const ComputedStyle &styleBefore);
		void resolveEffectIfNeeded(const ComputedStyle &style);
		void drawWithStyle(const ComputedStyle &renderStyle, float parentOpacity);
		void drawInline(const ComputedStyle &renderStyle, float globalOp);
		void drawLayer(const ComputedStyle &renderStyle, float globalOp);
		void drawChildren(float globalOp);

		void arrangeAbsoluteAndPortals(const Rect &inner);
		void arrangeAbsolute(Layout *c, const ComputedStyle &cst, const Rect &in);
		void arrangeVerticalFlow(const Rect &inner, float gapY);
		void arrangeHorizontalFlow(const Rect &inner, float gapX);
		void arrangeWrapFlow(const Rect &inner, float gapX, float gapY);
		void arrangeStackFlow(const Rect &inner);

		void translateSubtree(float dx, float dy);

		static float clampSafe(float v, float lo, float hi);
	};

	template <typename Derived, typename Base = Layout>
	class TLayout : public Base
	{
	public:
		template <typename... Args>
		TLayout(Args &&...args) : Base(std::forward<Args>(args)...) {}

		template <typename... Args>
		static std::shared_ptr<Derived> create(Args &&...args)
		{
			auto p = std::make_shared<Derived>(std::forward<Args>(args)...);
			static_cast<TLayout<Derived, Base> *>(p.get())->onBuild();
			return p;
		}
		std::shared_ptr<Derived> cls(const std::string &name)
		{
			this->addClass(name);
			return self();
		}
		std::shared_ptr<Derived> id(const std::string &node_id)
		{
			this->setId(node_id);
			return self();
		}
		std::shared_ptr<Derived> withRef(const std::string &r)
		{
			this->setRef(r);
			return self();
		}
		std::shared_ptr<Derived> size(Value w, Value h)
		{
			this->setSize(w, h);
			return self();
		}
		std::shared_ptr<Derived> passThrough(bool p = true)
		{
			this->setPassThrough(p);
			return self();
		}
		std::shared_ptr<Derived> with(std::shared_ptr<Layout> c)
		{
			this->addChild(std::move(c));
			return self();
		}
		std::shared_ptr<Derived> self()
		{
			return std::static_pointer_cast<Derived>(this->shared_from_this());
		}

	protected:
		virtual void onBuild() {}
	};

	inline bool nodeMatchesSimple(const Layout *node, const SimpleSelector &ss)
	{
		if (!node)
			return false;

		if (ss.requireHover && !node->isHoveredState())
			return false;
		if (ss.requirePressed && !node->isPressedState())
			return false;
		if (ss.requireFocus && !node->isFocusedState())
			return false;
		if (ss.requireDisabled && !node->isDisabledState())
			return false;
		if (ss.requireChecked && !node->isCheckedState())
			return false;

		auto matchesComponent = [node](SimpleSelector::Kind kind,
									   const std::string &name) -> bool
		{
			if (kind == SimpleSelector::Kind::Tag)
				return node->getStyleTag() == name;
			if (kind == SimpleSelector::Kind::Id)
				return node->nodeId == name;
			for (const auto &c : node->getStyleClasses())
				if (c == name)
					return true;
			return false;
		};

		if (!matchesComponent(ss.kind, ss.name))
			return false;

		for (const auto &[kind, name] : ss.extras)
		{
			if (!matchesComponent(kind, name))
				return false;
		}

		return true;
	}

	inline bool ruleMatches(const ThemeRule &r, const Layout *node)
	{
		if (r.chain.empty() || !node)
			return false;
		if (r.media.has_value() && !evaluateMedia(*r.media, Metrics::viewport))
			return false;
		if (!nodeMatchesSimple(node, r.chain.back()))
			return false;

		int i = (int)r.chain.size() - 2;
		const Layout *cur = node->getParent().get();
		while (i >= 0 && cur)
		{
			if (nodeMatchesSimple(cur, r.chain[i]))
				--i;
			cur = cur->getParent().get();
		}
		return i < 0;
	}
}