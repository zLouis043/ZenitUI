#pragma once

/**
 * \file InputAccumulator.hpp
 * \brief Thread-safe event-to-polling accumulator for event-driven `IPlatform` backends.
 *
 * Event-driven windowing libraries (SDL3, GLFW, Win32 WndProc) deliver input
 * asynchronously via callbacks or an event loop, whereas `IPlatform` exposes a
 * per-frame polling API (`pointer()`, `pollInputEvents()`). Backends can embed
 * an `InputAccumulator` to guarantee ZenitUI's one-frame edge-trigger invariants
 * without rewriting queue and synchronization logic.
 */

#include "CoreTypes.hpp"
#include "Utf8.hpp"

#include <algorithm>
#include <array>
#include <mutex>
#include <string_view>
#include <vector>

namespace ZenitUI
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	/**
	 * \class InputAccumulator
	 * \ingroup grp_backends
	 * \brief Buffers asynchronous OS mouse/keyboard/text events and drains them
	 *        once per frame into `PointerState` and `InputEvents`.
	 */
	class InputAccumulator
	{
	public:
		using ButtonDownArray = std::array<bool, static_cast<size_t>(MouseButton::Count)>;

		/** \brief Record logical pointer movement. */
		void pushMouseMove(Vec2 logicalPos)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			mousePos_ = logicalPos;
		}

		/** \brief Record a mouse button transition (`down == true` sets `pressed`, `false` sets `released`). */
		void pushMouseButton(MouseButton btn, bool down)
		{
			const size_t idx = static_cast<size_t>(btn);
			if (idx >= static_cast<size_t>(MouseButton::Count))
				return;
			std::lock_guard<std::mutex> lock(mtx_);
			down_[idx] = down;
			if (down)
				pendingPressed_[idx] = true;
			else
				pendingReleased_[idx] = true;
		}

		/** \brief Accumulate vertical scroll wheel delta for the current frame. */
		void pushMouseWheel(float deltaY)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			pendingWheelY_ += deltaY;
		}

		/** \brief Record a ZenitUI `Key::*` press or release. */
		void pushKey(int zenitKey, bool down)
		{
			if (zenitKey == 0)
				return;
			std::lock_guard<std::mutex> lock(mtx_);
			if (down)
			{
				pendingKeys_.push_back(zenitKey);
				if (std::find(heldKeys_.begin(), heldKeys_.end(), zenitKey) == heldKeys_.end())
					heldKeys_.push_back(zenitKey);
			}
			else
			{
				heldKeys_.erase(std::remove(heldKeys_.begin(), heldKeys_.end(), zenitKey), heldKeys_.end());
			}
		}

		/** \brief Record a single typed Unicode codepoint. */
		void pushChar(uint32_t codepoint)
		{
			if (codepoint == 0)
				return;
			std::lock_guard<std::mutex> lock(mtx_);
			pendingChars_.push_back(static_cast<int>(codepoint));
		}

		/** \brief Decode a UTF-8 text input string into typed codepoints. */
		void pushTextUtf8(std::string_view utf8)
		{
			if (utf8.empty())
				return;
			std::lock_guard<std::mutex> lock(mtx_);
			size_t pos = 0;
			while (pos < utf8.size())
			{
				const uint32_t cp = Utf8::next(utf8, pos);
				if (cp > 0)
					pendingChars_.push_back(static_cast<int>(cp));
			}
		}

		/** \brief Drain queued edge-triggered mouse states using the internally tracked position and down flags. */
		PointerState drainPointer()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			return drainPointerLocked(mousePos_, down_);
		}

		/** \brief Drain queued edge-triggered mouse states while supplying live OS position and button-down flags. */
		PointerState drainPointer(Vec2 livePos, const ButtonDownArray &liveDown)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			mousePos_ = livePos;
			down_ = liveDown;
			return drainPointerLocked(livePos, liveDown);
		}

		/** \brief Drain queued key presses and typed chars, using the internally tracked held-key list. */
		InputEvents drainEvents()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			InputEvents ev;
			ev.keys = std::move(pendingKeys_);
			ev.chars = std::move(pendingChars_);
			ev.held = heldKeys_;
			pendingKeys_.clear();
			pendingChars_.clear();
			return ev;
		}

		/** \brief Drain queued key presses and typed chars, attaching a live OS held-key snapshot. */
		InputEvents drainEvents(std::vector<int> liveHeldKeys)
		{
			std::lock_guard<std::mutex> lock(mtx_);
			InputEvents ev;
			ev.keys = std::move(pendingKeys_);
			ev.chars = std::move(pendingChars_);
			ev.held = std::move(liveHeldKeys);
			pendingKeys_.clear();
			pendingChars_.clear();
			return ev;
		}

		/** \brief Clear all pending and held state (e.g. on window focus loss). */
		void clear()
		{
			std::lock_guard<std::mutex> lock(mtx_);
			pendingWheelY_ = 0.0f;
			down_.fill(false);
			pendingPressed_.fill(false);
			pendingReleased_.fill(false);
			pendingKeys_.clear();
			pendingChars_.clear();
			heldKeys_.clear();
		}

	private:
		PointerState drainPointerLocked(Vec2 pos, const ButtonDownArray &down)
		{
			PointerState state;
			state.pos = pos;
			state.wheelY = pendingWheelY_;
			pendingWheelY_ = 0.0f;

			for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i)
			{
				state.buttons[i].down = down[i];
				state.buttons[i].pressed = pendingPressed_[i];
				state.buttons[i].released = pendingReleased_[i];
				pendingPressed_[i] = false;
				pendingReleased_[i] = false;
			}
			return state;
		}

		std::mutex mtx_;
		Vec2 mousePos_{0.0f, 0.0f};
		float pendingWheelY_{0.0f};
		ButtonDownArray down_{};
		ButtonDownArray pendingPressed_{};
		ButtonDownArray pendingReleased_{};
		std::vector<int> pendingKeys_;
		std::vector<int> pendingChars_;
		std::vector<int> heldKeys_;
	};

	/** @} */
} // namespace ZenitUI