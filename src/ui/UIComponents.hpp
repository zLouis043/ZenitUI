#pragma once

#include "Common.hpp"
#include "Layout.hpp"

namespace ZenitUI::UI {

	inline std::shared_ptr<Layout> VStack(std::initializer_list<std::shared_ptr<Layout>> children = {}) {
		auto l = std::make_shared<Layout>(LayoutType::Vertical);
		l->setStyleTag("VStack");
		for (auto& c : children) l->addChild(c);
		return l;
	}

	inline std::shared_ptr<Layout> HStack(std::initializer_list<std::shared_ptr<Layout>> children = {}) {
		auto l = std::make_shared<Layout>(LayoutType::Horizontal);
		l->setStyleTag("HStack");
		for (auto& c : children) l->addChild(c);
		return l;
	}

	class Text : public TLayout<Text> {
	public:
		Text(std::string text) : TLayout<Text>(LayoutType::Stack), text(std::move(text)) {
			setInteractive(false);
			setStyleTag("Text");
		}

		void setText(std::string new_text) { text = std::move(new_text); text_dirty = true; pendingTransition = true; }
		void setFont(FontHandle new_font) { font = new_font; text_dirty = true; pendingTransition = true; }
		void setWrap(bool w) { wrap_ = w; text_dirty = true; pendingTransition = true; }
		bool getWrap() const { return wrap_; }

	protected:
		Vec2 computeIntrinsicSize(float availW, float /*availH*/) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);

			if (!wrap_ || availW <= 0.0f) {
				if (text_dirty || fs != cachedFs || sp != cachedSp || cachedSize.y == 0.0f) {
					cachedSize = UIContext::get().renderer->measureText(font, text, fs, sp);
					cachedFs = fs; cachedSp = sp; text_dirty = false;
				}
				lines.clear();
				lines.push_back({ text, cachedSize });
				return cachedSize;
			}

			// Ricalcola le linee solo se qualcosa è cambiato
			if (text_dirty || fs != cachedFs || sp != cachedSp || availW != cachedWrapW) {
				wrapText(availW, fs, sp);
				cachedFs = fs; cachedSp = sp; cachedWrapW = availW; text_dirty = false;
			}

