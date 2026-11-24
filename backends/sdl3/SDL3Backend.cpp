#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "SDL3Backend.hpp"
#include "Logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

namespace ZenitUI
{
	// =========================================================================
	// Internal OpenGL 3.3 Loader, Math & File Helpers
	// =========================================================================
	namespace
	{
		constexpr int kRoundedCornerSegments = 12;
		constexpr int kCircleSegments = 32;
		constexpr size_t kMaxBatchVertices = 16384;

		struct GLProcs
		{
			PFNGLGENVERTEXARRAYSPROC GenVertexArrays{nullptr};
			PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays{nullptr};
			PFNGLBINDVERTEXARRAYPROC BindVertexArray{nullptr};
			PFNGLGENBUFFERSPROC GenBuffers{nullptr};
			PFNGLDELETEBUFFERSPROC DeleteBuffers{nullptr};
			PFNGLBINDBUFFERPROC BindBuffer{nullptr};
			PFNGLBUFFERDATAPROC BufferData{nullptr};
			PFNGLBUFFERSUBDATAPROC BufferSubData{nullptr};
			PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray{nullptr};
			PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer{nullptr};

			PFNGLCREATESHADERPROC CreateShader{nullptr};
			PFNGLSHADERSOURCEPROC ShaderSource{nullptr};
			PFNGLCOMPILESHADERPROC CompileShader{nullptr};
			PFNGLGETSHADERIVPROC GetShaderiv{nullptr};
			PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog{nullptr};
			PFNGLDELETESHADERPROC DeleteShader{nullptr};

			PFNGLCREATEPROGRAMPROC CreateProgram{nullptr};
			PFNGLATTACHSHADERPROC AttachShader{nullptr};
			PFNGLLINKPROGRAMPROC LinkProgram{nullptr};
			PFNGLGETPROGRAMIVPROC GetProgramiv{nullptr};
			PFNGLGETPROGRAMINFOLOGPROC GetProgramInfoLog{nullptr};
			PFNGLDELETEPROGRAMPROC DeleteProgram{nullptr};
			PFNGLUSEPROGRAMPROC UseProgram{nullptr};
			PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation{nullptr};
			PFNGLUNIFORM1IPROC Uniform1i{nullptr};
			PFNGLUNIFORM1FPROC Uniform1f{nullptr};
			PFNGLUNIFORM2FVPROC Uniform2fv{nullptr};
			PFNGLUNIFORM3FVPROC Uniform3fv{nullptr};
			PFNGLUNIFORM4FVPROC Uniform4fv{nullptr};
			PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv{nullptr};

			PFNGLACTIVETEXTUREPROC ActiveTexture{nullptr};
			PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate{nullptr};

			PFNGLGENFRAMEBUFFERSPROC GenFramebuffers{nullptr};
			PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers{nullptr};
			PFNGLBINDFRAMEBUFFERPROC BindFramebuffer{nullptr};
			PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D{nullptr};
			PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus{nullptr};

			template <typename Fn>
			static bool loadOne(Fn &out, const char *name)
			{
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
				out = reinterpret_cast<Fn>(SDL_GL_GetProcAddress(name));
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
				if (!out)
					logError("SDL3Renderer", "", 0, 0, std::string("Missing GL entry point: ") + name);
				return out != nullptr;
			}

			bool loadAll()
			{
				bool ok = true;
				ok &= loadOne(GenVertexArrays, "glGenVertexArrays");
				ok &= loadOne(DeleteVertexArrays, "glDeleteVertexArrays");
				ok &= loadOne(BindVertexArray, "glBindVertexArray");
				ok &= loadOne(GenBuffers, "glGenBuffers");
				ok &= loadOne(DeleteBuffers, "glDeleteBuffers");
				ok &= loadOne(BindBuffer, "glBindBuffer");
				ok &= loadOne(BufferData, "glBufferData");
				ok &= loadOne(BufferSubData, "glBufferSubData");
				ok &= loadOne(EnableVertexAttribArray, "glEnableVertexAttribArray");
				ok &= loadOne(VertexAttribPointer, "glVertexAttribPointer");

				ok &= loadOne(CreateShader, "glCreateShader");
				ok &= loadOne(ShaderSource, "glShaderSource");
				ok &= loadOne(CompileShader, "glCompileShader");
				ok &= loadOne(GetShaderiv, "glGetShaderiv");
				ok &= loadOne(GetShaderInfoLog, "glGetShaderInfoLog");
				ok &= loadOne(DeleteShader, "glDeleteShader");

				ok &= loadOne(CreateProgram, "glCreateProgram");
				ok &= loadOne(AttachShader, "glAttachShader");
				ok &= loadOne(LinkProgram, "glLinkProgram");
				ok &= loadOne(GetProgramiv, "glGetProgramiv");
				ok &= loadOne(GetProgramInfoLog, "glGetProgramInfoLog");
				ok &= loadOne(DeleteProgram, "glDeleteProgram");
				ok &= loadOne(UseProgram, "glUseProgram");
				ok &= loadOne(GetUniformLocation, "glGetUniformLocation");
				ok &= loadOne(Uniform1i, "glUniform1i");
				ok &= loadOne(Uniform1f, "glUniform1f");
				ok &= loadOne(Uniform2fv, "glUniform2fv");
				ok &= loadOne(Uniform3fv, "glUniform3fv");
				ok &= loadOne(Uniform4fv, "glUniform4fv");
				ok &= loadOne(UniformMatrix4fv, "glUniformMatrix4fv");

				ok &= loadOne(ActiveTexture, "glActiveTexture");
				ok &= loadOne(BlendFuncSeparate, "glBlendFuncSeparate");

				ok &= loadOne(GenFramebuffers, "glGenFramebuffers");
				ok &= loadOne(DeleteFramebuffers, "glDeleteFramebuffers");
				ok &= loadOne(BindFramebuffer, "glBindFramebuffer");
				ok &= loadOne(FramebufferTexture2D, "glFramebufferTexture2D");
				ok &= loadOne(CheckFramebufferStatus, "glCheckFramebufferStatus");
				return ok;
			}
		};

		// Shared default vertex shader matching ZenitUI's GLSL 330 fragment shader contract.
		constexpr const char *kDefaultVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uProjection;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
    fragTexCoord = aTexCoord;
    fragColor = aColor;
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)glsl";

