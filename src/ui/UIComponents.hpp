#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

	inline std::shared_ptr<Layout> VStack(std::initializer_list<std::shared_ptr<Layout>> children = {}) {
		auto l = std::make_shared<Layout>(LayoutType::Vertical);
		for (auto& c : children) l->addChild(c);
		return l;
	}

	inline std::shared_ptr<Layout> HStack(std::initializer_list<std::shared_ptr<Layout>> children = {}) {
		auto l = std::make_shared<Layout>(LayoutType::Horizontal);
		for (auto& c : children) l->addChild(c);
		return l;
	}

	// ---------- Text ----------
	class Text : public TLayout<Text> {
	public:
		Text(std::string text) : TLayout<Text>(LayoutType::Stack), text(std::move(text)) { setInteractive(false); }

		void setText(std::string new_text) { text = std::move(new_text); text_dirty = true; pendingTransition = true; }
		void setFont(FontHandle new_font) { font = new_font; text_dirty = true; pendingTransition = true; }

	protected:
		Vec2 computeIntrinsicSize(float availW, float availH) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);

			if (text_dirty || fs != cachedFs || sp != cachedSp) {
				cachedSize = UIContext::get().renderer->measureText(font, text, fs, sp);
				cachedFs = fs;
				cachedSp = sp;
				text_dirty = false;
			}
			return cachedSize;
		}

		void renderSelf(float op, const ComputedStyle& style) override {
			float fs = style.fontSize.resolve(Metrics::viewport.y);
			float sp = style.letterSpacing.resolve(Metrics::viewport.x);
			Vec2 pos = { rect.x, rect.y };

			if (style.itemsH == Align::Center) pos.x += (rect.width - cachedSize.x) * 0.5f;
			if (style.itemsV == Align::Center) pos.y += (rect.height - cachedSize.y) * 0.5f;

			UIContext::get().renderer->drawText(font, text, pos, fs, sp, style.color.withAlpha(op));
		}
	private:
		std::string text;
		FontHandle font{0};
		bool text_dirty{true};
		Vec2 cachedSize;
		float cachedFs{ -1.0f };
		float cachedSp{ -1.0f };
	};
	inline std::shared_ptr<Text> Label(std::string t) { return std::make_shared<Text>(t); }

	// ---------- Panel ----------
	class Panel : public TLayout<Panel> {
	public:
		Panel() : TLayout<Panel>(LayoutType::Stack) { setBlocksRaycast(true); setInteractive(false); }
	protected:
		void renderSelf(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;
			if (bgTexture.valid()) {
				r->drawNineSlice(bgTexture, bgPatchInfo, getRect(), Colors::White.withAlpha(op));
			} else {
				Layout::renderSelf(op, style);
			}
		}
	};
	inline std::shared_ptr<Panel> Pan() { return std::make_shared<Panel>(); }

	// ---------- Button ----------
	class Button : public Panel {
	public:
		Button(std::function<void()> cb) { onClick = std::move(cb); setInteractive(true); }

		std::shared_ptr<Button> cls(const std::string& name) {
			this->styleClasses.push_back(name);
			this->pendingTransition = true;
			return self();
		}
		std::shared_ptr<Button> id(const std::string& node_id) {
			this->nodeId = node_id;
			return self();
		}
		std::shared_ptr<Button> size(Value w, Value h) {
			this->inlineBase.width = w;
			this->inlineBase.height = h;
			this->pendingTransition = true;
			return self();
		}
		std::shared_ptr<Button> self() {
			return std::static_pointer_cast<Button>(this->shared_from_this());
		}
	};

	inline std::shared_ptr<Button> Btn(std::string text, std::function<void()> cb = nullptr) {
		auto b = std::make_shared<Button>(std::move(cb));
		b->addChild(Label(text)->cls("btn-text"));
		return b;
	}

	// ---------- ImageContainer ----------
	class ImageContainer : public TLayout<ImageContainer> {
	public:
		ImageContainer(TextureHandle tex) : TLayout<ImageContainer>(LayoutType::Stack), tex(tex) { setInteractive(false); }
		void setTexture(TextureHandle t) { tex = t; }
	protected:
		void renderSelf(float op, const ComputedStyle& style) override {
			if (tex.valid())
				UIContext::get().renderer->drawTexture(tex, {0,0,(float)tex.width,(float)tex.height}, rect, style.tint.withAlpha(op));
		}
	private:
		TextureHandle tex;
	};

	// ---------- Toggle ----------
	class Toggle : public TLayout<Toggle> {
	public:
		Toggle(bool state = false) : TLayout<Toggle>(LayoutType::Stack), isChecked(state) {
			setInteractive(true);
			inlineBase.background = Colors::DarkGray;
			inlineBase.radius = Px(8.0f);
			pendingTransition = true;

			onClick = [this]() { isChecked = !isChecked; if (onToggle) onToggle(isChecked); };
		}
		std::function<void(bool)> onToggle = nullptr;
		bool isChecked;
	protected:
		void renderSelf(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;
			Color track = isChecked ? Colors::Green : style.background;
			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);
			r->fillRoundedRect(rect, rPx, track.withAlpha(op));

			float knobR = rect.height * 0.4f;
			float knobY = rect.y + rect.height * 0.5f;
			float inset = (rect.height - knobR * 2.0f) * 0.5f;
			float knobX = isChecked ? rect.x + rect.width - knobR - inset
			                        : rect.x + knobR + inset;
			r->fillCircle({ knobX, knobY }, knobR, Colors::White.withAlpha(op));
		}
	};

	// ---------- Slider ----------
	class Slider : public TLayout<Slider> {
	public:
		Slider(float val = 0.5f) : TLayout<Slider>(LayoutType::Stack), value(val) {
			setInteractive(true);
			onPress = [this]() { isDragging = true; };
			onRelease = [this]() { isDragging = false; };
		}
		std::function<void(float)> onValueChanged = nullptr;
	protected:
		void onPostUpdate(float dt) override {
			if (isDragging) {
				float px = UIContext::get().pointer.pos.x;
				float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
				if (percent != value) { value = percent; if (onValueChanged) onValueChanged(value); }
			}
			if (!UIContext::get().pointer.down) isDragging = false;
		}
		void renderSelf(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;
			float th = rect.height * 0.3f;
			Rect track = { rect.x, rect.y + (rect.height - th) * 0.5f, rect.width, th };
			r->fillRoundedRect(track, 4.0f, Colors::DarkGray.withAlpha(op));
			Rect fill = { track.x, track.y, track.width * value, track.height };
			r->fillRoundedRect(fill, 4.0f, style.color.withAlpha(op));
			r->fillCircle({rect.x + rect.width * value, rect.center().y}, rect.height * 0.5f, style.color.withAlpha(op));
		}
	private:
		float value; bool isDragging{false};
	};

	// ---------- CanvasLayout ----------
	class CanvasLayout : public TLayout<CanvasLayout> {
	public:
		CanvasLayout() : TLayout<CanvasLayout>(LayoutType::Stack) { setInteractive(true); }
		~CanvasLayout() { if (target.valid()) UIContext::get().renderer->destroyTarget(target); }
		void draw(float parentOp) override {
			auto r = UIContext::get().renderer;
			if (rect.width <= 0 || rect.height <= 0) return;
			if (!target.valid() || target.width != (int)rect.width || target.height != (int)rect.height) {
				if (target.valid()) r->destroyTarget(target);
				target = r->createTarget((int)rect.width, (int)rect.height);
			}
			float op = currentStyle.opacity * parentOp;
			r->pushTarget(target);
			renderSelf(op, currentStyle);
			for (auto& c : children) c->draw(op);
			r->popTarget();

			Transform2D tr; tr.pivot = rect.center();
			tr.translate = { currentStyle.translateX.resolve(Metrics::viewport.x), currentStyle.translateY.resolve(Metrics::viewport.y) };
			tr.rotationDeg = currentStyle.rotation; tr.scale = currentStyle.scale;

			r->pushTransform(tr);
			if (hasShader && r->supports(Feature::Effects)) r->pushEffect(customEffect);
			r->drawTarget(target, rect, Colors::White.withAlpha(op));
			if (hasShader && r->supports(Feature::Effects)) r->popEffect();
			r->popTransform();
		}
	private:
		TargetHandle target;
	};

	// ---------- Modal ----------
	class Modal : public TLayout<Modal> {
	public:
		Modal() : TLayout<Modal>(LayoutType::Stack) {
			setInteractive(false);
			Style base; base.background = Colors::Black.withAlpha(0.6f); base.opacity = 0.0f;
			setInlineBase(base);
			content = VStack()->cls("modal-content");
			addChild(content);

			auto intro = std::make_shared<UIAnimation>(0.4f);
			intro->addTrack<Value>(VH(100.0f), Value(0.0f), [](Layout* l, Value v){ l->getInlineBase().translateY = v; }, TransitionFunction::EaseOutBack);
			content->addAnimation("Intro", intro);
		}
		void show() {
			setInteractive(true); Style vis; vis.opacity = 1.0f; setInlineBase(vis);
			content->playAnimation("Intro", false);
		}
		void hide() {
			setInteractive(false); Style hid; hid.opacity = 0.0f; setInlineBase(hid);
			content->playAnimation("Intro", true);
		}
		std::shared_ptr<Layout> content;
	};
}