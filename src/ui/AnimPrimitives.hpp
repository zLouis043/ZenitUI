#pragma once

#include "Common.hpp"

#include "CoreTypes.hpp"
#include "Style.hpp"

namespace ZenitUI {

	class Layout;

	// ===================== UIAnimation imperativa (invariata) =====================
	class UIAnimation {
	public:
		struct TrackBase {
			virtual ~TrackBase() = default;
			virtual void apply(float raw_t, Layout* target) = 0;
		};

		template <typename T>
		struct Track : public TrackBase {
			T start_val, end_val;
			std::function<void(Layout*, T)> setter;
			TransitionFunction transition;

			Track(T s, T e, std::function<void(Layout*, T)> setter, TransitionFunction tr)
				: start_val(s), end_val(e), setter(std::move(setter)), transition(tr) {}

			void apply(float raw_t, Layout* target) override {
				float ratio = getRatio(raw_t, transition);
				setter(target, lerpProp(start_val, end_val, ratio));
			}
		};

		UIAnimation(float duration = 1.0f, float delay = 0.0f)
			: duration(duration), delay(delay) {}

		template <typename T, typename Callable>
		void addTrack(T start, T end, Callable setter, TransitionFunction tr = TransitionFunction::Linear) {
			tracks.push_back(std::make_unique<Track<T>>(start, end, std::function<void(Layout*, T)>(setter), tr));
		}

		float duration;
		float delay{ 0.0f };
		bool blocksInput{ true };
		std::vector<std::unique_ptr<TrackBase>> tracks;
		std::function<void()> onFinished;
	};

	struct AnimState {
		std::shared_ptr<UIAnimation> anim;
		float elapsed{ 0.0f };
		float delayElapsed{ 0.0f };
		bool playing{ false };
		bool reverse{ false };
	};

	// ===================== Keyframes (definizione pura, senza timing) =====================
	struct Keyframe {
		float t{ 0.0f };
		Style delta;
	};

	struct KeyframeAnimation {
		std::string name;
		std::vector<Keyframe> keyframes;
	};

	// ===================== Istanza attiva (timing incluso) =====================
	struct ActiveCssAnimation {
		std::string name;
		float elapsed{ 0.0f };
		double startTime{ 0.0 }; 
		float duration{ 1.0f };
		float delay{ 0.0f };
		int iterations{ 1 };          // -1 = infinite
		bool alternate{ false };
		bool fillForwards{ false };
		bool finished{ false };
		TransitionFunction ease{ TransitionFunction::Linear };
	};

	// ===================== Evaluators =====================
	inline Style evaluateKeyframes(const KeyframeAnimation& anim, float localT, TransitionFunction ease) {
		Style out;
		if (anim.keyframes.empty()) return out;
		if (anim.keyframes.size() == 1) return anim.keyframes[0].delta;

		const Keyframe* k0 = &anim.keyframes.front();
		const Keyframe* k1 = &anim.keyframes.back();
		for (size_t i = 0; i + 1 < anim.keyframes.size(); ++i) {
			if (localT >= anim.keyframes[i].t && localT <= anim.keyframes[i + 1].t) {
				k0 = &anim.keyframes[i];
				k1 = &anim.keyframes[i + 1];
				break;
			}
		}

		float span = k1->t - k0->t;
		float u = (span > 0.0f) ? (localT - k0->t) / span : 0.0f;
		u = getRatio(std::clamp(u, 0.0f, 1.0f), ease);

#define X(T, name, def) \
		if (k0->delta.name.is_set && k1->delta.name.is_set) \
			out.name = lerpProp(k0->delta.name.value, k1->delta.name.value, u); \
		else if (k0->delta.name.is_set) out.name = k0->delta.name.value; \
		else if (k1->delta.name.is_set) out.name = k1->delta.name.value;
		BUBBLE_STYLE_PROPS(X)
#undef X

		return out;
	}

	// Campiona un'istanza attiva: ritorna localT ∈ [0,1] e setta finished.
	inline float sampleActive(const ActiveCssAnimation& a, bool& finished) {
		finished = false;
		float t = a.elapsed - a.delay;
		if (t < 0.0f) return 0.0f;
		if (a.duration <= 0.0f) { finished = true; return 1.0f; }

		float total = (a.iterations < 0) ? -1.0f : (a.duration * (float)a.iterations);
		if (a.iterations >= 0 && t >= total) {
			t = total;
			finished = true;
		}
		float iter = std::floor(t / a.duration);

		// Quando finished e t è esattamente su total, iter punta alla prossima
		// iterazione inesistente. Lo clampiamo all'ultima valida.
		if (a.iterations > 0 && finished && iter > 0.0f) {
			iter = std::min(iter, (float)(a.iterations - 1));
		}

		float local = (t - iter * a.duration) / a.duration;
		if (a.alternate && ((int)iter % 2 == 1)) local = 1.0f - local;
		return std::clamp(local, 0.0f, 1.0f);
	}

} // namespace ZenitUI