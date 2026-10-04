#pragma once

#include "Common.hpp"
#include "Layout.hpp"

// ============================================================
//  COME SCRIVERE UN NUOVO WIDGET
// ============================================================
//
//  class MioWidget : public TLayout<MioWidget> {
//  public:
//      MioWidget(...) : TLayout<MioWidget>(LayoutType::Stack) {
//          setStyleTag("MioWidget");     // obbligatorio: per il CSS
//          // config comportamento (interactive, focusable, ecc.)
//          // default inline via getInlineBase().xxx = ...
//          // callback (onClick, ecc.)
//      }
//
//  protected:
//      // 1) Solo se hai una size intrinseca (testo, icona, ecc.)
//      Vec2 computeIntrinsicSize(float availW, float availH) override { ... }
//
//      // 2) Solo se devi ricalcolare qualcosa dopo aver saputo la tua size finale
//      //    (es. wrap di testo, layout interno custom, ...)
//      void onLayout() override { ... }
//
//      // 3) Solo se hai logica per-frame (input, animazioni locali, drag, ...)
//      void onUpdate(float dt) override { ... }
//
//      // 4) QUASI SEMPRE: il disegno del tuo contenuto
//      void renderContent(float op, const ComputedStyle& style) override { ... }
//
//      // 5) Solo se hai un "vestito" particolare (nine-slice texture, effetto custom)
//      void renderChrome(float op, const ComputedStyle& style) override { ... }
//
//      // 6) Solo se il tuo layout è strutturalmente diverso (scroll, portal, ...)
//      void arrange(Rect space) override { ... }
//
//      // 7) Solo in casi estremi: se devi renderizzare su texture target
//      void draw(float parentOpacity) override { ... }
//  };
//
//  Regola d'oro: se il tuo widget scrive più di 2-3 hook, chiediti se sta
//  facendo troppo. Spesso puoi spostare logica in una funzione helper o
//  in una classe separata.
// ============================================================

namespace ZenitUI::UI
{

	inline std::shared_ptr<Layout> VStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
	{
		auto l = std::make_shared<Layout>(LayoutType::Vertical);
		l->setStyleTag("VStack");
		for (auto &c : children)
			l->addChild(c);
		return l;
	}

	inline std::shared_ptr<Layout> HStack(std::initializer_list<std::shared_ptr<Layout>> children = {})
	{
		auto l = std::make_shared<Layout>(LayoutType::Horizontal);
		l->setStyleTag("HStack");
		for (auto &c : children)
			l->addChild(c);
		return l;
	}

	class Text : public TLayout<Text>
	{
	public:
		Text(std::string text) : TLayout<Text>(LayoutType::Stack), text(std::move(text))
		{
			setInteractive(false);
			setStyleTag("Text");
		}

		void setText(std::string new_text)
		{
			text = std::move(new_text);
			text_dirty = true;
			pendingTransition = true;
		}

		// Font di fallback del widget: usato SOLO se il CSS non specifica `font:`
		// (o se l'asset CSS non esiste nel provider).
		void setFont(FontHandle f)
		{
			defaultFont = f;
			text_dirty = true;
			pendingTransition = true;
		}

		void setWrap(bool w)
		{
			wrap_ = w;
			text_dirty = true;
			pendingTransition = true;
		}
		bool getWrap() const { return wrap_; }

	protected:
		Vec2 computeIntrinsicSize(float availW, float /*availH*/) override
		{
			FontHandle f = resolveFont(currentStyle, defaultFont);
			if (wrap_ && availW > 0.0f)
				return measureWrapped(availW, f);
			return measureSingleLine(f);
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			if (lines.empty())
				return;

			// Il font può cambiare per stato (:hover con font diverso, ecc.),
			// quindi lo risolviamo qui ad ogni frame di draw.
			FontHandle f = resolveFont(style, defaultFont);

			float fs = style.fontSize.resolve(Metrics::viewport.y);
			float sp = style.letterSpacing.resolve(Metrics::viewport.x);

			float pl = style.padding.left.resolve(rect.width);
			float pr = style.padding.right.resolve(rect.width);
			float pt = style.padding.top.resolve(rect.height);
			float pb = style.padding.bottom.resolve(rect.height);

			float availW = std::max(0.0f, rect.width - pl - pr);
			float availH = std::max(0.0f, rect.height - pt - pb);

			float totalTextH = 0.0f;
			for (auto &ln : lines)
				totalTextH += ln.size.y;

			Vec2 pos = {rect.x + pl, rect.y + pt};
			Align ha = (style.textAlign == Align::Auto) ? style.itemsH : style.textAlign;

			if (style.itemsV == Align::Center)
				pos.y += (availH - totalTextH) * 0.5f;
			else if (style.itemsV == Align::End)
				pos.y += availH - totalTextH;

			auto r = UIContext::get().renderer;
			r->pushClip(rect);
			float curY = pos.y;
			for (auto &ln : lines)
			{
				float x = pos.x;
				if (ha == Align::Center)
					x += (availW - ln.size.x) * 0.5f;
				else if (ha == Align::End)
					x += availW - ln.size.x;
				r->drawText(f, ln.text, {x, curY}, fs, sp, style.color.withAlpha(op));
				curY += ln.size.y;
			}
			r->popClip();
		}

		void onLayout() override
		{
			if (!wrap_)
				return;

			float pl = currentStyle.padding.left.resolve(rect.width);
			float pr = currentStyle.padding.right.resolve(rect.width);
			float newAvailW = std::max(0.0f, rect.width - pl - pr);

			if (newAvailW > 0.0f && newAvailW != cachedWrapW)
			{
				FontHandle f = resolveFont(currentStyle, defaultFont);
				float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
				float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);

				wrapText(newAvailW, fs, sp, f);
				cachedFs = fs;
				cachedSp = sp;
				cachedWrapW = newAvailW;
				cachedFontId = f.id;

				float maxW = 0.0f, totalH = 0.0f;
				for (auto &ln : lines)
				{
					maxW = std::max(maxW, ln.size.x);
					totalH += ln.size.y;
				}
				cachedSize = {maxW, totalH};
			}
		}

	private:
		struct Line
		{
			std::string text;
			Vec2 size;
		};

