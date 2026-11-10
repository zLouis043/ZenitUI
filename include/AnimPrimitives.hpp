#pragma once

/**
 * \file AnimPrimitives.hpp
 * \brief Low-level animation types: imperative tracks, CSS keyframes, and
 *        the per-instance active animation state.
 *
 * Two animation systems coexist:
 *
 *  - **Imperative** (`UIAnimation`, `AnimState`): driven by code, one track
 *    per property, applied per-frame by `AnimationPlayer::tickImperative`.
 *    Used by `Modal::show`, `Popup::openInternal`, and any component that
 *    needs runtime-controlled tweens.
 *
 *  - **Declarative CSS** (`KeyframeAnimation`, `ActiveCssAnimation`): driven
 *    by the `animations` field of a `Style`, sampled from the theme's
 *    `@keyframes` registry, and overlaid onto the resolved style at draw time
 *    by `Anim::overlayCssStyle` / `Anim::overlayCssComputed`.
 *
 * The two systems share easing (`Easing.hpp`) and style interpolation
 * (`Style.hpp::lerpProp` and friends) but are otherwise independent.
 */

#include "Common.hpp"

#include "CoreTypes.hpp"
#include "Style.hpp"

namespace ZenitUI
{

	class Layout;

	/**
	 * \class UIAnimation
	 * \brief A time-parameterised sequence of per-property tracks.
	 *
	 * Each track pairs a `start` and `end` value with a setter callback that
	 * writes the interpolated value into the target `Layout`. The setter is
	 * type-erased behind `TrackBase`, so `UIAnimation` can hold tracks of
	 * different value types (e.g. `float` opacity and `Value` translateY).
	 *
	 * Duration and delay are in seconds. `blocksInput`, if set, is reported
	 * back to `Layout::update` via `AnimationPlayer::tickImperative` and
	 * blocks input to the subtree for the animation's whole lifetime.
	 */
	class UIAnimation
	{
	public:
		/** \brief Type-erased track interface. */
		struct TrackBase
		{
			virtual ~TrackBase() = default;
			/** \brief Advance the track to normalized time \p raw_t. */
			virtual void apply(float raw_t, Layout *target) = 0;
		};

		/** \brief Concrete typed track: start/end pair plus a setter. */
		template <typename T>
		struct Track : public TrackBase
		{
			T start_val, end_val;
			std::function<void(Layout *, T)> setter;
			TransitionFunction transition;

			Track(T s, T e, std::function<void(Layout *, T)> setter, TransitionFunction tr)
				: start_val(s), end_val(e), setter(std::move(setter)), transition(tr) {}

			void apply(float raw_t, Layout *target) override
			{
				float ratio = getRatio(raw_t, transition);
				setter(target, lerpProp(start_val, end_val, ratio));
			}
		};

		UIAnimation(float duration = 1.0f, float delay = 0.0f)
			: duration(duration), delay(delay) {}

		/** \brief Append a typed track. */
		template <typename T, typename Callable>
		void addTrack(T start, T end, Callable setter, TransitionFunction tr = TransitionFunction::Linear)
		{
			tracks.push_back(std::make_unique<Track<T>>(start, end, std::function<void(Layout *, T)>(setter), tr));
		}

		float duration;
		float delay{0.0f};
		bool blocksInput{true};
		std::vector<std::unique_ptr<TrackBase>> tracks;
		std::function<void()> onFinished;
	};

	/** \brief Playback state for one `UIAnimation` on one node. */
	struct AnimState
	{
		std::shared_ptr<UIAnimation> anim;
		float elapsed{0.0f};
		float delayElapsed{0.0f};
		bool playing{false};
		bool reverse{false};
	};

	/** \brief One keyframe of a CSS animation: a normalized time and a sparse style delta. */
	struct Keyframe
	{
		float t{0.0f};
		Style delta;
	};

	/** \brief A named list of keyframes, keyed by `name` in `Theme::keyframes`. */
	struct KeyframeAnimation
	{
		std::string name;
		std::vector<Keyframe> keyframes;
	};