		constexpr const char *kDefaultFragmentShader = R"glsl(
#version 330 core
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    finalColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
}
)glsl";

		Rect intersectRect(const Rect &a, const Rect &b)
		{
			const float x1 = std::max(a.x, b.x);
			const float y1 = std::max(a.y, b.y);
			const float x2 = std::min(a.x + a.width, b.x + b.width);
			const float y2 = std::min(a.y + a.height, b.y + b.height);
			return {x1, y1, std::max(0.0f, x2 - x1), std::max(0.0f, y2 - y1)};
		}

		// Column-major orthographic matrix mapping (0, 0) top-left to (w, h) bottom-right.
		void makeOrthoTopDown(float w, float h, float out[16])
		{
			const float safeW = (w > 0.0f) ? w : 1.0f;
			const float safeH = (h > 0.0f) ? h : 1.0f;
			std::fill(out, out + 16, 0.0f);
			out[0] = 2.0f / safeW;
			out[5] = -2.0f / safeH;
			out[10] = -1.0f;
			out[12] = -1.0f;
			out[13] = 1.0f;
			out[15] = 1.0f;
		}

		bool readTextFile(const std::string &path, std::string &out)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in)
				return false;
			std::ostringstream ss;
			ss << in.rdbuf();
			out = ss.str();
			return true;
		}

		bool readBinaryFile(const std::string &path, std::vector<unsigned char> &out)
		{
			std::ifstream in(path, std::ios::binary | std::ios::ate);
			if (!in)
				return false;
			const std::streamsize sz = in.tellg();
			if (sz <= 0)
				return false;
			in.seekg(0, std::ios::beg);
			out.resize(static_cast<size_t>(sz));
			return static_cast<bool>(in.read(reinterpret_cast<char *>(out.data()), sz));
		}

		uint32_t decodeUtf8(std::string_view s, size_t &i)
		{
			if (i >= s.size())
				return 0;
			const uint8_t b0 = static_cast<uint8_t>(s[i++]);
			if (b0 < 0x80u)
				return b0;

			auto nextCont = [&](uint32_t &cp) -> bool
			{
				if (i >= s.size())
					return false;
				const uint8_t b = static_cast<uint8_t>(s[i]);
				if ((b & 0xC0u) != 0x80u)
					return false;
				++i;
				cp = (cp << 6) | (b & 0x3Fu);
				return true;
			};

			if ((b0 & 0xE0u) == 0xC0u)
			{
				uint32_t cp = b0 & 0x1Fu;
				return nextCont(cp) ? cp : 0xFFFDu;
			}
			if ((b0 & 0xF0u) == 0xE0u)
			{
				uint32_t cp = b0 & 0x0Fu;
				return (nextCont(cp) && nextCont(cp)) ? cp : 0xFFFDu;
			}
			if ((b0 & 0xF8u) == 0xF0u)
			{
				uint32_t cp = b0 & 0x07u;
				return (nextCont(cp) && nextCont(cp) && nextCont(cp)) ? cp : 0xFFFDu;
			}
			return 0xFFFDu;
		}

		struct ScancodeBinding
		{
			SDL_Scancode sdl;
			int zenit;
		};

		constexpr ScancodeBinding kScancodeMap[] = {
			{SDL_SCANCODE_BACKSPACE, Key::Backspace},
			{SDL_SCANCODE_DELETE, Key::Delete},
			{SDL_SCANCODE_RETURN, Key::Enter},
			{SDL_SCANCODE_KP_ENTER, Key::Enter},
			{SDL_SCANCODE_ESCAPE, Key::Escape},
			{SDL_SCANCODE_LEFT, Key::Left},
			{SDL_SCANCODE_RIGHT, Key::Right},
			{SDL_SCANCODE_UP, Key::Up},
			{SDL_SCANCODE_DOWN, Key::Down},
			{SDL_SCANCODE_HOME, Key::Home},
			{SDL_SCANCODE_END, Key::End},
			{SDL_SCANCODE_TAB, Key::Tab},
			{SDL_SCANCODE_SPACE, Key::Space},
			{SDL_SCANCODE_A, Key::A},
			{SDL_SCANCODE_C, Key::C},
			{SDL_SCANCODE_V, Key::V},
			{SDL_SCANCODE_X, Key::X},
			{SDL_SCANCODE_Y, Key::Y},
			{SDL_SCANCODE_Z, Key::Z},
		};

		int mapScancode(SDL_Scancode sc)
		{
			for (const auto &entry : kScancodeMap)
			{
				if (entry.sdl == sc)
					return entry.zenit;
			}
			return 0;
		}

		int mapMouseButtonIndex(Uint8 sdlButton)
		{
			switch (sdlButton)
			{
			case SDL_BUTTON_LEFT:
				return static_cast<int>(MouseButton::Left);
			case SDL_BUTTON_RIGHT:
				return static_cast<int>(MouseButton::Right);
			case SDL_BUTTON_MIDDLE:
				return static_cast<int>(MouseButton::Middle);
			default:
				return -1;
			}
		}
	} // namespace

	// =========================================================================
	// 1. SDL3Platform (IPlatform)
	// =========================================================================

	struct SDL3Platform::Impl
	{
		SDL_Window *window{nullptr};
		std::mutex mtx;

		float pendingWheelY{0.0f};
		std::array<bool, static_cast<size_t>(MouseButton::Count)> pendingPressed{};
		std::array<bool, static_cast<size_t>(MouseButton::Count)> pendingReleased{};
		std::vector<int> pendingKeys;
		std::vector<int> pendingChars;
		bool watchInstalled{false};

		static bool SDLCALL onSdlEvent(void *userdata, SDL_Event *ev)
		{
			auto *self = static_cast<Impl *>(userdata);
			if (!self || !ev)
				return true;

			std::lock_guard<std::mutex> lock(self->mtx);
			switch (ev->type)
			{
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			{
				const int idx = mapMouseButtonIndex(ev->button.button);
				if (idx >= 0)
					self->pendingPressed[static_cast<size_t>(idx)] = true;
				break;
			}
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				const int idx = mapMouseButtonIndex(ev->button.button);
				if (idx >= 0)
					self->pendingReleased[static_cast<size_t>(idx)] = true;
				break;
			}
			case SDL_EVENT_MOUSE_WHEEL:
				self->pendingWheelY += ev->wheel.y;
				break;
			case SDL_EVENT_KEY_DOWN:
			{
				if (const int mapped = mapScancode(ev->key.scancode); mapped != 0)
					self->pendingKeys.push_back(mapped);
				break;
			}
			case SDL_EVENT_TEXT_INPUT:
			{
				if (ev->text.text)
				{
					const std::string_view sv(ev->text.text);
					size_t i = 0;
					while (i < sv.size())
					{
						const uint32_t cp = decodeUtf8(sv, i);
						if (cp > 0)
							self->pendingChars.push_back(static_cast<int>(cp));
					}
				}
				break;
			}
			default:
				break;
			}
			return true;
		}
	};

	SDL3Platform::SDL3Platform(SDL_Window *window) : impl_(std::make_unique<Impl>())
	{
		impl_->window = window;
		impl_->watchInstalled = SDL_AddEventWatch(&Impl::onSdlEvent, impl_.get());
		if (window)
			SDL_StartTextInput(window);
	}

	SDL3Platform::~SDL3Platform()
	{
		if (impl_)
		{
			if (impl_->window)
				SDL_StopTextInput(impl_->window);
			if (impl_->watchInstalled)
				SDL_RemoveEventWatch(&Impl::onSdlEvent, impl_.get());
		}
	}

	Vec2 SDL3Platform::viewportSize()
	{
		int w = 0, h = 0;
		if (impl_->window)
			SDL_GetWindowSize(impl_->window, &w, &h);
		return {static_cast<float>(w), static_cast<float>(h)};
	}

	double SDL3Platform::time()
	{
		return static_cast<double>(SDL_GetTicksNS()) * 1e-9;
	}

	float SDL3Platform::dpiScale()
	{
		if (!impl_->window)
			return 1.0f;
		const float density = SDL_GetWindowPixelDensity(impl_->window);
		return (density > 0.01f) ? density : 1.0f;
	}

	EdgeInsets SDL3Platform::safeArea()
	{
		return {};
	}

	PointerState SDL3Platform::pointer()
	{
		float mx = 0.0f, my = 0.0f;
		const SDL_MouseButtonFlags mask = SDL_GetMouseState(&mx, &my);

		PointerState state;
		state.pos = {mx, my};

		{
			std::lock_guard<std::mutex> lock(impl_->mtx);
			state.wheelY = impl_->pendingWheelY;
			impl_->pendingWheelY = 0.0f;

			auto fillBtn = [&](MouseButton btn, Uint32 sdlMask)
			{
				const size_t idx = static_cast<size_t>(btn);
				state[btn].down = (mask & sdlMask) != 0;
				state[btn].pressed = impl_->pendingPressed[idx];
				state[btn].released = impl_->pendingReleased[idx];
				impl_->pendingPressed[idx] = false;
				impl_->pendingReleased[idx] = false;
			};

			fillBtn(MouseButton::Left, SDL_BUTTON_LMASK);
			fillBtn(MouseButton::Right, SDL_BUTTON_RMASK);
			fillBtn(MouseButton::Middle, SDL_BUTTON_MMASK);
		}

		return state;
	}

	ModifierState SDL3Platform::modifiers()
	{
		const SDL_Keymod km = SDL_GetModState();
		ModifierState m;
		m.shift = (km & SDL_KMOD_SHIFT) != 0;
		m.ctrl = (km & SDL_KMOD_CTRL) != 0;
		m.alt = (km & SDL_KMOD_ALT) != 0;
		m.super = (km & SDL_KMOD_GUI) != 0;
		return m;
	}

	InputEvents SDL3Platform::pollInputEvents()
	{
		InputEvents ev;
		{
			std::lock_guard<std::mutex> lock(impl_->mtx);
			ev.keys = std::move(impl_->pendingKeys);
			ev.chars = std::move(impl_->pendingChars);
			impl_->pendingKeys.clear();
			impl_->pendingChars.clear();
		}

		int numKeys = 0;
		const bool *kb = SDL_GetKeyboardState(&numKeys);
		if (kb && numKeys > 0)
		{
			for (const auto &entry : kScancodeMap)
			{
				if (entry.sdl < numKeys && kb[entry.sdl])
					ev.held.push_back(entry.zenit);
			}
		}

		return ev;
	}

	void SDL3Platform::setClipboardText(const std::string &text)
	{
		SDL_SetClipboardText(text.c_str());
	}

	std::string SDL3Platform::getClipboardText()
	{
		char *raw = SDL_GetClipboardText();
		if (!raw)
			return {};
		std::string result(raw);
		SDL_free(raw);
		return result;
	}

	// =========================================================================
	// 2. SDL3Renderer (IRenderer — OpenGL 3.3 Core 2D Batcher)
	// =========================================================================

	struct SDL3Renderer::Impl
	{
		struct Vertex
		{
			float x, y;
			float u, v;
			uint8_t r, g, b, a;
		};

		struct TextureObj
		{
			GLuint glId{0};
			int width{0};
			int height{0};
		};

		struct GlyphInfo
		{
			float u0{0}, v0{0}, u1{0}, v1{0};
			float xoff{0}, yoff{0}, w{0}, h{0};
			float xadvance{0};
		};

		struct FontObj
		{
			GLuint glTex{0};
			float baseSize{48.0f};
			float ascentPx{0.0f};
			std::unordered_map<uint32_t, GlyphInfo> glyphs;
		};

		using UniformCache = std::unordered_map<
			std::string, GLint, detail::SdlTransparentStringHash, std::equal_to<>>;

		struct ShaderObj
		{
			GLuint program{0};
			GLint locProjection{-1};
			GLint locTexture0{-1};
			GLint locColDiffuse{-1};
			UniformCache uniforms;
		};

		struct TargetObj
		{
			GLuint fbo{0};
			GLuint colorTex{0};
			int width{0};
			int height{0};
		};

		struct TargetState
		{
			TargetHandle target{};
			bool clipActive{false};
			Rect currentClip{0, 0, 0, 0};
			std::vector<Rect> clipStack;
			std::vector<Transform2D> transformStack;
			int targetW{0};
			int targetH{0};
		};

		SDL_Window *window{nullptr};
		GLProcs gl;
		bool ready{false};

		GLuint vao{0};
		GLuint vbo{0};
		GLuint whiteTex{0};
		ShaderObj defaultShader{};

		std::vector<Vertex> batch;
		GLuint batchTex{0};
		GLuint activeProgram{0};
		float activeProjection[16]{};

		uint32_t nextId{1};
		std::unordered_map<uint32_t, TextureObj> textures;
		std::unordered_map<uint32_t, FontObj> fonts;
		std::unordered_map<uint32_t, ShaderObj> shaders;
		std::unordered_map<uint32_t, TargetObj> targets;

		std::vector<TargetState> targetStates;
		std::vector<Rect> clipStack;
		std::vector<Transform2D> transformStack;
		std::vector<EffectHandle> effectStack;

		bool clipActive{false};
		Rect currentClip{0, 0, 0, 0};
		float dpiScale{1.0f};
		int fbWidth{1};
		int fbHeight{1};

		bool insideTarget{false};
		TargetHandle currentTarget{};
		int currentTargetW{0};
		int currentTargetH{0};

		GLuint compileStage(GLenum type, const char *src)
		{
			GLuint s = gl.CreateShader(type);
			gl.ShaderSource(s, 1, &src, nullptr);
			gl.CompileShader(s);
			GLint ok = 0;
			gl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
			if (!ok)
			{
				char log[1024];
				gl.GetShaderInfoLog(s, sizeof(log), nullptr, log);
				logError("SDL3Renderer", "", 0, 0, std::string("GLSL compile error: ") + log);
				gl.DeleteShader(s);
				return 0;
			}
			return s;
		}

		ShaderObj buildShaderProgram(const char *vsSrc, const char *fsSrc)
		{
			ShaderObj obj;
			GLuint vs = compileStage(GL_VERTEX_SHADER, vsSrc);
			if (!vs)
				return obj;
			GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsSrc);
			if (!fs)
			{
				gl.DeleteShader(vs);
				return obj;
			}

			GLuint prog = gl.CreateProgram();
			gl.AttachShader(prog, vs);
			gl.AttachShader(prog, fs);
			gl.LinkProgram(prog);
			gl.DeleteShader(vs);
			gl.DeleteShader(fs);

			GLint ok = 0;
			gl.GetProgramiv(prog, GL_LINK_STATUS, &ok);
			if (!ok)
			{
				char log[1024];
				gl.GetProgramInfoLog(prog, sizeof(log), nullptr, log);
				logError("SDL3Renderer", "", 0, 0, std::string("GLSL link error: ") + log);
				gl.DeleteProgram(prog);
				return obj;
			}

			obj.program = prog;
			obj.locProjection = gl.GetUniformLocation(prog, "uProjection");
			obj.locTexture0 = gl.GetUniformLocation(prog, "texture0");
			obj.locColDiffuse = gl.GetUniformLocation(prog, "colDiffuse");
			return obj;
		}

		GLint uniformLoc(ShaderObj &sh, const char *name)
		{
			if (auto it = sh.uniforms.find(std::string_view(name)); it != sh.uniforms.end())
				return it->second;
			const GLint loc = gl.GetUniformLocation(sh.program, name);
			sh.uniforms.emplace(name, loc);
			return loc;
		}

		const ShaderObj &currentShaderObj() const
		{
			if (!effectStack.empty())
			{
				if (auto it = shaders.find(effectStack.back().id); it != shaders.end())
					return it->second;
			}
			return defaultShader;
		}

		void bindActiveProgramUniforms()
		{
			const ShaderObj &sh = currentShaderObj();
			activeProgram = sh.program;
			gl.UseProgram(activeProgram);
			if (sh.locProjection >= 0)
				gl.UniformMatrix4fv(sh.locProjection, 1, GL_FALSE, activeProjection);
			if (sh.locTexture0 >= 0)
				gl.Uniform1i(sh.locTexture0, 0);
			if (sh.locColDiffuse >= 0)
			{
				constexpr float kWhite[4] = {1.0f, 1.0f, 1.0f, 1.0f};
				gl.Uniform4fv(sh.locColDiffuse, 1, kWhite);
			}
		}

		void updateProjectionAndViewport()
		{
			if (insideTarget)
			{
				glViewport(0, 0, currentTargetW, currentTargetH);
				makeOrthoTopDown(static_cast<float>(currentTargetW),
								 static_cast<float>(currentTargetH),
								 activeProjection);
			}
			else
			{
				glViewport(0, 0, fbWidth, fbHeight);
				makeOrthoTopDown(static_cast<float>(fbWidth) / dpiScale,
								 static_cast<float>(fbHeight) / dpiScale,
								 activeProjection);
			}
			bindActiveProgramUniforms();
		}

		void flush()
		{
			if (batch.empty() || !ready)
				return;

			bindActiveProgramUniforms();
			gl.ActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, batchTex ? batchTex : whiteTex);

			gl.BindVertexArray(vao);
			gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
			gl.BufferSubData(GL_ARRAY_BUFFER,
							 0,
							 static_cast<GLsizeiptr>(batch.size() * sizeof(Vertex)),
							 batch.data());
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch.size()));
			batch.clear();
		}

		void ensureBatch(GLuint tex, size_t vertexCount)
		{
			const GLuint resolved = tex ? tex : whiteTex;
			if (batchTex != resolved || batch.size() + vertexCount > kMaxBatchVertices)
			{
				flush();
				batchTex = resolved;
			}
		}

		Vec2 transformPoint(Vec2 p) const
		{
			for (auto it = transformStack.rbegin(); it != transformStack.rend(); ++it)
				p = applyTransform(*it, p);
			return p;
		}

		void pushVertex(Vec2 p, float u, float v, Color c)
		{
			if (!transformStack.empty())
				p = transformPoint(p);
			batch.push_back({p.x, p.y, u, v, c.r, c.g, c.b, c.a});
		}

		void pushQuad(Rect dst, float u0, float v0, float u1, float v1, Color c, GLuint tex)
		{
			if (dst.width <= 0.0f || dst.height <= 0.0f || c.a == 0)
				return;
			ensureBatch(tex, 6);

			const Vec2 tl{dst.x, dst.y};
			const Vec2 tr{dst.x + dst.width, dst.y};
			const Vec2 br{dst.x + dst.width, dst.y + dst.height};
			const Vec2 bl{dst.x, dst.y + dst.height};

			pushVertex(tl, u0, v0, c);
			pushVertex(bl, u0, v1, c);
			pushVertex(br, u1, v1, c);

			pushVertex(tl, u0, v0, c);
			pushVertex(br, u1, v1, c);
			pushVertex(tr, u1, v0, c);
		}

		Rect transformClipToScreen(const Rect &local) const
		{
			if (transformStack.empty())
				return local;

			Vec2 corners[4] = {
				transformPoint({local.x, local.y}),
				transformPoint({local.x + local.width, local.y}),
				transformPoint({local.x, local.y + local.height}),
				transformPoint({local.x + local.width, local.y + local.height})};

			float minX = corners[0].x, maxX = corners[0].x;
			float minY = corners[0].y, maxY = corners[0].y;
			for (int i = 1; i < 4; ++i)
			{
				minX = std::min(minX, corners[i].x);
				maxX = std::max(maxX, corners[i].x);
				minY = std::min(minY, corners[i].y);
				maxY = std::max(maxY, corners[i].y);
			}
			return {minX, minY, maxX - minX, maxY - minY};
		}

		void applyScissor(const Rect &clip)
		{
			flush();
			const float scale = insideTarget ? 1.0f : dpiScale;
			const int targetH = insideTarget ? currentTargetH : fbHeight;

			const int x = static_cast<int>(std::floor(clip.x * scale));
			const int y = static_cast<int>(std::floor(clip.y * scale));
			const int w = static_cast<int>(std::ceil(clip.width * scale));
			const int h = static_cast<int>(std::ceil(clip.height * scale));

			glEnable(GL_SCISSOR_TEST);
			if (w <= 0 || h <= 0)
				glScissor(0, 0, 0, 0);
			else
				glScissor(x, targetH - (y + h), w, h);
		}
	};

	SDL3Renderer::SDL3Renderer(SDL_Window *window) : impl_(std::make_unique<Impl>())
	{
		impl_->window = window;
		if (!impl_->gl.loadAll())
			return;

		impl_->defaultShader = impl_->buildShaderProgram(kDefaultVertexShader, kDefaultFragmentShader);
		if (impl_->defaultShader.program == 0)
			return;

		impl_->gl.GenVertexArrays(1, &impl_->vao);
		impl_->gl.GenBuffers(1, &impl_->vbo);
		impl_->gl.BindVertexArray(impl_->vao);
		impl_->gl.BindBuffer(GL_ARRAY_BUFFER, impl_->vbo);
		impl_->gl.BufferData(GL_ARRAY_BUFFER,
							 static_cast<GLsizeiptr>(kMaxBatchVertices * sizeof(Impl::Vertex)),
							 nullptr,
							 GL_STREAM_DRAW);

		impl_->gl.EnableVertexAttribArray(0);
		impl_->gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Impl::Vertex),
									  reinterpret_cast<void *>(offsetof(Impl::Vertex, x)));
		impl_->gl.EnableVertexAttribArray(1);
		impl_->gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Impl::Vertex),
									  reinterpret_cast<void *>(offsetof(Impl::Vertex, u)));
		impl_->gl.EnableVertexAttribArray(2);
		impl_->gl.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Impl::Vertex),
									  reinterpret_cast<void *>(offsetof(Impl::Vertex, r)));
		impl_->gl.BindVertexArray(0);

		// 1x1 opaque white texture so untextured primitives share the exact same shader pipeline.
		const uint8_t whitePixel[4] = {255, 255, 255, 255};
		glGenTextures(1, &impl_->whiteTex);
		glBindTexture(GL_TEXTURE_2D, impl_->whiteTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
		glBindTexture(GL_TEXTURE_2D, 0);

		impl_->batch.reserve(kMaxBatchVertices);
		impl_->ready = true;
	}

	SDL3Renderer::~SDL3Renderer()
	{
		if (!impl_ || !impl_->ready)
			return;

		for (auto &[id, t] : impl_->targets)
		{
			if (t.colorTex)
				glDeleteTextures(1, &t.colorTex);
			if (t.fbo)
				impl_->gl.DeleteFramebuffers(1, &t.fbo);
		}
		for (auto &[id, t] : impl_->textures)
		{
			if (t.glId)
				glDeleteTextures(1, &t.glId);
		}
		for (auto &[id, f] : impl_->fonts)
		{
			if (f.glTex)
				glDeleteTextures(1, &f.glTex);
		}
		for (auto &[id, s] : impl_->shaders)
		{
			if (s.program)
				impl_->gl.DeleteProgram(s.program);
		}
		if (impl_->whiteTex)
			glDeleteTextures(1, &impl_->whiteTex);
		if (impl_->defaultShader.program)
			impl_->gl.DeleteProgram(impl_->defaultShader.program);
		if (impl_->vbo)
			impl_->gl.DeleteBuffers(1, &impl_->vbo);
		if (impl_->vao)
			impl_->gl.DeleteVertexArrays(1, &impl_->vao);
	}

	bool SDL3Renderer::isReady() const
	{
		return impl_ && impl_->ready;
	}

	// ---- Frame lifecycle & DPI ----

	void SDL3Renderer::setDpiScale(float scale)
	{
		impl_->dpiScale = (scale > 0.01f) ? scale : 1.0f;
	}

	void SDL3Renderer::beginFrame()
	{
		if (!impl_->ready)
			return;

		if (impl_->window)
			SDL_GetWindowSizeInPixels(impl_->window, &impl_->fbWidth, &impl_->fbHeight);
		impl_->fbWidth = std::max(1, impl_->fbWidth);
		impl_->fbHeight = std::max(1, impl_->fbHeight);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_BLEND);
		impl_->gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
									GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		impl_->updateProjectionAndViewport();
	}

	void SDL3Renderer::endFrame()
	{
		if (!impl_->ready)
			return;
		impl_->flush();
		glDisable(GL_SCISSOR_TEST);
	}

	// ---- 2D vector primitives ----

	void SDL3Renderer::fillRect(Rect r, Color c)
	{
		impl_->pushQuad(r, 0.0f, 0.0f, 1.0f, 1.0f, c, impl_->whiteTex);
	}

	void SDL3Renderer::fillRoundedRect(Rect r, float radiusPx, Color c)
	{
		if (r.width <= 0.0f || r.height <= 0.0f || c.a == 0)
			return;

		const float maxR = std::min(r.width, r.height) * 0.5f;
		const float rad = std::clamp(radiusPx, 0.0f, maxR);
		if (rad <= 0.01f)
		{
			fillRect(r, c);
			return;
		}

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);
		constexpr float kStartAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};
		constexpr int kRingPts = 4 * (segs + 1);

		const Vec2 centers[4] = {
			{r.x + r.width - rad, r.y + rad},
			{r.x + r.width - rad, r.y + r.height - rad},
			{r.x + rad,           r.y + r.height - rad},
			{r.x + rad,           r.y + rad}};

		Vec2 ring[kRingPts];
		int idx = 0;
		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				const float a = kStartAngles[corner] + kStep * static_cast<float>(s);
				ring[idx++] = {centers[corner].x + std::cos(a) * rad,
							   centers[corner].y + std::sin(a) * rad};
			}
		}

		const Vec2 center{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
		impl_->ensureBatch(impl_->whiteTex, static_cast<size_t>(kRingPts) * 3);
		for (int i = 0; i < kRingPts; ++i)
		{
			const int next = (i + 1) % kRingPts;
			impl_->pushVertex(center, 0.5f, 0.5f, c);
			impl_->pushVertex(ring[next], 0.5f, 0.5f, c);
			impl_->pushVertex(ring[i], 0.5f, 0.5f, c);
		}
	}

	void SDL3Renderer::fillCircle(Vec2 center, float radius, Color c)
	{
		if (radius <= 0.0f || c.a == 0)
			return;

		constexpr int segs = kCircleSegments;
		constexpr float kTwoPi = 6.2831853071795864f;
		constexpr float kStep = kTwoPi / static_cast<float>(segs);

		impl_->ensureBatch(impl_->whiteTex, static_cast<size_t>(segs) * 3);
		for (int i = 0; i < segs; ++i)
		{
			const float a0 = kStep * static_cast<float>(i);
			const float a1 = kStep * static_cast<float>(i + 1);
			impl_->pushVertex(center, 0.5f, 0.5f, c);
			impl_->pushVertex({center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius}, 0.5f, 0.5f, c);
			impl_->pushVertex({center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius}, 0.5f, 0.5f, c);
		}
	}

	void SDL3Renderer::strokeRect(Rect r, float thickness, Color c)
	{
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f || c.a == 0)
			return;
		const float t = std::min(thickness, std::min(r.width, r.height) * 0.5f);
		fillBorder(r, {t, t, t, t}, c);
	}

	void SDL3Renderer::strokeRoundedRect(Rect r, float radiusPx, float thickness, Color c)
	{
		if (thickness <= 0.0f || r.width <= 0.0f || r.height <= 0.0f || c.a == 0)
			return;

		const float maxR = std::min(r.width, r.height) * 0.5f;
		if (maxR <= 0.0f)
			return;

		const float t = std::min(thickness, maxR);
		if (t >= maxR - 0.01f)
		{
			fillRoundedRect(r, radiusPx, c);
			return;
		}

		const float outerR = std::clamp(radiusPx, 0.0f, maxR);
		const float innerR = std::max(0.0f, outerR - t);

		constexpr int segs = kRoundedCornerSegments;
		constexpr float kHalfPi = 1.5707963267948966f;
		constexpr float kStep = kHalfPi / static_cast<float>(segs);
		constexpr float kStartAngles[4] = {-kHalfPi, 0.0f, kHalfPi, 2.0f * kHalfPi};
		constexpr int kRingPts = 4 * (segs + 1);

		const Vec2 outerCenters[4] = {
			{r.x + r.width - outerR, r.y + outerR},
			{r.x + r.width - outerR, r.y + r.height - outerR},
			{r.x + outerR,           r.y + r.height - outerR},
			{r.x + outerR,           r.y + outerR}};

		const Vec2 innerCenters[4] = {
			{r.x + r.width - t - innerR, r.y + t + innerR},
			{r.x + r.width - t - innerR, r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + r.height - t - innerR},
			{r.x + t + innerR,           r.y + t + innerR}};

		Vec2 outerPts[kRingPts];
		Vec2 innerPts[kRingPts];

		int idx = 0;
		for (int corner = 0; corner < 4; ++corner)
		{
			for (int s = 0; s <= segs; ++s)
			{
				const float a = kStartAngles[corner] + kStep * static_cast<float>(s);
				const float ca = std::cos(a);
				const float sa = std::sin(a);
				outerPts[idx] = {outerCenters[corner].x + ca * outerR,
								 outerCenters[corner].y + sa * outerR};
				innerPts[idx] = {innerCenters[corner].x + ca * innerR,
								 innerCenters[corner].y + sa * innerR};
				++idx;
			}
		}

		impl_->ensureBatch(impl_->whiteTex, static_cast<size_t>(kRingPts) * 6);
		for (int i = 0; i < kRingPts; ++i)
		{
			const int next = (i + 1) % kRingPts;
			impl_->pushVertex(outerPts[i], 0.5f, 0.5f, c);
			impl_->pushVertex(innerPts[next], 0.5f, 0.5f, c);
			impl_->pushVertex(outerPts[next], 0.5f, 0.5f, c);

			impl_->pushVertex(outerPts[i], 0.5f, 0.5f, c);
			impl_->pushVertex(innerPts[i], 0.5f, 0.5f, c);
			impl_->pushVertex(innerPts[next], 0.5f, 0.5f, c);
		}
	}

	// ---- Textures & nine-slice ----

	void SDL3Renderer::drawTexture(TextureHandle t, Rect src, Rect dst, Color tint)
	{
		auto it = impl_->textures.find(t.id);
		if (it == impl_->textures.end() || it->second.width <= 0 || it->second.height <= 0)
			return;
		const float tw = static_cast<float>(it->second.width);
		const float th = static_cast<float>(it->second.height);
		impl_->pushQuad(dst,
						src.x / tw,
						src.y / th,
						(src.x + src.width) / tw,
						(src.y + src.height) / th,
						tint,
						it->second.glId);
	}

	void SDL3Renderer::drawNineSlice(TextureHandle t, NineSlice s, Rect dst, Color tint)
	{
		auto it = impl_->textures.find(t.id);
		if (it == impl_->textures.end() || it->second.width <= 0 || it->second.height <= 0)
			return;

		const float tw = static_cast<float>(it->second.width);
		const float th = static_cast<float>(it->second.height);

		const float sl = std::clamp(static_cast<float>(s.left), 0.0f, tw);
		const float sr = std::clamp(static_cast<float>(s.right), 0.0f, tw - sl);
		const float st = std::clamp(static_cast<float>(s.top), 0.0f, th);
		const float sb = std::clamp(static_cast<float>(s.bottom), 0.0f, th - st);

		const float dl = std::min(sl, dst.width * 0.5f);
		const float dr = std::min(sr, dst.width - dl);
		const float dt = std::min(st, dst.height * 0.5f);
		const float db = std::min(sb, dst.height - dt);

		const float xs[4] = {dst.x, dst.x + dl, dst.x + dst.width - dr, dst.x + dst.width};
		const float ys[4] = {dst.y, dst.y + dt, dst.y + dst.height - db, dst.y + dst.height};
		const float us[4] = {0.0f, sl / tw, (tw - sr) / tw, 1.0f};
		const float vs[4] = {0.0f, st / th, (th - sb) / th, 1.0f};

		for (int row = 0; row < 3; ++row)
		{
			for (int col = 0; col < 3; ++col)
			{
				const Rect cell{xs[col], ys[row], xs[col + 1] - xs[col], ys[row + 1] - ys[row]};
				impl_->pushQuad(cell, us[col], vs[row], us[col + 1], vs[row + 1], tint, it->second.glId);
			}
		}
	}

	// ---- Text rendering & metrics ----

	void SDL3Renderer::drawText(FontHandle f, std::string_view s, Vec2 pos, float size, float spacing, Color c)
	{
		if (s.empty() || size <= 0.0f || c.a == 0)
			return;

		auto it = impl_->fonts.find(f.id);
		if (it == impl_->fonts.end())
		{
			if (impl_->fonts.empty())
				return;
			it = impl_->fonts.begin();
		}

		const auto &font = it->second;
		const float ratio = size / font.baseSize;
		const float startX = std::round(pos.x);
		float cursorX = startX;
		float baselineY = std::round(pos.y + font.ascentPx * ratio);

		size_t i = 0;
		while (i < s.size())
		{
			const uint32_t cp = decodeUtf8(s, i);
			if (cp == '\n')
			{
				cursorX = startX;
				baselineY = std::round(baselineY + size * 1.2f);
				continue;
			}
			auto git = font.glyphs.find(cp);
			if (git == font.glyphs.end())
				git = font.glyphs.find('?');
			if (git == font.glyphs.end())
				continue;

			const auto &g = git->second;
			if (g.w > 0.0f && g.h > 0.0f)
			{
				const Rect dst{
					cursorX + g.xoff * ratio,
					baselineY + g.yoff * ratio,
					g.w * ratio,
					g.h * ratio};
				impl_->pushQuad(dst, g.u0, g.v0, g.u1, g.v1, c, font.glTex);
			}
			cursorX += g.xadvance * ratio + spacing;
		}
	}

	Vec2 SDL3Renderer::measureText(FontHandle f, std::string_view s, float size, float spacing)
	{
		if (s.empty() || size <= 0.0f)
			return {0.0f, size};

		auto it = impl_->fonts.find(f.id);
		if (it == impl_->fonts.end())
		{
			if (impl_->fonts.empty())
			{
				const size_t n = Utf8::codepointCount(s);
				const float w = static_cast<float>(n) * (size * 0.5f) +
								(n > 1 ? spacing * static_cast<float>(n - 1) : 0.0f);
				return {w, size};
			}
			it = impl_->fonts.begin();
		}

		const auto &font = it->second;
		const float ratio = size / font.baseSize;
		float lineW = 0.0f;
		float maxW = 0.0f;
		int lines = 1;
		bool lineHasGlyph = false;

		size_t i = 0;
		while (i < s.size())
		{
			const uint32_t cp = decodeUtf8(s, i);
			if (cp == '\n')
			{
				maxW = std::max(maxW, lineW);
				lineW = 0.0f;
				lineHasGlyph = false;
				++lines;
				continue;
			}
			auto git = font.glyphs.find(cp);
			if (git == font.glyphs.end())
				git = font.glyphs.find('?');
			if (git == font.glyphs.end())
				continue;

			if (lineHasGlyph)
				lineW += spacing;
			lineW += git->second.xadvance * ratio;
			lineHasGlyph = true;
		}

		maxW = std::max(maxW, lineW);
		const float totalH = (lines == 1) ? size : (size + static_cast<float>(lines - 1) * size * 1.2f);
		return {maxW, totalH};
	}

	Vec2 SDL3Renderer::measureCodepoint(FontHandle f, uint32_t codepoint, float size, float /*spacing*/)
	{
		auto it = impl_->fonts.find(f.id);
		if (it == impl_->fonts.end())
		{
			if (impl_->fonts.empty())
				return {size * 0.5f, size};
			it = impl_->fonts.begin();
		}

		const auto &font = it->second;
		auto git = font.glyphs.find(codepoint);
		if (git == font.glyphs.end())
			git = font.glyphs.find('?');
		if (git == font.glyphs.end())
			return {0.0f, size};
		return {git->second.xadvance * (size / font.baseSize), size};
	}

	// ---- Transform, clip & shader stacks ----

	void SDL3Renderer::pushTransform(const Transform2D &t)
	{
		impl_->transformStack.push_back(t);
	}

	void SDL3Renderer::popTransform()
	{
		if (!impl_->transformStack.empty())
			impl_->transformStack.pop_back();
	}

	void SDL3Renderer::pushClip(Rect r)
	{
		const Rect screenRect = impl_->transformClipToScreen(r);
		if (impl_->clipActive)
		{
			impl_->clipStack.push_back(impl_->currentClip);
			impl_->currentClip = intersectRect(impl_->currentClip, screenRect);
		}
		else
		{
			const Rect bounds = impl_->insideTarget
									? Rect{0.0f, 0.0f, static_cast<float>(impl_->currentTargetW), static_cast<float>(impl_->currentTargetH)}
									: Rect{0.0f, 0.0f, static_cast<float>(impl_->fbWidth) / impl_->dpiScale, static_cast<float>(impl_->fbHeight) / impl_->dpiScale};
			impl_->clipStack.push_back(bounds);
			impl_->currentClip = intersectRect(bounds, screenRect);
			impl_->clipActive = true;
		}
		impl_->applyScissor(impl_->currentClip);
	}

	void SDL3Renderer::popClip()
	{
		if (impl_->clipStack.empty())
			return;
		impl_->currentClip = impl_->clipStack.back();
		impl_->clipStack.pop_back();

		if (impl_->clipStack.empty())
		{
			impl_->flush();
			impl_->clipActive = false;
			glDisable(GL_SCISSOR_TEST);
			return;
		}
		impl_->applyScissor(impl_->currentClip);
	}

	Rect SDL3Renderer::getClipRect() const
	{
		if (impl_->insideTarget && !impl_->targetStates.empty())
		{
			const auto &root = impl_->targetStates.front();
			return root.clipActive
					   ? root.currentClip
					   : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
		}
		return impl_->clipActive
				   ? impl_->currentClip
				   : Rect{0.0f, 0.0f, Metrics::viewport.x, Metrics::viewport.y};
	}

	void SDL3Renderer::pushEffect(EffectHandle e)
	{
		if (impl_->shaders.find(e.id) == impl_->shaders.end())
			return;
		impl_->flush();
		impl_->effectStack.push_back(e);
		impl_->bindActiveProgramUniforms();
	}

	void SDL3Renderer::popEffect()
	{
		if (impl_->effectStack.empty())
			return;
		impl_->flush();
		impl_->effectStack.pop_back();
		impl_->bindActiveProgramUniforms();
	}

	// ---- Shader uniforms ----

	void SDL3Renderer::setEffectFloat(EffectHandle e, const char *name, float value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		impl_->gl.UseProgram(it->second.program);
		impl_->gl.Uniform1f(loc, value);
		impl_->gl.UseProgram(impl_->activeProgram);
	}

	void SDL3Renderer::setEffectVec2(EffectHandle e, const char *name, Vec2 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		const float arr[2] = {value.x, value.y};
		impl_->gl.UseProgram(it->second.program);
		impl_->gl.Uniform2fv(loc, 1, arr);
		impl_->gl.UseProgram(impl_->activeProgram);
	}

	void SDL3Renderer::setEffectVec3(EffectHandle e, const char *name, Vec3 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		const float arr[3] = {value.x, value.y, value.z};
		impl_->gl.UseProgram(it->second.program);
		impl_->gl.Uniform3fv(loc, 1, arr);
		impl_->gl.UseProgram(impl_->activeProgram);
	}

	void SDL3Renderer::setEffectVec4f(EffectHandle e, const char *name, Vec4 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		const float arr[4] = {value.x, value.y, value.z, value.w};
		impl_->gl.UseProgram(it->second.program);
		impl_->gl.Uniform4fv(loc, 1, arr);
		impl_->gl.UseProgram(impl_->activeProgram);
	}

	void SDL3Renderer::setEffectVec4(EffectHandle e, const char *name, Color value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		const float arr[4] = {
			value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
		impl_->gl.UseProgram(it->second.program);
		impl_->gl.Uniform4fv(loc, 1, arr);
		impl_->gl.UseProgram(impl_->activeProgram);
	}

	// ---- Off-screen render targets ----

	TargetHandle SDL3Renderer::createTarget(int w, int h)
	{
		if (!impl_->ready || w <= 0 || h <= 0)
			return {};

		impl_->flush();

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		GLuint fbo = 0;
		impl_->gl.GenFramebuffers(1, &fbo);
		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
		impl_->gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		const GLenum status = impl_->gl.CheckFramebufferStatus(GL_FRAMEBUFFER);

		// Restore previously bound FBO (screen = 0 or active nested target).
		GLuint prevFbo = 0;
		if (impl_->insideTarget)
		{
			if (auto it = impl_->targets.find(impl_->currentTarget.id); it != impl_->targets.end())
				prevFbo = it->second.fbo;
		}
		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, prevFbo);

		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			logError("SDL3Renderer", "", 0, 0, "Failed to create off-screen FBO");
			impl_->gl.DeleteFramebuffers(1, &fbo);
			glDeleteTextures(1, &tex);
			return {};
		}

		const uint32_t id = impl_->nextId++;
		impl_->targets[id] = {fbo, tex, w, h};
		return {id, w, h};
	}

	void SDL3Renderer::destroyTarget(TargetHandle t)
	{
		if (auto it = impl_->targets.find(t.id); it != impl_->targets.end())
		{
			if (it->second.colorTex)
				glDeleteTextures(1, &it->second.colorTex);
			if (it->second.fbo)
				impl_->gl.DeleteFramebuffers(1, &it->second.fbo);
			impl_->targets.erase(it);
		}
	}

	void SDL3Renderer::pushTarget(TargetHandle t)
	{
		auto it = impl_->targets.find(t.id);
		if (it == impl_->targets.end())
			return;

		impl_->flush();
		if (impl_->clipActive)
			glDisable(GL_SCISSOR_TEST);

		impl_->targetStates.push_back({
			impl_->currentTarget,
			impl_->clipActive,
			impl_->currentClip,
			std::move(impl_->clipStack),
			std::move(impl_->transformStack),
			impl_->currentTargetW,
			impl_->currentTargetH});

		impl_->clipActive = false;
		impl_->clipStack.clear();
		impl_->currentClip = {0.0f, 0.0f, static_cast<float>(t.width), static_cast<float>(t.height)};
		impl_->transformStack.clear();

		impl_->currentTarget = t;
		impl_->currentTargetW = t.width;
		impl_->currentTargetH = t.height;
		impl_->insideTarget = true;

		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
		impl_->updateProjectionAndViewport();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SDL3Renderer::popTarget()
	{
		impl_->flush();
		if (impl_->clipActive)
			glDisable(GL_SCISSOR_TEST);

		if (!impl_->targetStates.empty())
		{
			auto prev = std::move(impl_->targetStates.back());
			impl_->targetStates.pop_back();
			impl_->currentTarget = prev.target;
			impl_->clipActive = prev.clipActive;
			impl_->currentClip = prev.currentClip;
			impl_->clipStack = std::move(prev.clipStack);
			impl_->transformStack = std::move(prev.transformStack);
			impl_->currentTargetW = prev.targetW;
			impl_->currentTargetH = prev.targetH;
		}
		else
		{
			impl_->currentTarget = {};
			impl_->clipActive = false;
			impl_->clipStack.clear();
			impl_->currentClip = {0.0f, 0.0f,
								  static_cast<float>(impl_->fbWidth) / impl_->dpiScale,
								  static_cast<float>(impl_->fbHeight) / impl_->dpiScale};
			impl_->transformStack.clear();
			impl_->currentTargetW = 0;
			impl_->currentTargetH = 0;
		}

		impl_->insideTarget = impl_->currentTarget.valid();

		GLuint fbo = 0;
		if (impl_->insideTarget)
		{
			if (auto it = impl_->targets.find(impl_->currentTarget.id); it != impl_->targets.end())
				fbo = it->second.fbo;
		}
		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
		impl_->updateProjectionAndViewport();

		if (impl_->clipActive)
			impl_->applyScissor(impl_->currentClip);
	}

	void SDL3Renderer::drawTarget(TargetHandle t, Rect dst, Color tint)
	{
		auto it = impl_->targets.find(t.id);
		if (it == impl_->targets.end())
			return;
		// OpenGL FBO textures are stored bottom-up: v=1 is top, v=0 is bottom.
		impl_->pushQuad(dst, 0.0f, 1.0f, 1.0f, 0.0f, tint, it->second.colorTex);
	}

	void SDL3Renderer::drawTarget(TargetHandle t, Rect src, Rect dst, Color tint)
	{
		auto it = impl_->targets.find(t.id);
		if (it == impl_->targets.end() || it->second.width <= 0 || it->second.height <= 0)
			return;
		const float tw = static_cast<float>(it->second.width);
		const float th = static_cast<float>(it->second.height);
		const float u0 = src.x / tw;
		const float u1 = (src.x + src.width) / tw;
		const float vTop = 1.0f - (src.y / th);
		const float vBot = 1.0f - ((src.y + src.height) / th);
		impl_->pushQuad(dst, u0, vTop, u1, vBot, tint, it->second.colorTex);
	}

	bool SDL3Renderer::inTarget() const
	{
		return impl_->insideTarget;
	}

	void SDL3Renderer::clearTarget(TargetHandle t, Color c)
	{
		auto it = impl_->targets.find(t.id);
		if (it == impl_->targets.end())
			return;
		impl_->flush();
		const bool wasScissor = impl_->clipActive;
		if (wasScissor)
			glDisable(GL_SCISSOR_TEST);

		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
		glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLuint prevFbo = 0;
		if (impl_->insideTarget)
		{
			if (auto cur = impl_->targets.find(impl_->currentTarget.id); cur != impl_->targets.end())
				prevFbo = cur->second.fbo;
		}
		impl_->gl.BindFramebuffer(GL_FRAMEBUFFER, prevFbo);
		if (wasScissor)
			impl_->applyScissor(impl_->currentClip);
	}

	// ---- Resource creation helpers ----

	TextureHandle SDL3Renderer::createTextureRGBA(int width, int height, const unsigned char *rgbaPixels)
	{
		if (!impl_->ready || width <= 0 || height <= 0 || !rgbaPixels)
			return {};

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
		glBindTexture(GL_TEXTURE_2D, 0);

		const uint32_t id = impl_->nextId++;
		impl_->textures[id] = {tex, width, height};
		return {id, width, height};
	}

	void SDL3Renderer::destroyTexture(TextureHandle h)
	{
		if (auto it = impl_->textures.find(h.id); it != impl_->textures.end())
		{
			if (it->second.glId)
				glDeleteTextures(1, &it->second.glId);
			impl_->textures.erase(it);
		}
	}

	FontHandle SDL3Renderer::createFontFromTTF(const unsigned char *ttfData,
											   size_t ttfSize,
											   int baseSize,
											   const CodepointSet &codepoints)
	{
		if (!impl_->ready || !ttfData || ttfSize == 0 || baseSize <= 0)
			return {};

		stbtt_fontinfo info{};
		if (!stbtt_InitFont(&info, ttfData, stbtt_GetFontOffsetForIndex(ttfData, 0)))
			return {};

		std::vector<int> cps = codepoints.build();
		if (cps.empty())
		{
			for (int cp = 32; cp <= 126; ++cp)
				cps.push_back(cp);
		}
		std::sort(cps.begin(), cps.end());
		cps.erase(std::unique(cps.begin(), cps.end()), cps.end());

		// Group sorted codepoints into contiguous runs for stbtt_PackFontRanges.
		struct Run
		{
			int first;
			int count;
			size_t offset;
		};
		std::vector<Run> runs;
		for (size_t i = 0; i < cps.size();)
		{
			const int start = cps[i];
			size_t j = i + 1;
			while (j < cps.size() && cps[j] == cps[j - 1] + 1)
				++j;
			runs.push_back({start, static_cast<int>(j - i), i});
			i = j;
		}

		std::vector<stbtt_packedchar> packed(cps.size());
		std::vector<stbtt_pack_range> ranges(runs.size());
		for (size_t r = 0; r < runs.size(); ++r)
		{
			ranges[r].font_size = static_cast<float>(baseSize);
			ranges[r].first_unicode_codepoint_in_range = runs[r].first;
			ranges[r].array_of_unicode_codepoints = nullptr;
			ranges[r].num_chars = runs[r].count;
			ranges[r].chardata_for_range = packed.data() + runs[r].offset;
		}

		int atlasDim = 512;
		std::vector<unsigned char> alpha;
		bool packedOk = false;
		for (; atlasDim <= 4096; atlasDim *= 2)
		{
			alpha.assign(static_cast<size_t>(atlasDim) * static_cast<size_t>(atlasDim), 0);
			stbtt_pack_context pc{};
			if (!stbtt_PackBegin(&pc, alpha.data(), atlasDim, atlasDim, 0, 1, nullptr))
				continue;
			stbtt_PackSetOversampling(&pc, 1, 1);
			packedOk = stbtt_PackFontRanges(&pc, ttfData, 0, ranges.data(), static_cast<int>(ranges.size())) != 0;
			stbtt_PackEnd(&pc);
			if (packedOk)
				break;
		}

		if (!packedOk)
		{
			logError("SDL3Renderer", "", 0, 0, "Failed to pack font atlas up to 4096x4096");
			return {};
		}

		// Expand single-channel coverage to white RGBA so the standard shader tints text cleanly.
		std::vector<unsigned char> rgba(alpha.size() * 4);
		for (size_t i = 0; i < alpha.size(); ++i)
		{
			rgba[i * 4 + 0] = 255;
			rgba[i * 4 + 1] = 255;
			rgba[i * 4 + 2] = 255;
			rgba[i * 4 + 3] = alpha[i];
		}

		GLuint glTex = 0;
		glGenTextures(1, &glTex);
		glBindTexture(GL_TEXTURE_2D, glTex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasDim, atlasDim, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		int ascent = 0, descent = 0, lineGap = 0;
		stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
		const float vScale = stbtt_ScaleForPixelHeight(&info, static_cast<float>(baseSize));

		Impl::FontObj font;
		font.glTex = glTex;
		font.baseSize = static_cast<float>(baseSize);
		font.ascentPx = static_cast<float>(ascent) * vScale;
		font.glyphs.reserve(cps.size());

		const float invAtlas = 1.0f / static_cast<float>(atlasDim);
		for (size_t i = 0; i < cps.size(); ++i)
		{
			const stbtt_packedchar &pc = packed[i];
			Impl::GlyphInfo g;
			g.u0 = static_cast<float>(pc.x0) * invAtlas;
			g.v0 = static_cast<float>(pc.y0) * invAtlas;
			g.u1 = static_cast<float>(pc.x1) * invAtlas;
			g.v1 = static_cast<float>(pc.y1) * invAtlas;
			g.xoff = pc.xoff;
			g.yoff = pc.yoff;
			g.w = pc.xoff2 - pc.xoff;
			g.h = pc.yoff2 - pc.yoff;
			g.xadvance = pc.xadvance;
			font.glyphs[static_cast<uint32_t>(cps[i])] = g;
		}

		const uint32_t id = impl_->nextId++;
		impl_->fonts[id] = std::move(font);
		return {id};
	}

	void SDL3Renderer::destroyFont(FontHandle h)
	{
		if (auto it = impl_->fonts.find(h.id); it != impl_->fonts.end())
		{
			if (it->second.glTex)
				glDeleteTextures(1, &it->second.glTex);
			impl_->fonts.erase(it);
		}
	}

	EffectHandle SDL3Renderer::createEffectFromSource(const char *fsSource, const char *vsSource)
	{
		if (!impl_->ready || !fsSource)
			return {};
		Impl::ShaderObj sh = impl_->buildShaderProgram(vsSource ? vsSource : kDefaultVertexShader, fsSource);
		if (sh.program == 0)
			return {};
		const uint32_t id = impl_->nextId++;
		impl_->shaders[id] = std::move(sh);
		return {id};
	}

	void SDL3Renderer::destroyEffect(EffectHandle h)
	{
		if (auto it = impl_->shaders.find(h.id); it != impl_->shaders.end())
		{
			if (it->second.program)
				impl_->gl.DeleteProgram(it->second.program);
			impl_->shaders.erase(it);
		}
	}

	// =========================================================================
	// 3. SDL3AssetProvider (IAssetProvider)
	// =========================================================================

	SDL3AssetProvider::~SDL3AssetProvider()
	{
		for (const auto &[k, h] : fonts_)
			renderer_.destroyFont(h);
		for (const auto &[k, h] : textures_)
			renderer_.destroyTexture(h);
		for (const auto &[k, h] : effects_)
			renderer_.destroyEffect(h);
	}

	FontHandle SDL3AssetProvider::loadFont(std::string_view name, std::string_view path)
	{
		if (name.empty() || path.empty())
			return {};
		if (auto it = fonts_.find(name); it != fonts_.end())
			return it->second;

		std::vector<unsigned char> bytes;
		if (!readBinaryFile(std::string(path), bytes))
		{
			logError("Assets", "", 0, 0,
					 "failed to read font '" + std::string(name) + "' from '" + std::string(path) + "'");
			return {};
		}
		return loadFontFromMemory(name, bytes.data(), bytes.size());
	}

	FontHandle SDL3AssetProvider::loadFontFromMemory(std::string_view name, const void *data, size_t size)
	{
		if (name.empty() || !data || size == 0)
			return {};
		if (auto it = fonts_.find(name); it != fonts_.end())
			return it->second;

		const FontHandle h = renderer_.createFontFromTTF(
			static_cast<const unsigned char *>(data), size, defaultFontSize_, defaultCodepoints_);
		if (h.id == 0)
		{
			logError("Assets", "", 0, 0, "failed to bake font '" + std::string(name) + "'");
			return {};
		}
		fonts_[std::string(name)] = h;
		return h;
	}

	void SDL3AssetProvider::unloadFont(std::string_view name)
	{
		if (auto it = fonts_.find(name); it != fonts_.end())
		{
			renderer_.destroyFont(it->second);
			fonts_.erase(it);
		}
	}

	TextureHandle SDL3AssetProvider::loadTexture(std::string_view name, std::string_view path)
	{
		if (name.empty() || path.empty())
			return {};
		if (auto it = textures_.find(name); it != textures_.end())
			return it->second;

		int w = 0, h = 0, ch = 0;
		unsigned char *pixels = stbi_load(std::string(path).c_str(), &w, &h, &ch, 4);
		if (!pixels)
		{
			logError("Assets", "", 0, 0,
					 "failed to load texture '" + std::string(name) + "' from '" + std::string(path) + "'");
			return {};
		}
		const TextureHandle handle = renderer_.createTextureRGBA(w, h, pixels);
		stbi_image_free(pixels);
		if (!handle.valid())
			return {};

		textures_[std::string(name)] = handle;
		return handle;
	}

	void SDL3AssetProvider::unloadTexture(std::string_view name)
	{
		if (auto it = textures_.find(name); it != textures_.end())
		{
			renderer_.destroyTexture(it->second);
			textures_.erase(it);
		}
	}

	EffectHandle SDL3AssetProvider::loadEffect(std::string_view name,
											   std::string_view fsPath,
											   std::string_view vsPath)
	{
		if (name.empty() || fsPath.empty())
			return {};
		if (auto it = effects_.find(name); it != effects_.end())
			return it->second;

		std::string fsSrc;
		if (!readTextFile(std::string(fsPath), fsSrc))
		{
			logError("Assets", "", 0, 0,
					 "failed to read fragment shader '" + std::string(fsPath) + "'");
			return {};
		}

		std::string vsSrc;
		const char *vsPtr = nullptr;
		if (!vsPath.empty())
		{
			if (!readTextFile(std::string(vsPath), vsSrc))
			{
				logError("Assets", "", 0, 0,
						 "failed to read vertex shader '" + std::string(vsPath) + "'");
				return {};
			}
			vsPtr = vsSrc.c_str();
		}

		const EffectHandle h = renderer_.createEffectFromSource(fsSrc.c_str(), vsPtr);
		if (!h.valid())
			return {};

		effects_[std::string(name)] = h;
		return h;
	}

	void SDL3AssetProvider::unloadEffect(std::string_view name)
	{
		if (auto it = effects_.find(name); it != effects_.end())
		{
			renderer_.destroyEffect(it->second);
			effects_.erase(it);
		}
	}

	// =========================================================================
	// 4. SDL3Backend (IBackend)
	// =========================================================================

	bool SDL3Backend::init(const WindowConfig &config)
	{
		if (initialized_)
			return true;

		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			logError("SDL3Backend", "", 0, 0, std::string("SDL_Init failed: ") + SDL_GetError());
			return false;
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

		if (config.msaa)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		}

		SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
		if (config.resizable)
			flags |= SDL_WINDOW_RESIZABLE;

		window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
		if (!window_ && config.msaa)
		{
			// Retry without MSAA on drivers that reject 4x multisample pixel formats.
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
			window_ = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
		}
		if (!window_)
		{
			logError("SDL3Backend", "", 0, 0, std::string("SDL_CreateWindow failed: ") + SDL_GetError());
			SDL_Quit();
			return false;
		}

		if (config.minWidth > 0 && config.minHeight > 0)
			SDL_SetWindowMinimumSize(window_, config.minWidth, config.minHeight);

		glContext_ = SDL_GL_CreateContext(window_);
		if (!glContext_)
		{
			logError("SDL3Backend", "", 0, 0, std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
			SDL_DestroyWindow(window_);
			window_ = nullptr;
			SDL_Quit();
			return false;
		}

		SDL_GL_MakeCurrent(window_, glContext_);
		SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);

		platform_ = std::make_unique<SDL3Platform>(window_);
		renderer_ = std::make_unique<SDL3Renderer>(window_);
		if (!renderer_->isReady())
		{
			shutdown();
			return false;
		}
		assets_ = std::make_unique<SDL3AssetProvider>(*renderer_);

		lastTicksNs_ = SDL_GetTicksNS();
		deltaTime_ = 1.0f / 60.0f;
		running_ = true;
		initialized_ = true;

		bindToContext();
		return true;
	}

	void SDL3Backend::shutdown()
	{
		if (!initialized_ && !window_ && !glContext_)
			return;

		auto &ctx = UIContext::get();
		if (renderer_)
			ctx.targetPool.clear(*renderer_, &ctx.renderStats);

		unbindFromContext();
		assets_.reset();
		renderer_.reset();
		platform_.reset();

		if (glContext_)
		{
			SDL_GL_DestroyContext(glContext_);
			glContext_ = nullptr;
		}
		if (window_)
		{
			SDL_DestroyWindow(window_);
			window_ = nullptr;
		}
		SDL_Quit();
		running_ = false;
		initialized_ = false;
	}

	bool SDL3Backend::pollFrame()
	{
		if (!initialized_)
			return false;

		const uint64_t now = SDL_GetTicksNS();
		const double elapsed = static_cast<double>(now - lastTicksNs_) * 1e-9;
		lastTicksNs_ = now;
		deltaTime_ = static_cast<float>(std::clamp(elapsed, 0.0001, 0.1));

		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_EVENT_QUIT)
				running_ = false;
		}

		return running_;
	}

	void SDL3Backend::beginDraw(Color clearColor)
	{
		int fbW = 1, fbH = 1;
		if (window_)
			SDL_GetWindowSizeInPixels(window_, &fbW, &fbH);
		glViewport(0, 0, fbW, fbH);
		glDisable(GL_SCISSOR_TEST);
		glClearColor(clearColor.r / 255.0f, clearColor.g / 255.0f,
					 clearColor.b / 255.0f, clearColor.a / 255.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SDL3Backend::endDraw()
	{
		if (window_)
			SDL_GL_SwapWindow(window_);
	}
} // namespace ZenitUI