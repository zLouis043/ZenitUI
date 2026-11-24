#pragma once

/**
 * \file Backend.hpp
 * \brief Unified host interface (`IBackend`) for backend-agnostic ZenitUI apps.
 *
 * Applications that want to switch between Raylib, SDL3, or custom backends
 * without changing their main loop or asset loading can own an `IBackend`
 * instance instead of calling library-specific window/context APIs directly.
 *
 * Users integrating ZenitUI into an existing engine can still instantiate
 * `IPlatform`, `IRenderer`, and `IAssetProvider` directly without `IBackend`.
 */

#include "UIContext.hpp"

#include <string>
#include <string_view>

namespace ZenitUI
{
	/** \addtogroup grp_backends
	 *  @{
	 */

	/**
	 * \struct WindowConfig
	 * \brief Backend-agnostic window and graphics-context creation parameters.
	 */
	struct WindowConfig
	{
		std::string title{"ZenitUI"};
		int width{1280};
		int height{800};
		int minWidth{0};
		int minHeight{0};
		int targetFps{60}; ///< Hint for backends with built-in frame pacing (0 = uncapped/vsync).
		bool resizable{true};
		bool vsync{true};
		bool msaa{true};
	};

	/**
	 * \class IBackend
	 * \ingroup grp_backends
	 * \brief Unified window, frame-loop, and subsystem host for interchangeable backends.
	 */
	class IBackend
	{
	public:
		virtual ~IBackend() = default;

		/** \brief Create the window and graphics context, and bind subsystems to `UIContext`. */
		virtual bool init(const WindowConfig &config) = 0;

		/** \brief Release pooled targets, unbind `UIContext`, and tear down the window/context. */
		virtual void shutdown() = 0;

		/** \brief Pump OS events for the next frame; returns `false` when the window should close. */
		virtual bool pollFrame() = 0;

		/** \brief Return the elapsed time in seconds since the previous `pollFrame()`. */
		virtual float deltaTime() const = 0;

		/** \brief Begin presenting a frame and clear the main framebuffer to `clearColor`. */
		virtual void beginDraw(Color clearColor) = 0;

		/** \brief Finish the frame and swap buffers. */
		virtual void endDraw() = 0;

		/** \brief Short human-readable backend identifier (e.g. `"Raylib"`, `"SDL3 + OpenGL 3.3"`). */
		virtual std::string_view name() const = 0;

		/** \brief Access the underlying platform adapter. */
		virtual IPlatform &platform() = 0;

		/** \brief Access the underlying renderer adapter. */
		virtual IRenderer &renderer() = 0;

		/** \brief Access the underlying asset provider. */
		virtual IAssetProvider &assets() = 0;

		/** \brief Attach this backend's adapters to `UIContext::get()`. */
		void bindToContext()
		{
			auto &ctx = UIContext::get();
			ctx.platform = &platform();
			ctx.renderer = &renderer();
			ctx.assets = &assets();
		}

		/** \brief Clear `UIContext::get()` pointers if they still reference this backend. */
		void unbindFromContext()
		{
			auto &ctx = UIContext::get();
			if (ctx.platform == &platform())
				ctx.platform = nullptr;
			if (ctx.renderer == &renderer())
				ctx.renderer = nullptr;
			if (ctx.assets == &assets())
				ctx.assets = nullptr;
		}
	};

	/** @} */
} // namespace ZenitUI