		void wrapText(float availW, float fs, float sp, FontHandle f)
		{
			lines.clear();
			auto r = UIContext::get().renderer;
			if (!r)
				return;

			std::string cur;
			std::istringstream iss(text);
			std::string word;

			auto flush = [&]()
			{
				if (cur.empty())
					return;
				Vec2 sz = r->measureText(f, cur, fs, sp);
				lines.push_back({cur, sz});
				cur.clear();
			};

			while (iss >> word)
			{
				std::string candidate = cur.empty() ? word : (cur + " " + word);
				Vec2 sz = r->measureText(f, candidate, fs, sp);
				if (sz.x <= availW || cur.empty())
				{
					cur = candidate;
				}
				else
				{
					flush();
					cur = word;
				}
			}
			flush();
			if (lines.empty())
			{
				Vec2 sz = r->measureText(f, text, fs, sp);
				lines.push_back({text, sz});
			}
		}

		Vec2 measureSingleLine(FontHandle f)
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);

			// Invalida la cache se cambia testo, font (id), fontSize o letterSpacing.
			if (text_dirty || f.id != cachedFontId ||
				fs != cachedFs || sp != cachedSp || cachedSize.y == 0.0f)
			{
				cachedSize = UIContext::get().renderer->measureText(f, text, fs, sp);
				cachedFs = fs;
				cachedSp = sp;
				cachedFontId = f.id;
				text_dirty = false;
			}
			lines.clear();
			lines.push_back({text, cachedSize});
			return cachedSize;
		}

		Vec2 measureWrapped(float availW, FontHandle f)
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);

			if (text_dirty || f.id != cachedFontId ||
				fs != cachedFs || sp != cachedSp || availW != cachedWrapW)
			{
				wrapText(availW, fs, sp, f);
				cachedFs = fs;
				cachedSp = sp;
				cachedWrapW = availW;
				cachedFontId = f.id;
				text_dirty = false;
			}

			float maxW = 0.0f, totalH = 0.0f;
			for (auto &ln : lines)
			{
				maxW = std::max(maxW, ln.size.x);
				totalH += ln.size.y;
			}
			cachedSize = {maxW, totalH};
			return cachedSize;
		}

		std::string text;
		FontHandle defaultFont{0}; // fallback del widget (CSS vince)
		bool text_dirty{true};
		bool wrap_{false};
		Vec2 cachedSize;
		float cachedFs{-1.0f};
		float cachedSp{-1.0f};
		float cachedWrapW{-1.0f};
		uint32_t cachedFontId{0}; // ultimo id di font risolto
		std::vector<Line> lines;
	};
	inline std::shared_ptr<Text> Label(std::string t) { return std::make_shared<Text>(t); }

	// ---------- Panel ----------
	class Panel : public TLayout<Panel>
	{
	public:
		Panel() : TLayout<Panel>(LayoutType::Stack)
		{
			setBlocksRaycast(true);
			setInteractive(false);
			setStyleTag("Panel");
		}
	};
	inline std::shared_ptr<Panel> Pan() { return std::make_shared<Panel>(); }

	// ---------- Button ----------
	class Button : public TLayout<Button, Panel>
	{
	public:
		Button(std::function<void()> cb)
		{
			onClick = std::move(cb);
			setInteractive(true);
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Button");
			getInlineBase().itemsH = Align::Center;
			getInlineBase().itemsV = Align::Center;
		}
	};

	inline std::shared_ptr<Button> Btn(std::string text, std::function<void()> cb = nullptr)
	{
		auto b = std::make_shared<Button>(std::move(cb));
		b->addChild(Label(text)->cls("btn-text"));
		return b;
	}

	// ---------- ImageContainer ----------
	class ImageContainer : public TLayout<ImageContainer>
	{
	public:
		ImageContainer(TextureHandle tex) : TLayout<ImageContainer>(LayoutType::Stack), tex(tex)
		{
			setInteractive(false);
			setStyleTag("ImageContainer");
		}
		void setTexture(TextureHandle t) { tex = t; }

	protected:
		void renderContent(float op, const ComputedStyle &style) override
		{
			if (tex.valid())
				UIContext::get().renderer->drawTexture(tex, {0, 0, (float)tex.width, (float)tex.height}, rect, style.tint.withAlpha(op));
		}

	private:
		TextureHandle tex;
	};

	// ---------- Toggle ----------
	class Toggle : public TLayout<Toggle>
	{
	public:
		Toggle(bool state = false) : TLayout<Toggle>(LayoutType::Stack), isChecked(state)
		{
			setInteractive(true);
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Toggle");

			// Default inline (il CSS può sempre sovrascrivere).
			getInlineBase().background = Colors::DarkGray;
			getInlineBase().radius = Px(8.0f);

			onClick = [this]()
			{
				isChecked = !isChecked;
				if (onToggle)
					onToggle(isChecked);
			};
		}

		std::function<void(bool)> onToggle = nullptr;
		bool isChecked;

	protected:
		Vec2 computeIntrinsicSize(float, float) override
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return {fs * 2.5f, fs * 1.4f};
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			auto r = UIContext::get().renderer;

			Style trackStyle = partStyle("track");
			Style knobStyle = partStyle("knob");

			// Track: background dal ::track se settato, altrimenti comportamento di default
			Color track;
			if (!isEnabled)
			{
				track = Color{70, 70, 75, 255};
			}
			else if (isChecked)
			{
				// checked vince: verde di default, o color del ::track se vuoi override
				track = trackStyle.color.is_set ? trackStyle.color.value : Colors::Green;
			}
			else
			{
				track = trackStyle.background.is_set ? trackStyle.background.value : style.background;
			}

			// Knob: color dal ::knob se settato, altrimenti bianco
			Color knobColor = Colors::White;
			if (!isEnabled)
				knobColor = Color{150, 150, 155, 255};
			if (knobStyle.color.is_set)
				knobColor = knobStyle.color.value;

			// Radius del track: dal ::track se settato, altrimenti da style
			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = trackStyle.radius.is_set
							? std::clamp(trackStyle.radius.value.resolve(maxR * 2.0f), 0.0f, maxR)
							: std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);

			r->fillRoundedRect(rect, rPx, track.withAlpha(op));

			float knobR = rect.height * 0.4f;
			float knobY = rect.y + rect.height * 0.5f;
			float inset = (rect.height - knobR * 2.0f) * 0.5f;
			float knobX = isChecked ? rect.x + rect.width - knobR - inset
									: rect.x + knobR + inset;
			r->fillCircle({knobX, knobY}, knobR, knobColor.withAlpha(op));
		}
	};

	// ---------- Slider ----------
	class Slider : public TLayout<Slider>
	{
	public:
		Slider(float val = 0.5f) : TLayout<Slider>(LayoutType::Stack), value(val)
		{
			setInteractive(true);
			setFocusable(true);
			setStyleTag("Slider");
		}

		std::function<void(float)> onValueChanged = nullptr;

	protected:
		Vec2 computeIntrinsicSize(float, float) override
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return {120.0f, std::max(24.0f, fs * 1.2f)};
		}

		void onUpdate(float) override
		{
			// Frecce da tastiera (solo se focusato)
			if (isFocused)
			{
				auto &ev = UIContext::get().inputEvents;
				constexpr float STEP = 0.05f;
				constexpr float STEP_FINE = 0.01f;

				for (int k : ev.keys)
				{
					float delta = 0.0f;
					if (k == Key::Left)
						delta = -STEP;
					if (k == Key::Right)
						delta = +STEP;
					if (delta == 0.0f)
						continue;

					if (UIContext::get().shiftHeld)
						delta *= (STEP_FINE / STEP);
					float nv = std::clamp(value + delta, 0.0f, 1.0f);
					if (nv != value)
					{
						value = nv;
						if (onValueChanged)
							onValueChanged(value);
					}
				}
			}

			// Drag: inizia quando isPressed diventa true per la prima volta,
			// continua finché il tasto è down (anche fuori dal widget, grazie al capture).
			if (isPressed && !hasPointerCapture())
			{
				capturePointer();
			}

			if (hasPointerCapture() && rect.width > 0.0f)
			{
				float px = UIContext::get().pointer.pos.x;
				float percent = std::clamp((px - rect.x) / rect.width, 0.0f, 1.0f);
				if (percent != value)
				{
					value = percent;
					if (onValueChanged)
						onValueChanged(value);
				}
			}
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			auto r = UIContext::get().renderer;

			Style trackStyle = partStyle("track");
			Style fillStyle = partStyle("fill");
			Style knobStyle = partStyle("knob");

			float th = rect.height * 0.3f;
			Rect trackRect = {rect.x, rect.y + (rect.height - th) * 0.5f, rect.width, th};

			// Track
			Color trackColor = isEnabled ? Colors::DarkGray : Color{55, 55, 60, 255};
			if (trackStyle.background.is_set)
				trackColor = trackStyle.background.value;

			float trackRadius = trackStyle.radius.is_set
									? trackStyle.radius.value.resolve(th * 0.5f)
									: 4.0f;

			r->fillRoundedRect(trackRect, trackRadius, trackColor.withAlpha(op));

			// Fill
			Color fillColor = isEnabled ? style.color : Color{100, 100, 105, 255};
			if (fillStyle.color.is_set)
				fillColor = fillStyle.color.value;

			Rect fillRect = {trackRect.x, trackRect.y, trackRect.width * value, trackRect.height};
			r->fillRoundedRect(fillRect, trackRadius, fillColor.withAlpha(op));

			// Knob
			Color knobColor = isEnabled ? style.color : Color{130, 130, 135, 255};
			if (knobStyle.color.is_set)
				knobColor = knobStyle.color.value;

			float knobR = knobStyle.radius.is_set
							  ? knobStyle.radius.value.resolve(rect.height * 0.5f)
							  : rect.height * 0.5f;

			r->fillCircle({rect.x + rect.width * value, rect.center().y},
						  knobR, knobColor.withAlpha(op));
		}

	private:
		float value;
	};

	// ---------- CanvasLayout ----------
	class CanvasLayout : public TLayout<CanvasLayout>
	{
	public:
		CanvasLayout() : TLayout<CanvasLayout>(LayoutType::Stack)
		{
			setInteractive(true);
			setStyleTag("Canvas");
		}
		~CanvasLayout()
		{
			if (target.valid())
				UIContext::get().renderer->destroyTarget(target);
		}
		void draw(float parentOp) override
		{
			auto r = UIContext::get().renderer;
			if (rect.width <= 0 || rect.height <= 0)
				return;
			if (!target.valid() || target.width != (int)rect.width || target.height != (int)rect.height)
			{
				if (target.valid())
					r->destroyTarget(target);
				target = r->createTarget((int)rect.width, (int)rect.height);
			}
			float op = currentStyle.opacity * parentOp;
			r->pushTarget(target);
			renderChrome(op, currentStyle);
			renderContent(op, currentStyle);
			for (auto &c : children)
				c->draw(op);
			r->popTarget();

			Transform2D tr;
			tr.pivot = rect.center();
			tr.translate = {currentStyle.translateX.resolveSelf(rect.width), currentStyle.translateY.resolveSelf(rect.height)};
			tr.rotationDeg = currentStyle.rotation;
			tr.scale = currentStyle.scale;

			r->pushTransform(tr);
			if (hasShader && r->supports(Feature::Effects))
				r->pushEffect(customEffect);
			r->drawTarget(target, rect, Colors::White.withAlpha(op));
			if (hasShader && r->supports(Feature::Effects))
				r->popEffect();
			r->popTransform();
		}

	private:
		TargetHandle target;
	};

	// ---------- Modal ----------
	class Modal : public TLayout<Modal>
	{
	public:
		Modal() : TLayout<Modal>(LayoutType::Stack)
		{
		}
		void show()
		{
			setInteractive(true);
			Style vis;
			vis.opacity = 1.0f;
			setInlineBase(vis);
			content->playAnimation("Intro", false);
		}
		void hide()
		{
			setInteractive(false);
			Style hid;
			hid.opacity = 0.0f;
			setInlineBase(hid);
			content->playAnimation("Intro", true);
		}

	protected:
		void onBuild() override
		{
			setInteractive(false);
			setStyleTag("Modal");
			setFocusScope(true);
			Style base;
			base.background = Colors::Black.withAlpha(0.6f);
			base.opacity = 0.0f;
			setInlineBase(base);
			content = VStack()->cls("modal-content");
			addChild(content);

			auto intro = std::make_shared<UIAnimation>(0.4f);
			intro->addTrack<Value>(VH(100.0f), Value(0.0f), [](Layout *l, Value v)
								   { l->getInlineBase().translateY = v; }, TransitionFunction::EaseOutBack);
			content->addAnimation("Intro", intro);
		}

		std::shared_ptr<Layout> content;
	};

	// ---------- ScrollView ----------
	class ScrollView : public TLayout<ScrollView>
	{
	public:
		ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t)
		{
			setInteractive(true);
			setBlocksRaycast(true);
			setStyleTag("ScrollView");
			inlineBase.overflow = Overflow::Scroll;
			pendingTransition = true;
		}

		// --- API di scroll programmatico ---
		void scrollTo(float x, float y)
		{
			scrollX = std::max(0.0f, x);
			scrollY = std::max(0.0f, y);
			clampScroll();
			pendingTransition = true;
		}
		void scrollToY(float y) { scrollTo(scrollX, y); }
		void scrollToX(float x) { scrollTo(x, scrollY); }
		void scrollToTop() { scrollToY(0.0f); }
		void scrollToBottom() { scrollToY(maxScrollY); }
		void scrollToLeft() { scrollToX(0.0f); }
		void scrollToRight() { scrollToX(maxScrollX); }

		float getScrollX() const { return scrollX; }
		float getScrollY() const { return scrollY; }
		float getMaxScrollX() const { return maxScrollX; }
		float getMaxScrollY() const { return maxScrollY; }

		void arrange(Rect space) override
		{
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

			if (type == LayoutType::Vertical)
			{
				float contentH = scrollContentSize.y + pt + pb;
				offsetSpace.height = std::max(space.height, contentH);
			}
			else if (type == LayoutType::Horizontal)
			{
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
			onLayout();
		}

	protected:
		void onUpdate(float dt) override
		{
			auto &ctx = UIContext::get();

			// 1) Drag del thumb (con pointer capture) e click sul track
			if (!hasPointerCapture())
			{
				if (ctx.pointer.pressed && ctx.topmostConsumer == this && maxScrollY > 0.0f)
				{
					Rect thumb = getThumbRect();
					if (thumb.contains(ctx.pointer.pos))
					{
						// Inizia drag del thumb con capture
						capturePointer();
						dragStartMouseY = ctx.pointer.pos.y;
						dragStartScrollY = scrollY;
						velocityX = velocityY = 0.0f;
					}
					else
					{
						// Click sul track (sfondo): salto di pagina
						float ratio = (ctx.pointer.pos.y - rect.y) / rect.height;
						scrollY = std::clamp(ratio * scrollContentSize.y - rect.height * 0.5f,
											 0.0f, maxScrollY);
						pendingTransition = true;
					}
				}
			}
			else if (ctx.pointer.down)
			{
				// Continuo il drag anche se il cursore è uscito dal widget
				float dy = ctx.pointer.pos.y - dragStartMouseY;
				float ratio = (rect.height > 0.0f) ? (scrollContentSize.y / rect.height) : 1.0f;
				scrollY = std::clamp(dragStartScrollY + dy * ratio, 0.0f, maxScrollY);
				pendingTransition = true;
			}

			// 2) Wheel: solo se il widget è attivo e il mouse è dentro.
			// Un widget disabilitato o non interattivo non consuma la wheel.
			if (!ctx.wheelConsumedThisFrame && isInteractive &&
				!hasPointerCapture() && rect.contains(ctx.pointer.pos))
			{
				if (ctx.pointer.wheelY != 0.0f)
				{
					if (IsShiftHeld())
						velocityX -= ctx.pointer.wheelY * WHEEL_IMPULSE;
					else
						velocityY -= ctx.pointer.wheelY * WHEEL_IMPULSE;
					ctx.wheelConsumedThisFrame = true;
					pendingTransition = true;
				}
			}

			// 3) Momentum
			if (std::abs(velocityX) > VELOCITY_MIN)
			{
				scrollX += velocityX * dt;
				velocityX *= std::pow(DECAY, dt * 60.0f);
			}
			else
				velocityX = 0.0f;

			if (std::abs(velocityY) > VELOCITY_MIN)
			{
				scrollY += velocityY * dt;
				velocityY *= std::pow(DECAY, dt * 60.0f);
			}
			else
				velocityY = 0.0f;

			clampScroll();
		}

		void draw(float parentOpacity) override
		{
			Layout::draw(parentOpacity);

			// Ridisegna la scrollbar DOPO i figli, così sta sopra.
			auto r = UIContext::get().renderer;
			if (!r || maxScrollY <= 0.0f || rect.height <= 10.0f)
				return;

			const float MARGIN = 4.0f;
			const float TRACK_W = 6.0f;
			float op = currentStyle.opacity * parentOpacity;
			if (op <= 0.001f)
				return;

			// Clip sul rect dello ScrollView per non sbordare
			r->pushClip(rect);

			Rect track = {
				rect.x + rect.width - TRACK_W - MARGIN,
				rect.y + MARGIN,
				TRACK_W,
				rect.height - MARGIN * 2};
			r->fillRoundedRect(track, TRACK_W * 0.5f, Color{40, 40, 40, 180}.withAlpha(op));

			Rect thumb = getThumbRect();
			Color thumbColor = hasPointerCapture()
								   ? Color{180, 180, 180, 255}
								   : Color{130, 130, 130, 220};
			r->fillRoundedRect(thumb, TRACK_W * 0.5f, thumbColor.withAlpha(op));

			r->popClip();
		}

	private:
		float scrollX{0.0f};
		float scrollY{0.0f};
		float maxScrollX{0.0f};
		float maxScrollY{0.0f};

		float velocityX{0.0f};
		float velocityY{0.0f};

		static constexpr float WHEEL_IMPULSE = 400.0f;
		static constexpr float DECAY = 0.90f;
		static constexpr float VELOCITY_MIN = 5.0f;

		float dragStartMouseY{0.0f};
		float dragStartScrollY{0.0f};

		void clampScroll()
		{
			scrollX = std::clamp(scrollX, 0.0f, maxScrollX);
			scrollY = std::clamp(scrollY, 0.0f, maxScrollY);
		}

		Rect getThumbRect() const
		{
			const float TRACK_W = 6.0f;
			const float MARGIN = 4.0f;

			float trackH = std::max(0.0f, rect.height - MARGIN * 2.0f); // <-- max
			float contentH = scrollContentSize.y;
			float viewportRatio = (contentH > 0.0f) ? (rect.height / contentH) : 1.0f;
			float thumbH = std::min(trackH, std::max(30.0f, trackH * viewportRatio)); // <-- min
			float scrollRatio = (maxScrollY > 0.0f) ? (scrollY / maxScrollY) : 0.0f;
			float available = std::max(0.0f, trackH - thumbH); // <-- max
			float thumbY = rect.y + MARGIN + std::clamp(scrollRatio * available, 0.0f, available);

			return {rect.x + rect.width - TRACK_W - MARGIN, thumbY, TRACK_W, thumbH};
		}

		static bool IsShiftHeld() { return UIContext::get().shiftHeld; }
	};

	// ---------- Checkbox ----------
	class Checkbox : public TLayout<Checkbox>
	{
	public:
		Checkbox(bool state = false) : TLayout<Checkbox>(LayoutType::Stack), isChecked(state)
		{
			setInteractive(true);
			setFocusable(true);
			setKeyboardActivates(true);
			setStyleTag("Checkbox");

			getInlineBase().background = Colors::DarkGray;
			getInlineBase().radius = Px(6.0f);

			onClick = [this]()
			{
				isChecked = !isChecked;
				if (onToggle)
					onToggle(isChecked);
			};
		}

		std::function<void(bool)> onToggle = nullptr;
		bool isChecked;

	protected:
		Vec2 computeIntrinsicSize(float, float) override
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return {fs * 1.4f, fs * 1.4f};
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			auto r = UIContext::get().renderer;

			Style boxStyle = partStyle("box");
			Style markStyle = partStyle("mark");

			// Box
			Color box = isChecked ? Colors::Green : style.background;
			if (!isEnabled)
				box = Color{70, 70, 75, 255};
			if (boxStyle.background.is_set)
				box = boxStyle.background.value;

			float maxR = std::min(rect.width, rect.height) * 0.5f;
			float rPx = boxStyle.radius.is_set
							? std::clamp(boxStyle.radius.value.resolve(maxR * 2.0f), 0.0f, maxR)
							: std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);

			r->fillRoundedRect(rect, rPx, box.withAlpha(op));

			// Mark
			if (isChecked)
			{
				Color mark = Colors::White;
				if (!isEnabled)
					mark = Color{150, 150, 155, 255};
				if (markStyle.color.is_set)
					mark = markStyle.color.value;

				float inner = rect.height * 0.35f;
				Rect m = {
					rect.center().x - inner * 0.5f,
					rect.center().y - inner * 0.5f,
					inner, inner};
				r->fillRoundedRect(m, 3.0f, mark.withAlpha(op));
			}
		}
	};

	// ---------- ProgressBar ----------
	class ProgressBar : public TLayout<ProgressBar>
	{
	public:
		ProgressBar(float v = 0.0f) : TLayout<ProgressBar>(LayoutType::Stack), value(std::clamp(v, 0.0f, 1.0f))
		{
			setInteractive(false);
			setStyleTag("ProgressBar");
			getInlineBase().background = Colors::DarkGray;
			getInlineBase().radius = Px(4.0f);
		}

		void setValue(float v)
		{
			value = std::clamp(v, 0.0f, 1.0f);
			pendingTransition = true;
		}
		float getValue() const { return value; }

	protected:
		Vec2 computeIntrinsicSize(float, float) override
		{
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
			return {120.0f, std::max(16.0f, fs * 0.8f)};
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			auto r = UIContext::get().renderer;

			Style trackStyle = partStyle("track");
			Style fillStyle = partStyle("fill");

			float maxR = std::min(rect.width, rect.height) * 0.5f;

			// Track
			Color trackColor = style.background;
			if (trackStyle.background.is_set)
				trackColor = trackStyle.background.value;

			float rPx = trackStyle.radius.is_set
							? std::clamp(trackStyle.radius.value.resolve(maxR * 2.0f), 0.0f, maxR)
							: std::clamp(style.radius.resolve(maxR * 2.0f), 0.0f, maxR);

			r->fillRoundedRect(rect, rPx, trackColor.withAlpha(op));

			// Fill
			float w = rect.width * value;
			if (w > 0.5f)
			{
				Color fillColor = isEnabled ? style.color : Color{90, 90, 95, 255};
				if (fillStyle.color.is_set)
					fillColor = fillStyle.color.value;

				Rect fill = {rect.x, rect.y, w, rect.height};
				r->fillRoundedRect(fill, rPx, fillColor.withAlpha(op));
			}
		}

	private:
		float value;
	};

	// ---------- TextInput ----------
	class TextInput : public TLayout<TextInput>
	{
	public:
		TextInput(std::string initial = "")
			: TLayout<TextInput>(LayoutType::Stack), text(std::move(initial))
		{
			setInteractive(true);
			setFocusable(true);
			setStyleTag("TextInput");
			cursorPos = (int)text.size();

			onClick = [this]()
			{
				if (!isEnabled)
					return;
				UIContext::get().requestFocus(shared_from_this());
			};

			// Default inline: solo comportamento di base. Font/colori stanno nel CSS.
			inlineBase.background = Color{30, 30, 36, 255};
			inlineBase.color = Colors::White;
			inlineBase.radius = Px(6.0f);
			inlineBase.padding = Spacing(VH(1.2f), VW(1.5f));
			inlineBase.borderColor = Color{60, 60, 70, 255};
			inlineBase.borderWidth = Px(1.0f);
			pendingTransition = true;
		}

		void setText(std::string t)
		{
			text = std::move(t);
			cursorPos = std::min(cursorPos, (int)text.size());
			text_dirty = true;
			pendingTransition = true;
		}
		const std::string &getText() const { return text; }
		void clear() { setText(""); }

		// Font di fallback se il CSS non specifica `font:`.
		void setFont(FontHandle f)
		{
			defaultFont = f;
			pendingTransition = true;
		}

		std::function<void(const std::string &)> onTextChanged;
		std::function<void(const std::string &)> onSubmit;

	protected:
		Vec2 computeIntrinsicSize(float, float) override
		{
			FontHandle f = resolveFont(currentStyle, defaultFont);
			float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);

			// Larghezza minima basata su una misura reale del font risolto:
			// larghezza di un carattere tipico * 12.
			float charW = 0.0f;
			if (auto r = UIContext::get().renderer)
			{
				charW = r->measureText(f, "M", fs, 0.0f).x;
			}
			if (charW <= 0.0f)
				charW = fs * 0.6f; // fallback prudente

			return {std::max(120.0f, charW * 12.0f), fs * 1.6f};
		}

		void onEnabledChanged(bool nowEnabled) override
		{
			if (!nowEnabled)
			{
				hasFocusCache = false;
				repeatStates.clear();
				cursorBlink = 0.0f;
			}
		}

		void onUpdate(float dt) override
		{
			hasFocusCache = UIContext::get().hasFocus(this);
			cursorBlink += dt;
			if (cursorBlink > 1.0f)
				cursorBlink -= 1.0f;

			if (!hasFocusCache)
			{
				repeatStates.clear();
				return;
			}

			auto &ev = UIContext::get().inputEvents;
			bool changed = false;

			// --- Caratteri digitati ---
			for (int c : ev.chars)
			{
				if (c < 32 || c > 126)
					continue;
				text.insert(text.begin() + cursorPos, (char)c);
				cursorPos++;
				changed = true;
			}

			// --- Azioni discrete per un tasto di editing ---
			auto doAction = [&](int key)
			{
				switch (key)
				{
				case Key::Backspace:
					if (cursorPos > 0)
					{
						text.erase(text.begin() + cursorPos - 1);
						cursorPos--;
						changed = true;
					}
					break;
				case Key::Delete:
					if (cursorPos < (int)text.size())
					{
						text.erase(text.begin() + cursorPos);
						changed = true;
					}
					break;
				case Key::Left:
					if (cursorPos > 0)
					{
						cursorPos--;
						changed = true;
					}
					break;
				case Key::Right:
					if (cursorPos < (int)text.size())
					{
						cursorPos++;
						changed = true;
					}
					break;
				}
			};

			// --- Tasti one-shot ---
			for (int k : ev.keys)
			{
				switch (k)
				{
				case Key::Enter:
					if (onSubmit)
						onSubmit(text);
					UIContext::get().releaseFocus();
					hasFocusCache = false;
					repeatStates.clear();
					return;
				case Key::Escape:
					UIContext::get().releaseFocus();
					hasFocusCache = false;
					repeatStates.clear();
					return;
				case Key::Home:
					cursorPos = 0;
					changed = true;
					break;
				case Key::End:
					cursorPos = (int)text.size();
					changed = true;
					break;
				default:
					break;
				}
			}

			// --- Tasti con auto-repeat ---
			constexpr float REPEAT_DELAY = 0.40f;
			constexpr float REPEAT_INTERVAL = 0.04f;

			auto handleRepeat = [&](int key)
			{
				bool isHeld = std::find(ev.held.begin(), ev.held.end(), key) != ev.held.end();
				auto it = repeatStates.find(key);

				if (!isHeld)
				{
					if (it != repeatStates.end())
						repeatStates.erase(it);
					return;
				}
				if (it == repeatStates.end())
				{
					doAction(key);
					RepeatState st;
					st.holdTime = 0.0f;
					st.nextFireTime = REPEAT_DELAY;
					repeatStates[key] = st;
				}
				else
				{
					it->second.holdTime += dt;
					while (it->second.holdTime >= it->second.nextFireTime)
					{
						doAction(key);
						it->second.nextFireTime += REPEAT_INTERVAL;
					}
				}
			};

			handleRepeat(Key::Backspace);
			handleRepeat(Key::Delete);
			handleRepeat(Key::Left);
			handleRepeat(Key::Right);

			if (changed)
			{
				if (onTextChanged)
					onTextChanged(text);
				pendingTransition = true;
			}

			// --- Auto-scroll orizzontale, con il font risolto dallo stile ---
			{
				auto r = UIContext::get().renderer;
				if (!r)
					return;

				FontHandle f = resolveFont(currentStyle, defaultFont);
				float fs = currentStyle.fontSize.resolve(Metrics::viewport.y);
				float sp = currentStyle.letterSpacing.resolve(Metrics::viewport.x);
				float pl = currentStyle.padding.left.resolve(rect.width);
				float availW = std::max(0.0f, rect.width - pl * 2.0f);

				std::string prefix = text.substr(0, cursorPos);
				Vec2 prefixSize = r->measureText(f, prefix, fs, sp);
				Vec2 fullSize = r->measureText(f, text, fs, sp);

				if (prefixSize.x - scrollX > availW)
					scrollX = prefixSize.x - availW;
				if (prefixSize.x - scrollX < 0.0f)
					scrollX = prefixSize.x;

				float maxScroll = std::max(0.0f, fullSize.x - availW);
				scrollX = std::clamp(scrollX, 0.0f, maxScroll);
			}
		}

		void renderContent(float op, const ComputedStyle &style) override
		{
			auto r = UIContext::get().renderer;
			FontHandle f = resolveFont(style, defaultFont);
			float fs = style.fontSize.resolve(Metrics::viewport.y);
			float sp = style.letterSpacing.resolve(Metrics::viewport.x);
			float pl = style.padding.left.resolve(rect.width);

			r->pushClip(rect);

			Vec2 pos = {rect.x + pl - scrollX, rect.y + (rect.height - fs) * 0.5f - fs * 0.1f};

			Color textColor = isEnabled ? style.color : Color{120, 120, 125, 255};
			r->drawText(f, text, pos, fs, sp, textColor.withAlpha(op));

			if (hasFocusCache && isEnabled && cursorBlink < 0.5f)
			{
				Style cursorStyle = partStyle("cursor");
				Color cursorColor = cursorStyle.color.is_set
										? cursorStyle.color.value
										: textColor;

				// Larghezza: usa borderWidth del part se settato, altrimenti 2px
				float cursorW = cursorStyle.borderWidth.is_set
									? cursorStyle.borderWidth.value.resolve(fs)
									: 2.0f;

				std::string prefix = text.substr(0, cursorPos);
				Vec2 prefixSize = r->measureText(f, prefix, fs, sp);
				float cx = pos.x + prefixSize.x;
				float cy = pos.y;
				float ch = fs;
				r->fillRect({cx, cy, cursorW, ch}, cursorColor.withAlpha(op));
			}

			r->popClip();
		}

	private:
		std::string text;
		int cursorPos{0};
		bool text_dirty{true};
		bool hasFocusCache{false};
		float cursorBlink{0.0f};
		float scrollX{0.0f};

		FontHandle defaultFont{0};

		struct RepeatState
		{
			float holdTime{0.0f};
			float nextFireTime{0.0f};
		};
		std::unordered_map<int, RepeatState> repeatStates;
	};

	// ---------- Dropdown ----------
	class Dropdown : public TLayout<Dropdown>
	{
	public:
		Dropdown(std::vector<std::string> options, int selected = 0)
			: TLayout<Dropdown>(LayoutType::Vertical),
			  options(std::move(options)), selected(selected) {}

		int getSelected() const { return selected; }
		void setSelected(int idx)
		{
			if (idx < 0 || idx >= (int)options.size())
				return;
			selected = idx;
			refreshButtonText();
			if (onChange)
				onChange(selected, options[selected]);
			pendingTransition = true;
		}

		bool getOpen() const { return isOpen; }
		void setOpen(bool o)
		{
			if (isOpen == o)
				return;
			isOpen = o;
			applyOpenState();
		}

		std::function<void(int, const std::string &)> onChange;

	protected:
		void onEnabledChanged(bool nowEnabled) override
		{
			if (!nowEnabled && isOpen)
			{
				isOpen = false;
				applyOpenState();
			}
		}

		void onUpdate(float) override
		{
			if (!isOpen)
				return;
			auto &ctx = UIContext::get();
			if (!ctx.pointer.pressed)
				return;
			if (!rect.contains(ctx.pointer.pos) &&
				!listContainer->getRect().contains(ctx.pointer.pos))
			{
				setOpen(false);
			}
		}

		void onBuild() override
		{
			setStyleTag("Dropdown");
			setInteractive(true);
			setFocusable(true);
			pendingTransition = true;

			getInlineBase().position = Position::Relative;
			getInlineBase().zIndex = ZIndex(10);

			button = Btn("", [this]()
						 { toggleOpen(); });
			button->getInlineBase().width = Percent(100);
			button->getInlineBase().height = VH(3.0f);
			button->getInlineBase().background = Color{40, 40, 45, 255};
			button->getInlineBase().radius = Px(6.0f);
			button->getInlineBase().borderColor = Color{70, 70, 80, 255};
			button->getInlineBase().borderWidth = Px(1.0f);
			button->getInlineBase().itemsH = Align::Center;
			button->getInlineBase().itemsV = Align::Center;
			addChild(button);
			listContainer = std::make_shared<ScrollView>();
			listContainer->getInlineBase().position = Position::Absolute;
			listContainer->setPortal(true);
			listContainer->getInlineBase().width = VW(15.0f);
			listContainer->getInlineBase().height = VH(25.0f);
			listContainer->getInlineBase().background = Color{30, 30, 36, 255};
			listContainer->getInlineBase().borderColor = Color{60, 60, 70, 255};
			listContainer->getInlineBase().borderWidth = Px(1.0f);
			listContainer->getInlineBase().itemsH = Align::Stretch;
			for (size_t i = 0; i < options.size(); ++i)
			{
				int idx = (int)i;
				auto item = Label(options[i]);
				item->getInlineBase().padding = Spacing(VH(1.0f), VW(1.0f));
				item->getInlineBase().itemsV = Align::Center;
				item->getInlineBase().background = Color{40, 40, 50, 255};
				item->getInlineBase().color = Colors::White;
				item->setInteractive(true);
				item->setFocusable(true);
				item->onClick = [this, idx]()
				{
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

		void arrange(Rect space) override
		{
			TLayout<Dropdown>::arrange(space);

			if (button && listContainer)
			{
				Rect btnRect = button->getRect();
				listContainer->arrange({btnRect.x,
										btnRect.y + btnRect.height + 4.0f,
										btnRect.width,
										listContainer->getMeasuredSize().y});
			}
		}

	private:
		std::vector<std::string> options;
		int selected{0};
		bool isOpen{false};
		std::shared_ptr<Button> button;
		std::shared_ptr<ScrollView> listContainer;

		void toggleOpen() { setOpen(!isOpen); }

		void applyOpenState()
		{
			listContainer->setEnabled(isOpen);
			listContainer->setInteractive(isOpen);
			listContainer->getInlineBase().opacity = isOpen ? 1.0f : 0.0f;
			pendingTransition = true;
		}

		void refreshButtonText()
		{
			if (selected < 0 || selected >= (int)options.size())
				return;
			if (button->children.empty())
				return;
			auto t = std::dynamic_pointer_cast<Text>(button->children.front());
			if (t)
				t->setText(options[selected]);
		}
	};

	// ---------- Tooltip ----------
	class Tooltip : public TLayout<Tooltip, Panel>
	{
	public:
		Tooltip(std::string text)
			: TLayout<Tooltip, Panel>(), text_(std::move(text)) {}

		// Aggancia un tooltip a un widget. showDelay in secondi (default 0.4).
		static std::shared_ptr<Tooltip> attach(std::shared_ptr<Layout> owner,
											   std::string text,
											   float showDelay = 0.4f)
		{
			auto tip = Tooltip::create(std::move(text));
			tip->anchor_ = owner.get();
			tip->showDelay_ = showDelay;

			auto prevEnter = owner->onHoverEnter;
			auto prevExit = owner->onHoverExit;

			owner->onHoverEnter = [tip, prevEnter]()
			{
				if (prevEnter)
					prevEnter();
				tip->show();
			};
			owner->onHoverExit = [tip, prevExit]()
			{
				if (prevExit)
					prevExit();
				tip->hide();
			};

			owner->addChild(tip);
			return tip;
		}

		void setAnchor(Layout *a) { anchor_ = a; }
		void setShowDelay(float s) { showDelay_ = s; }

		void show()
		{
			hovering_ = true;
			timer_ = 0.0f;
		}
		void hide()
		{
			hovering_ = false;
			visible_ = false;
			timer_ = 0.0f;
			setTargetOpacity(0.0f);
		}

		// Non usiamo lo space del parent: ci arrangiamo da soli in onPreUpdate
		// basandoci sull'anchor, che è aggiornato solo DOPO il nostro arrange.
		void arrange(Rect) override {}

	protected:
		void onBuild() override
		{
			setStyleTag("Tooltip");
			setPortal(true); // scappa al clipping dei parent
			setInteractive(false);
			setBlocksRaycast(false);
			inlineBase.position = Position::Absolute;
			inlineBase.opacity = 0.0f;

			auto label = Label(text_);
			label->getInlineBase().fontSize = VH(2.4f);
			addChild(label);
		}

		void onUpdate(float dt) override
		{
			// Delay + fade-in
			if (hovering_ && !visible_)
			{
				timer_ += dt;
				if (timer_ >= showDelay_)
				{
					visible_ = true;
					setTargetOpacity(1.0f);
				}
			}

			// Posizionamento rispetto all'anchor
			if (anchor_)
			{
				Rect a = anchor_->getRect();
				Vec2 ms = getMeasuredSize();
				if (ms.x > 0.0f && ms.y > 0.0f)
				{
					rect = {
						a.x + a.width * 0.5f - ms.x * 0.5f,
						a.y - ms.y - 6.0f,
						ms.x,
						ms.y};
					arrangeInto(rect);
				}
			}
		}

	private:
		std::string text_;
		Layout *anchor_{nullptr};
		float showDelay_{0.4f};
		float timer_{0.0f};
		bool hovering_{false};
		bool visible_{false};

		void setTargetOpacity(float op)
		{
			float old = inlineBase.opacity.get_or(0.0f);
			if (inlineBase.opacity.is_set && old == op)
				return;
			inlineBase.opacity = op;
			beginTransition();
		}
	};

	// ---------- Popup ----------
	//
	// Pannello fluttuante ancorato a un widget o a una posizione. Usa portal
	// per scappare al clipping, si chiude su click-outside e Escape.
	//
	// Uso tipico (context menu):
	//   popup->openBelow(btn);   // apre sotto btn
	//   popup->openAt({x, y});   // apre a coordinate schermo
	//
	class Popup : public TLayout<Popup, Panel>
	{
	public:
		Popup() : TLayout<Popup, Panel>() {}

		// Imposta il contenuto (di solito un VStack). Va chiamato una sola volta
		// in fase di costruzione.
		void setContent(std::shared_ptr<Layout> content)
		{
			content_ = std::move(content);
			if (content_)
				addChild(content_);
		}

		// Apre sotto l'anchor
		void openBelow(std::shared_ptr<Layout> anchor)
		{
			anchor_ = anchor.get();
			pos_ = {0.0f, 0.0f};
			openInternal();
		}

		// Apre sopra l'anchor
		void openAbove(std::shared_ptr<Layout> anchor)
		{
			anchor_ = anchor.get();
			above_ = true;
			openInternal();
		}

		// Apre a coordinate schermo
		void openAt(Vec2 screenPos)
		{
			anchor_ = nullptr;
			pos_ = screenPos;
			above_ = false;
			openInternal();
		}

		void close()
		{
			if (!isOpen_)
				return;
			isOpen_ = false;
			anchor_ = nullptr;
			inlineBase.opacity = 0.0f;
			beginTransition();
			setEnabled(false);
			if (onClose)
				onClose();
		}

		bool isOpen() const { return isOpen_; }

		// Config
		bool closeOnClickOutside{true};
		bool closeOnEscape{true};
		float offsetBelow{4.0f}; // distanza dall'anchor
		std::function<void()> onClose;

		void arrange(Rect) override { /* no-op */ }

	protected:
		void onBuild() override
		{
			setStyleTag("Popup");
			setPortal(true);
			setInteractive(true);
			setFocusable(true);
			setBlocksRaycast(true);
			inlineBase.position = Position::Absolute;
			inlineBase.opacity = 0.0f;
			isOpen_ = false;
			setEnabled(false); // chiuso all'inizio
		}

		void onUpdate(float) override
		{
			if (!isOpen_)
				return;
			auto &ctx = UIContext::get();

			// Calcola posizione
			Vec2 size = computePopupSize();
			float x = pos_.x, y = pos_.y;
			if (anchor_)
			{
				Rect a = anchor_->getRect();
				x = a.x;

				if (above_)
				{
					y = a.y - size.y - offsetBelow;
					// Flip sotto se esce dal bordo superiore
					if (y < 0.0f)
						y = a.y + a.height + offsetBelow;
				}
				else
				{
					y = a.y + a.height + offsetBelow;
					// Flip sopra se esce dal bordo inferiore
					if (y + size.y > Metrics::viewport.y)
						y = a.y - size.y - offsetBelow;
				}
			}

			if (x < 0.0f)
				x = 0.0f;
			if (y < 0.0f)
				y = 0.0f;
			if (x + size.x > Metrics::viewport.x)
				x = Metrics::viewport.x - size.x;
			if (y + size.y > Metrics::viewport.y)
				y = Metrics::viewport.y - size.y;

			rect = {x, y, size.x, size.y};
			arrangeInto(rect);

			// Click outside → close
			if (closeOnClickOutside && ctx.pointer.pressed)
			{
				if (!rect.contains(ctx.pointer.pos))
					close();
			}

			// Click destro outside → close
			if (closeOnClickOutside && ctx.pointer.rightPressed)
			{
				if (!rect.contains(ctx.pointer.pos))
					close();
			}

			// Escape → close
			if (closeOnEscape)
			{
				for (int k : ctx.inputEvents.keys)
				{
					if (k == Key::Escape)
					{
						close();
						return;
					}
				}
			}
		}

	private:
		std::shared_ptr<Layout> content_;
		Layout *anchor_{nullptr};
		Vec2 pos_{0.0f, 0.0f};
		bool isOpen_{false};
		bool above_{false};

		void openInternal()
		{
			isOpen_ = true;
			above_ = (above_ && anchor_); // rispetta la modalità, ma solo con anchor
			setEnabled(true);
			inlineBase.opacity = 1.0f;
			beginTransition();
			// Forza un ricalcolo size la prossima volta che gira
			if (content_)
				content_->beginTransition();
			UIContext::get().requestFocus(shared_from_this());
		}

		Vec2 computePopupSize()
		{
			if (!content_)
				return {100.0f, 40.0f};

			// Il contenuto è già stato misurato dal parent in fase di measure,
			// ma se è cambiato (es. testo dinamico) potrebbe essere stale.
			// Per ora prendiamo la measuredSize del contenuto, che il framework
			// ha già calcolato.
			Vec2 s = content_->getMeasuredSize();
			if (s.x <= 0.0f && s.y <= 0.0f)
			{
				// fallback: forziamo una measure con spazio ampio
				content_->measure(1000.0f, 1000.0f);
				s = content_->getMeasuredSize();
			}

			// Aggiungi padding/bordo del popup
			float pl = currentStyle.padding.left.resolve(0.0f);
			float pr = currentStyle.padding.right.resolve(0.0f);
			float pt = currentStyle.padding.top.resolve(0.0f);
			float pb = currentStyle.padding.bottom.resolve(0.0f);
			return {s.x + pl + pr, s.y + pt + pb};
		}
	};

	// Helper per costruire rapidamente un popup con voci di menu.
	// Ogni voce è un Button "trasparente" con classe "popup-item".
	inline std::shared_ptr<Popup> ContextMenu(std::vector<std::pair<std::string, std::function<void()>>> items)
	{
		auto popup = Popup::create();
		auto stack = VStack();
		stack->getInlineBase().gap = Px(2.0f);

		std::weak_ptr<Popup> weakPopup = popup;
		for (auto &[label, cb] : items)
		{
			auto item = Btn(label, [weakPopup, cb]()
							{
				if (cb) cb();
				if (auto p = weakPopup.lock()) p->close(); });
			item->cls("popup-item");
			item->getInlineBase().width = Percent(100);
			item->getInlineBase().justify = Justify::Start;
			stack->addChild(item);
		}
		popup->setContent(stack);
		return popup;
	}
}