	/** \brief Playback state for one CSS animation on one node. */
	struct ActiveCssAnimation
	{
		std::string name;
		float elapsed{0.0f};
		double startTime{0.0};
		float duration{1.0f};
		float delay{0.0f};
		int iterations{1};
		bool alternate{false};
		bool fillForwards{false};
		bool finished{false};
		TransitionFunction ease{TransitionFunction::Linear};

		mutable float cachedSampleElapsed{-1.0f};
		mutable float cachedLocalT{0.0f};
		mutable bool cachedFinished{false};
	};

	/**
	 * \brief Sample the given CSS animation's local time without touching the cache.
	 *
	 * Combines `elapsed`, `delay`, `duration`, `iterations`, `alternate` into
	 * a normalized `[0, 1]` progress value. Also reports whether this is the
	 * final frame of the animation via \p finished.
	 *
	 * \param a        Active animation.
	 * \param[out] finished  Set to `true` on the last sample.
	 * \return Local time in `[0, 1]`.
	 */
	inline float sampleActive(const ActiveCssAnimation &a, bool &finished)
	{
		finished = false;
		float t = a.elapsed - a.delay;
		if (t < 0.0f)
			return 0.0f;
		if (a.duration <= 0.0f)
		{
			finished = true;
			return 1.0f;
		}

		float total = (a.iterations < 0) ? -1.0f : (a.duration * (float)a.iterations);
		if (a.iterations >= 0 && t >= total)
		{
			t = total;
			finished = true;
		}
		float iter = std::floor(t / a.duration);

		if (a.iterations > 0 && finished && iter > 0.0f)
		{
			iter = std::min(iter, (float)(a.iterations - 1));
		}

		float local = (t - iter * a.duration) / a.duration;
		if (a.alternate && ((int)iter % 2 == 1))
			local = 1.0f - local;
		return std::clamp(local, 0.0f, 1.0f);
	}

	/**
	 * \brief Cached wrapper around `sampleActive`.
	 */
	inline float sampleActiveCached(const ActiveCssAnimation &a, bool &finished)
	{
		if (a.cachedSampleElapsed == a.elapsed)
		{
			finished = a.cachedFinished;
			return a.cachedLocalT;
		}

		a.cachedLocalT = sampleActive(a, finished);
		a.cachedFinished = finished;
		a.cachedSampleElapsed = a.elapsed;
		return a.cachedLocalT;
	}

	/**
	 * \brief Interpolate a `KeyframeAnimation` at local time \p localT.
	 *
	 * Finds the two keyframes bracketing \p localT, computes the local
	 * sub-progress, applies easing, then lerps every property present in
	 * both bracketing keyframes. Properties present in only one side are
	 * taken from that side directly.
	 */
	inline Style evaluateKeyframes(const KeyframeAnimation &anim, float localT, TransitionFunction ease)
	{
		Style out;
		if (anim.keyframes.empty())
			return out;
		if (anim.keyframes.size() == 1)
			return anim.keyframes[0].delta;

		const Keyframe *k0 = &anim.keyframes.front();
		const Keyframe *k1 = &anim.keyframes.back();
		for (size_t i = 0; i + 1 < anim.keyframes.size(); ++i)
		{
			if (localT >= anim.keyframes[i].t && localT <= anim.keyframes[i + 1].t)
			{
				k0 = &anim.keyframes[i];
				k1 = &anim.keyframes[i + 1];
				break;
			}
		}

		float span = k1->t - k0->t;
		float u = (span > 0.0f) ? (localT - k0->t) / span : 0.0f;
		u = getRatio(std::clamp(u, 0.0f, 1.0f), ease);

#define X(T, name, def)                                                     \
	if (k0->delta.name.is_set && k1->delta.name.is_set)                     \
		out.name = lerpProp(k0->delta.name.value, k1->delta.name.value, u); \
	else if (k0->delta.name.is_set)                                         \
		out.name = k0->delta.name.value;                                    \
	else if (k1->delta.name.is_set)                                         \
		out.name = k1->delta.name.value;
		BUBBLE_STYLE_PROPS(X)
#undef X

		return out;
	}

} // namespace ZenitUI