			float maxW = 0.0f, totalH = 0.0f;
			for (auto& ln : lines) { maxW = std::max(maxW, ln.size.x); totalH += ln.size.y; }
			cachedSize = { maxW, totalH };
			return cachedSize;
		}

		void renderContent(float op, const ComputedStyle& style) override {
			if (lines.empty()) return;

			float fs = style.fontSize.resolve(Metrics::viewport.y);
			float sp = style.letterSpacing.resolve(Metrics::viewport.x);

			float pl = style.padding.left.resolve(rect.width);
			float pr = style.padding.right.resolve(rect.width);
			float pt = style.padding.top.resolve(rect.height);
			float pb = style.padding.bottom.resolve(rect.height);

			float availW = std::max(0.0f, rect.width  - pl - pr);
			float availH = std::max(0.0f, rect.height - pt - pb);

			float totalTextH = 0.0f;
			for (auto& ln : lines) totalTextH += ln.size.y;

			Vec2 pos = { rect.x + pl, rect.y + pt };
			Align ha = (style.textAlign == Align::Auto) ? style.itemsH : style.textAlign;

			if (style.itemsV == Align::Center)      pos.y += (availH - totalTextH) * 0.5f;
			else if (style.itemsV == Align::End)    pos.y += availH - totalTextH;

			auto r = UIContext::get().renderer;
			r->pushClip(rect);
			float curY = pos.y;
			for (auto& ln : lines) {
				float x = pos.x;
				if (ha == Align::Center)   x += (availW - ln.size.x) * 0.5f;
				else if (ha == Align::End) x += availW - ln.size.x;
				r->drawText(font, ln.text, { x, curY }, fs, sp, style.color.withAlpha(op));
				curY += ln.size.y;
			}
			r->popClip();
		}

		void arrange(Rect space) override {
			if (wrap_) {
				float pl = currentStyle.padding.left.resolve(space.width);
				float pr = currentStyle.padding.right.resolve(space.width);
				float newAvailW = std::max(0.0f, space.width - pl - pr);

				if (newAvailW > 0.0f && newAvailW != cachedWrapW) {
					float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
					float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);
					wrapText(newAvailW, fs, sp);
					cachedFs      = fs;
					cachedSp      = sp;
					cachedWrapW   = newAvailW;

					float maxW = 0.0f, totalH = 0.0f;
					for (auto& ln : lines) { maxW = std::max(maxW, ln.size.x); totalH += ln.size.y; }
					cachedSize = { maxW, totalH };
				}
			}
			Layout::arrange(space);
		}

	private:
		struct Line {
			std::string text;
			Vec2 size;
		};

		void wrapText(float availW, float fs, float sp) {
			lines.clear();
			auto r = UIContext::get().renderer;
			if (!r) return;

			std::string cur;
			std::istringstream iss(text);
			std::string word;

			auto flush = [&]() {
				if (cur.empty()) return;
				Vec2 sz = r->measureText(font, cur, fs, sp);
				lines.push_back({ cur, sz });
				cur.clear();
			};

			while (iss >> word) {
				std::string candidate = cur.empty() ? word : (cur + " " + word);
				Vec2 sz = r->measureText(font, candidate, fs, sp);
				if (sz.x <= availW || cur.empty()) {
					cur = candidate;
				} else {
					flush();
					cur = word;
				}
			}
			flush();
			if (lines.empty()) {
				Vec2 sz = r->measureText(font, text, fs, sp);
				lines.push_back({ text, sz });
			}
		}

		std::string text;
		FontHandle font{0};
		bool text_dirty{true};
		bool wrap_{false};
		Vec2 cachedSize;
		float cachedFs{ -1.0f };
		float cachedSp{ -1.0f };
		float cachedWrapW{ -1.0f };
		std::vector<Line> lines;
	};
	inline std::shared_ptr<Text> Label(std::string t) { return std::make_shared<Text>(t); }

	// ---------- Panel ----------
	class Panel : public TLayout<Panel> {
	public:
		Panel() : TLayout<Panel>(LayoutType::Stack) { 
			setBlocksRaycast(true); 
			setInteractive(false); 
			setStyleTag("Panel");
		}
	protected:
		void renderChrome(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;
			if (!r) return;   
			if (bgTexture.valid()) {
				r->drawNineSlice(bgTexture, bgPatchInfo, getRect(), Colors::White.withAlpha(op));
				// Il bordo si disegna comunque (utile per focus ring)
				Color bc = style.borderColor.withAlpha(op);
				float bw = style.borderWidth.resolve(std::min(rect.width, rect.height) * 2.0f);
				if (bc.a > 0 && bw > 0.0f) r->strokeRect(rect, bw, bc);
			} else {
				Layout::renderChrome(op, style);
			}
		}
	};
	inline std::shared_ptr<Panel> Pan() { return std::make_shared<Panel>(); }

	// ---------- Button ----------
	class Button : public TLayout<Button, Panel> {
	public:
		Button(std::function<void()> cb) { 
			onClick = std::move(cb); 
			setInteractive(true); 
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Button");
			getInlineBase().itemsH = Align::Center;
    		getInlineBase().itemsV = Align::Center;
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
		ImageContainer(TextureHandle tex) : TLayout<ImageContainer>(LayoutType::Stack), tex(tex) { 
			setInteractive(false); 
			setStyleTag("ImageContainer");
		}
		void setTexture(TextureHandle t) { tex = t; }
	protected:
		void renderContent(float op, const ComputedStyle& style) override {
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
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Toggle");
			inlineBase.background = Colors::DarkGray;
			inlineBase.radius = Px(8.0f);
			pendingTransition = true;

			onClick = [this]() {
				if (!isEnabled) return;
				isChecked = !isChecked;
				if (onToggle) onToggle(isChecked);
			};
		}
		std::function<void(bool)> onToggle = nullptr;
		bool isChecked;
	protected:

		Vec2 computeIntrinsicSize(float, float) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return { fs * 2.5f, fs * 1.4f };
		}

		void renderContent(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;

			Color track;
			Color knobColor;

			if (!isEnabled) {
				track     = Color{ 70, 70, 75, 255 };
				knobColor = Color{ 150, 150, 155, 255 };
			} else {
				track     = isChecked ? Colors::Green : style.background;
				knobColor = Colors::White;
			}

			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);
			r->fillRoundedRect(rect, rPx, track.withAlpha(op));

			float knobR = rect.height * 0.4f;
			float knobY = rect.y + rect.height * 0.5f;
			float inset = (rect.height - knobR * 2.0f) * 0.5f;
			float knobX = isChecked ? rect.x + rect.width - knobR - inset
			                        : rect.x + knobR + inset;
			r->fillCircle({ knobX, knobY }, knobR, knobColor.withAlpha(op));
		}
	};

	// ---------- Slider ----------
	class Slider : public TLayout<Slider> {
	public:
		Slider(float val = 0.5f) : TLayout<Slider>(LayoutType::Stack), value(val) {
			setInteractive(true);
			setFocusable(true);
			setStyleTag("Slider");
			onPress = [this]() { isDragging = true; };
			onRelease = [this]() { isDragging = false; };
		}
		std::function<void(float)> onValueChanged = nullptr;
		protected:

		Vec2 computeIntrinsicSize(float, float) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return { 120.0f, std::max(24.0f, fs * 1.2f) };
		}

		void onPostUpdate(float) override {
			if (!isEnabled) { isDragging = false; return; }

			// --- Frecce da tastiera (solo se focusato) ---
			if (isFocused) {
				auto& ev = UIContext::get().inputEvents;
				constexpr float STEP       = 0.05f;
				constexpr float STEP_FINE  = 0.01f;

				for (int k : ev.keys) {
					float delta = 0.0f;
					if (k == Key::Left)  delta = -STEP;
					if (k == Key::Right) delta = +STEP;
					if (delta == 0.0f) continue;

					if (UIContext::get().shiftHeld) delta *= (STEP_FINE / STEP);
					float nv = std::clamp(value + delta, 0.0f, 1.0f);
					if (nv != value) {
						value = nv;
						if (onValueChanged) onValueChanged(value);
						pendingTransition = true;
					}
				}
			}

			// --- Drag col mouse ---
			if (isDragging) {
				float px = UIContext::get().pointer.pos.x;
				float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
				if (percent != value) { value = percent; if (onValueChanged) onValueChanged(value); }
			}
			if (!UIContext::get().pointer.down) isDragging = false;
		}
		void renderContent(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;
			float th = rect.height * 0.3f;
			Rect track = { rect.x, rect.y + (rect.height - th) * 0.5f, rect.width, th };

			Color trackColor = isEnabled ? Colors::DarkGray : Color{ 55, 55, 60, 255 };
			Color fillColor  = isEnabled ? style.color     : Color{ 100, 100, 105, 255 };
			Color knobColor  = isEnabled ? style.color     : Color{ 130, 130, 135, 255 };

			r->fillRoundedRect(track, 4.0f, trackColor.withAlpha(op));
			Rect fill = { track.x, track.y, track.width * value, track.height };
			r->fillRoundedRect(fill, 4.0f, fillColor.withAlpha(op));
			r->fillCircle({rect.x + rect.width * value, rect.center().y}, rect.height * 0.5f, knobColor.withAlpha(op));
		}
	private:
		float value; bool isDragging{false};
	};

	// ---------- CanvasLayout ----------
	class CanvasLayout : public TLayout<CanvasLayout> {
	public:
		CanvasLayout() : TLayout<CanvasLayout>(LayoutType::Stack) { 
			setInteractive(true); 
			setStyleTag("Canvas");
		}
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
			renderChrome(op, currentStyle);
			renderContent(op, currentStyle);
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
			
		}
		void show() {
			setInteractive(true); Style vis; vis.opacity = 1.0f; setInlineBase(vis);
			content->playAnimation("Intro", false);
		}
		void hide() {
			setInteractive(false); Style hid; hid.opacity = 0.0f; setInlineBase(hid);
			content->playAnimation("Intro", true);
		}
	protected:

		void onBuild() override {
			setInteractive(false);
			setStyleTag("Modal");
			setFocusScope(true); 
			Style base; base.background = Colors::Black.withAlpha(0.6f); base.opacity = 0.0f;
			setInlineBase(base);
			content = VStack()->cls("modal-content");
			addChild(content);

			auto intro = std::make_shared<UIAnimation>(0.4f);
			intro->addTrack<Value>(VH(100.0f), Value(0.0f), [](Layout* l, Value v){ l->getInlineBase().translateY = v; }, TransitionFunction::EaseOutBack);
			content->addAnimation("Intro", intro);
		}

		std::shared_ptr<Layout> content;
	};

	// ---------- ScrollView ----------
	class ScrollView : public TLayout<ScrollView> {
	public:
		ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t) {
			setInteractive(true);
			setBlocksRaycast(true);
			setStyleTag("ScrollView");
			inlineBase.overflow = Overflow::Scroll;
			pendingTransition = true;
		}

		// --- API di scroll programmatico ---
		void scrollTo(float x, float y) {
			scrollX = std::max(0.0f, x);
			scrollY = std::max(0.0f, y);
			clampScroll();
			pendingTransition = true;
		}
		void scrollToY(float y) { scrollTo(scrollX, y); }
		void scrollToX(float x) { scrollTo(x, scrollY); }
		void scrollToTop()    { scrollToY(0.0f); }
		void scrollToBottom() { scrollToY(maxScrollY); }
		void scrollToLeft()   { scrollToX(0.0f); }
		void scrollToRight()  { scrollToX(maxScrollX); }

		float getScrollX() const { return scrollX; }
		float getScrollY() const { return scrollY; }
		float getMaxScrollX() const { return maxScrollX; }
		float getMaxScrollY() const { return maxScrollY; }

		void arrange(Rect space) override {
			rect = space;

			float pl = currentStyle.padding.left.resolve(rect.width);
			float pr = currentStyle.padding.right.resolve(rect.width);
			float pt = currentStyle.padding.top.resolve(rect.height);
			float pb = currentStyle.padding.bottom.resolve(rect.height);

			// Spazio per i figli: parte dal viewport, ma sull'asse di scroll
			// viene esteso alla dimensione reale del contenuto. Senza questo,
			// arrangeInto attiva lo shrink del flex (pensato per contenitori
			// a dimensione fissa) e comprime i figli fino a farli sparire.
			Rect offsetSpace = space;
			offsetSpace.x -= scrollX;
			offsetSpace.y -= scrollY;

			if (type == LayoutType::Vertical) {
				float contentH = scrollContentSize.y + pt + pb;
				offsetSpace.height = std::max(space.height, contentH);
			} else if (type == LayoutType::Horizontal) {
				float contentW = scrollContentSize.x + pl + pr;
				offsetSpace.width = std::max(space.width, contentW);
			}

			arrangeInto(offsetSpace);

			// Calcolo maxScroll (invariato)
			float contentW = scrollContentSize.x + pl + pr;
			float contentH = scrollContentSize.y + pt + pb;
			maxScrollX = std::max(0.0f, contentW - rect.width);
			maxScrollY = std::max(0.0f, contentH - rect.height);
			clampScroll();
		}

	protected:
		

		void onPostUpdate(float dt) override {
			auto& ctx = UIContext::get();

			// 1) Drag del thumb
			if (draggingThumb) {
				if (!ctx.pointer.down) {
					draggingThumb = false;
				} else {
					float dy = ctx.pointer.pos.y - dragStartMouseY;
					float ratio = (rect.height > 0.0f) ? (scrollContentSize.y / rect.height) : 1.0f;
					scrollY = std::clamp(dragStartScrollY + dy * ratio, 0.0f, maxScrollY);
					pendingTransition = true;
				}
			}

			// 2) Wheel (solo se il mouse è dentro e nessuno l'ha già consumato)
			if (!ctx.wheelConsumedThisFrame && rect.contains(ctx.pointer.pos) && !draggingThumb) {
				if (ctx.pointer.wheelY != 0.0f) {
					if (IsShiftHeld()) velocityX -= ctx.pointer.wheelY * WHEEL_IMPULSE;
					else               velocityY -= ctx.pointer.wheelY * WHEEL_IMPULSE;
					ctx.wheelConsumedThisFrame = true;
					pendingTransition = true;
				}
			}

			// 3) Momentum
			if (std::abs(velocityX) > VELOCITY_MIN) {
				scrollX += velocityX * dt;
				velocityX *= std::pow(DECAY, dt * 60.0f);
			} else velocityX = 0.0f;

			if (std::abs(velocityY) > VELOCITY_MIN) {
				scrollY += velocityY * dt;
				velocityY *= std::pow(DECAY, dt * 60.0f);
			} else velocityY = 0.0f;

			clampScroll();
		}

		void update(float dt, bool ancestorBlocked) override {
			Layout::update(dt, ancestorBlocked);

			if (rect.height <= 0.0f) return;

			auto& ctx = UIContext::get();
			if (!ctx.pointer.pressed) return;
			if (!rect.contains(ctx.pointer.pos)) return;
			if (maxScrollY <= 0.0f) return;

			Rect thumb = getThumbRect();
			if (thumb.contains(ctx.pointer.pos)) {
				draggingThumb = true;
				dragStartMouseY = ctx.pointer.pos.y;
				dragStartScrollY = scrollY;
				velocityX = velocityY = 0.0f;
				return;
			}

			// Se il click è andato a un figlio interattivo, non fare scroll jump.
			// Solo un click sullo sfondo della ScrollView (topmostConsumer == this)
			// provoca il salto di pagina.
			Layout* top = ctx.topmostConsumer;
			if (top && top != this) return;

			float ratio = (ctx.pointer.pos.y - rect.y) / rect.height;
			scrollY = std::clamp(ratio * scrollContentSize.y - rect.height * 0.5f,
								0.0f, maxScrollY);
			pendingTransition = true;
		}

		/*/ Disegna la scrollbar (il contenuto lo disegnano i figli).
		void renderContent(float op, const ComputedStyle&) override {
			const float MARGIN = 4.0f;
    		if (maxScrollY <= 0.0f || rect.height <= MARGIN * 2.0f + 1.0f) return; 

			auto r = UIContext::get().renderer;
			const float TRACK_W = 6.0f;

			Rect track = {
				rect.x + rect.width - TRACK_W - MARGIN,
				rect.y + MARGIN,
				TRACK_W,
				rect.height - MARGIN * 2
			};
			r->fillRoundedRect(track, TRACK_W * 0.5f, Color{ 40, 40, 40, 180 }.withAlpha(op));
		}*/

		void draw(float parentOpacity) override {
			Layout::draw(parentOpacity);

			// Ridisegna la scrollbar DOPO i figli, così sta sopra.
			auto r = UIContext::get().renderer;
			if (!r || maxScrollY <= 0.0f || rect.height <= 10.0f) return;

			const float MARGIN = 4.0f;
			const float TRACK_W = 6.0f;
			float op = currentStyle.opacity * parentOpacity;
			if (op <= 0.001f) return;

			// Clip sul rect dello ScrollView per non sbordare
			r->pushClip(rect);

			Rect track = {
				rect.x + rect.width - TRACK_W - MARGIN,
				rect.y + MARGIN,
				TRACK_W,
				rect.height - MARGIN * 2
			};
			r->fillRoundedRect(track, TRACK_W * 0.5f, Color{ 40, 40, 40, 180 }.withAlpha(op));

			Rect thumb = getThumbRect();
			Color thumbColor = draggingThumb
				? Color{ 180, 180, 180, 255 }
				: Color{ 130, 130, 130, 220 };
			r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));

			r->popClip();
		}

	private:
		float scrollX{ 0.0f };
		float scrollY{ 0.0f };
		float maxScrollX{ 0.0f };
		float maxScrollY{ 0.0f };

		float velocityX{ 0.0f };
		float velocityY{ 0.0f };

		static constexpr float WHEEL_IMPULSE = 400.0f;
		static constexpr float DECAY         = 0.90f;
		static constexpr float VELOCITY_MIN  = 5.0f;

		bool  draggingThumb{ false };
		float dragStartMouseY{ 0.0f };
		float dragStartScrollY{ 0.0f };

		void clampScroll() {
			scrollX = std::clamp(scrollX, 0.0f, maxScrollX);
			scrollY = std::clamp(scrollY, 0.0f, maxScrollY);
		}

		Rect getThumbRect() const {
			const float TRACK_W = 6.0f;
			const float MARGIN  = 4.0f;

			float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f);               // <-- max
			float contentH = scrollContentSize.y;
			float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
			float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio)); // <-- min
			float scrollRatio = (maxScrollY > 0.0f) ? (scrollY / maxScrollY) : 0.0f;
			float available = std::max(0.0f, trackH - thumbH);                        // <-- max
			float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

			return { rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH };
		}

		static bool IsShiftHeld() { return UIContext::get().shiftHeld; }
	};

	// ---------- Checkbox ----------
	class Checkbox : public TLayout<Checkbox> {
	public:
		Checkbox(bool state = false) : TLayout<Checkbox>(LayoutType::Stack), isChecked(state) {
			setInteractive(true);
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Checkbox");
			inlineBase.background = Colors::DarkGray;
			inlineBase.radius = Px(6.0f);
			pendingTransition = true;

			onClick = [this]() {
				if (!isEnabled) return;
				isChecked = !isChecked;
				if (onToggle) onToggle(isChecked);
			};
		}
		std::function<void(bool)> onToggle = nullptr;
		bool isChecked;

	protected:

		Vec2 computeIntrinsicSize(float, float) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return { fs * 1.4f, fs * 1.4f };
		}

		void renderContent(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;

			Color box;
			Color check;

			if (!isEnabled) {
				box   = Color{ 70, 70, 75, 255 };
				check = Color{ 150, 150, 155, 255 };
			} else if (isChecked) {
				box   = Colors::Green;
				check = Colors::White;
			} else {
				box   = style.background;
				check = Colors::White;
			}

			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);
			r->fillRoundedRect(rect, rPx, box.withAlpha(op));

			if (isChecked) {
				// Due "check marks" disegnati come rettangoli ruotati è complicato.
				// Usiamo un piccolo quadrato centrale come indicatore, semplice e leggibile.
				float inner = rect.height * 0.35f;
				Rect mark = {
					rect.center().x - inner * 0.5f,
					rect.center().y - inner * 0.5f,
					inner, inner
				};
				r->fillRoundedRect(mark, 3.0f, check.withAlpha(op));
			}
		}
	};

		// ---------- ProgressBar ----------
	class ProgressBar : public TLayout<ProgressBar> {
	public:
		ProgressBar(float v = 0.0f) : TLayout<ProgressBar>(LayoutType::Stack), value(std::clamp(v, 0.0f, 1.0f)) {
			setInteractive(false);
			setStyleTag("ProgressBar");
			inlineBase.background = Colors::DarkGray;
			inlineBase.color = Colors::Green;
			inlineBase.radius = Px(4.0f);
			pendingTransition = true;
		}

		void setValue(float v) { value = std::clamp(v, 0.0f, 1.0f); pendingTransition = true; }
		float getValue() const { return value; }

	protected:

		Vec2 computeIntrinsicSize(float, float) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return { 120.0f, std::max(16.0f, fs * 0.8f) };
		}

		void renderContent(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;

			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);

			// Track
			r->fillRoundedRect(rect, rPx, style.background.withAlpha(op));

			// Fill
			float w = rect.width * value;
			if (w > 0.5f) {
				Rect fill = { rect.x, rect.y, w, rect.height };
				Color fillColor = isEnabled ? style.color : Color{ 90, 90, 95, 255 };
				r->fillRoundedRect(fill, rPx, fillColor.withAlpha(op));
			}
		}

	private:
		float value;
	};

		// ---------- TextInput ----------
	class TextInput : public TLayout<TextInput> {
	public:
		TextInput(std::string initial = "") : TLayout<TextInput>(LayoutType::Stack), text(std::move(initial)) {
			setInteractive(true);
			setFocusable(true);
			setStyleTag("TextInput");
			cursorPos = (int)text.size();

			onClick = [this]() {
				if (!isEnabled) return;
				UIContext::get().requestFocus(shared_from_this());
			};

			// Stile di default
			inlineBase.background = Color{ 30, 30, 36, 255 };
			inlineBase.color = Colors::White;
			inlineBase.radius = Px(6.0f);
			inlineBase.padding = Spacing(VH(1.2f), VW(1.5f));
			inlineBase.borderColor = Color{ 60, 60, 70, 255 };
			inlineBase.borderWidth = Px(1.0f);
			pendingTransition = true;
		}

		// API
		void setText(std::string t) {
			text = std::move(t);
			cursorPos = std::min(cursorPos, (int)text.size());
			text_dirty = true;
			pendingTransition = true;
		}
		const std::string& getText() const { return text; }
		void clear() { setText(""); }

		std::function<void(const std::string&)> onTextChanged;
		std::function<void(const std::string&)> onSubmit;

	protected:
		Vec2 computeIntrinsicSize(float, float) override {
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return { std::max(120.0f, fs * 12.0f), fs * 1.6f };
		}

				void onPostUpdate(float dt) override {
			if (!isEnabled) { hasFocusCache = false; repeatStates.clear(); return; }

			hasFocusCache = UIContext::get().hasFocus(this);
			cursorBlink += dt;
			if (cursorBlink > 1.0f) cursorBlink -= 1.0f;

			if (!hasFocusCache) { repeatStates.clear(); return; }

			auto& ev = UIContext::get().inputEvents;
			bool changed = false;

			// --- Caratteri (solo rising edge) ---
			for (int c : ev.chars) {
				if (c < 32 || c > 126) continue;
				text.insert(text.begin() + cursorPos, (char)c);
				cursorPos++;
				changed = true;
			}

			// --- Azioni per un tasto di editing ---
			auto doAction = [&](int key) {
				switch (key) {
				case Key::Backspace:
					if (cursorPos > 0) { text.erase(text.begin() + cursorPos - 1); cursorPos--; changed = true; }
					break;
				case Key::Delete:
					if (cursorPos < (int)text.size()) { text.erase(text.begin() + cursorPos); changed = true; }
					break;
				case Key::Left:
					if (cursorPos > 0) { cursorPos--; changed = true; }
					break;
				case Key::Right:
					if (cursorPos < (int)text.size()) { cursorPos++; changed = true; }
					break;
				}
			};

			// --- Tasti "one-shot": Enter, Escape, Home, End (solo rising edge) ---
			for (int k : ev.keys) {
				switch (k) {
				case Key::Enter:
					if (onSubmit) onSubmit(text);
					UIContext::get().releaseFocus();
					hasFocusCache = false;
					repeatStates.clear();
					return;
				case Key::Escape:
					UIContext::get().releaseFocus();
					hasFocusCache = false;
					repeatStates.clear();
					return;
				case Key::Home: cursorPos = 0; changed = true; break;
				case Key::End:  cursorPos = (int)text.size(); changed = true; break;
				default: break;
				}
			}

			// --- Tasti con repeat: Backspace, Delete, Left, Right ---
			constexpr float REPEAT_DELAY    = 0.40f;   // prima ripetizione
			constexpr float REPEAT_INTERVAL = 0.04f;   // intervallo successivo

			auto handleRepeat = [&](int key) {
				bool isHeld = std::find(ev.held.begin(), ev.held.end(), key) != ev.held.end();
				auto it = repeatStates.find(key);

				if (!isHeld) {
					if (it != repeatStates.end()) repeatStates.erase(it);
					return;
				}
				if (it == repeatStates.end()) {
					// rising edge: prima esecuzione immediata
					doAction(key);
					RepeatState st;
					st.holdTime = 0.0f;
					st.nextFireTime = REPEAT_DELAY;
					repeatStates[key] = st;
				} else {
					it->second.holdTime += dt;
					while (it->second.holdTime >= it->second.nextFireTime) {
						doAction(key);
						it->second.nextFireTime += REPEAT_INTERVAL;
					}
				}
			};

			handleRepeat(Key::Backspace);
			handleRepeat(Key::Delete);
			handleRepeat(Key::Left);
			handleRepeat(Key::Right);

			if (changed) {
				if (onTextChanged) onTextChanged(text);
				pendingTransition = true;
			}

			// --- Auto-scroll orizzontale (invariato) ---
			{
				auto r = UIContext::get().renderer;
				if (!r) return;
				float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
				float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);
				float pl = currentStyle.padding.left.resolve(rect.width);
				float availW = std::max(0.0f, rect.width - pl * 2.0f);

				std::string prefix = text.substr(0, cursorPos);
				Vec2 prefixSize = r->measureText(FontHandle{}, prefix, fs, sp);
				Vec2 fullSize   = r->measureText(FontHandle{}, text,   fs, sp);

				if (prefixSize.x - scrollX > availW) scrollX = prefixSize.x - availW;
				if (prefixSize.x - scrollX < 0.0f)   scrollX = prefixSize.x;

				float maxScroll = std::max(0.0f, fullSize.x - availW);
				scrollX = std::clamp(scrollX, 0.0f, maxScroll);
			}
		}

		void renderContent(float op, const ComputedStyle& style) override {
			auto r = UIContext::get().renderer;

			// Testo
			float fs = style.fontSize.resolve(Metrics::viewport.y);
			float sp = style.letterSpacing.resolve(Metrics::viewport.x);

			float pl = style.padding.left.resolve(rect.width);

			// Clip sul rect per nascondere l'overflow di testo
			r->pushClip(rect);

			Vec2 pos = { rect.x + pl - scrollX, rect.y + (rect.height - fs) * 0.5f - fs * 0.1f };
			Color textColor = isEnabled ? style.color : Color{ 120, 120, 125, 255 };
			r->drawText(FontHandle{}, text, pos, fs, sp, textColor.withAlpha(op));

			// Cursore (posizionato relativamente allo scrollX)
			if (hasFocusCache && isEnabled && cursorBlink < 0.5f) {
				std::string prefix = text.substr(0, cursorPos);
				Vec2 prefixSize = r->measureText(FontHandle{}, prefix, fs, sp);
				float cx = pos.x + prefixSize.x;
				float cy = pos.y;
				float ch = fs;
				r->fillRect({ cx, cy, 2.0f, ch }, textColor.withAlpha(op));
			}

			r->popClip();
		}

	private:
		std::string text;
		int cursorPos{ 0 };
		bool text_dirty{ true };
		bool hasFocusCache{ false };
		float cursorBlink{ 0.0f };
		float scrollX{ 0.0f };
		struct RepeatState { float holdTime{ 0.0f }; float nextFireTime{ 0.0f }; };
		std::unordered_map<int, RepeatState> repeatStates;
	};

		// ---------- Dropdown ----------
	class Dropdown : public TLayout<Dropdown> {
	public:
		Dropdown(std::vector<std::string> options, int selected = 0)
			: TLayout<Dropdown>(LayoutType::Vertical),
			  options(std::move(options)), selected(selected) {}

		int  getSelected() const { return selected; }
		void setSelected(int idx) {
			if (idx < 0 || idx >= (int)options.size()) return;
			selected = idx;
			refreshButtonText();
			if (onChange) onChange(selected, options[selected]);
			pendingTransition = true;
		}

		bool getOpen() const { return isOpen; }
		void setOpen(bool o) {
			if (isOpen == o) return;
			isOpen = o;
			applyOpenState();
		}

		std::function<void(int, const std::string&)> onChange;

	protected:
		void onPostUpdate(float) override {
			if (!isOpen) return;
			auto& ctx = UIContext::get();
			if (!ctx.pointer.pressed) return;
			if (!rect.contains(ctx.pointer.pos) &&
			    !listContainer->getRect().contains(ctx.pointer.pos)) {
				setOpen(false);
			}
		}

		void onBuild() override {
			setStyleTag("Dropdown");
			setInteractive(true);
			setFocusable(true);
			pendingTransition = true;

			getInlineBase().position = Position::Relative;
			getInlineBase().zIndex = ZIndex(10);

			button = Btn("", [this]() { toggleOpen(); });
			button->getInlineBase().width  = Percent(100);
			button->getInlineBase().height = VH(3.0f);
			button->getInlineBase().background = Color{ 40, 40, 45, 255 }; 
			button->getInlineBase().radius = Px(6.0f);
			button->getInlineBase().borderColor = Color{ 70, 70, 80, 255 };
			button->getInlineBase().borderWidth = Px(1.0f);
			button->getInlineBase().itemsH = Align::Center;
			button->getInlineBase().itemsV = Align::Center;
			addChild(button);
			listContainer = std::make_shared<ScrollView>();
			listContainer->getInlineBase().position = Position::Absolute;
			listContainer->setPortal(true);
			listContainer->getInlineBase().width       = VW(15.0f);
			listContainer->getInlineBase().height      = VH(25.0f);
			listContainer->getInlineBase().background  = Color{ 30, 30, 36, 255 };
			listContainer->getInlineBase().borderColor = Color{ 60, 60, 70, 255 };
			listContainer->getInlineBase().borderWidth = Px(1.0f);
			listContainer->getInlineBase().itemsH      = Align::Stretch;
			for (size_t i = 0; i < options.size(); ++i) {
				int idx = (int)i;
				auto item = Label(options[i]);
				item->getInlineBase().padding    = Spacing(VH(1.0f), VW(1.0f));
				item->getInlineBase().itemsV  = Align::Center;
				item->getInlineBase().background = Color{ 40, 40, 50, 255 };
				item->getInlineBase().color      = Colors::White;
				item->setInteractive(true);
				item->setFocusable(true);
				item->onClick = [this, idx]() {
					setSelected(idx);
					setOpen(false);
				};
				listContainer->addChild(item);
			}
			addChild(listContainer);
			isOpen = false;
			applyOpenState();
			refreshButtonText();
		}

		void arrange(Rect space) override {
			TLayout<Dropdown>::arrange(space);
			
			if (button && listContainer) {
				Rect btnRect = button->getRect();
				listContainer->arrange({
					btnRect.x, 
					btnRect.y + btnRect.height + 4.0f,
					btnRect.width,
					listContainer->getMeasuredSize().y
				});
			}
		}

	private:
		std::vector<std::string> options;
		int selected{ 0 };
		bool isOpen{ false };
		std::shared_ptr<Button> button;
		std::shared_ptr<ScrollView> listContainer;

		void toggleOpen() { setOpen(!isOpen); }

		void applyOpenState() {
			listContainer->setEnabled(isOpen);
			listContainer->setInteractive(isOpen);
			listContainer->getInlineBase().opacity = isOpen ? 1.0f : 0.0f;
			pendingTransition = true;
		}

		void refreshButtonText() {
			if (selected < 0 || selected >= (int)options.size()) return;
			if (button->children.empty()) return;
			auto t = std::dynamic_pointer_cast<Text>(button->children.front());
			if (t) t->setText(options[selected]);
		}
	};
}