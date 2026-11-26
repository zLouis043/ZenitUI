#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "SDL3Backend.hpp"
#include "Geometry2D.hpp"
#include "InputAccumulator.hpp"
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
#include <cstring>
#include <type_traits>

namespace ZenitUI
{
	// =========================================================================
	// Internal OpenGL 3.3 Loader, Math & File Helpers
	// =========================================================================
	namespace
	{
		constexpr size_t kMaxBatchVertices = 16384;
		constexpr size_t kMaxBatchIndices = 49152;
		constexpr size_t kVboRingVertices = 65536;
		constexpr size_t kEboRingIndices = 196608;

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
			PFNGLDRAWELEMENTSBASEVERTEXPROC DrawElementsBaseVertex{nullptr};

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
				ok &= loadOne(DrawElementsBaseVertex, "glDrawElementsBaseVertex");
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
		InputAccumulator input;
		bool watchInstalled{false};

		static bool SDLCALL onSdlEvent(void *userdata, SDL_Event *ev)
		{
			auto *self = static_cast<Impl *>(userdata);
			if (!self || !ev)
				return true;

			switch (ev->type)
			{
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			{
				const int idx = mapMouseButtonIndex(ev->button.button);
				if (idx >= 0)
					self->input.pushMouseButton(static_cast<MouseButton>(idx),
												ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
				break;
			}
			case SDL_EVENT_MOUSE_WHEEL:
				self->input.pushMouseWheel(ev->wheel.y);
				break;
			case SDL_EVENT_KEY_DOWN:
				self->input.pushKey(mapScancode(ev->key.scancode), true);
				break;
			case SDL_EVENT_KEY_UP:
				self->input.pushKey(mapScancode(ev->key.scancode), false);
				break;
			case SDL_EVENT_TEXT_INPUT:
				if (ev->text.text)
					self->input.pushTextUtf8(ev->text.text);
				break;
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

		InputAccumulator::ButtonDownArray liveDown{};
		liveDown[static_cast<size_t>(MouseButton::Left)] = (mask & SDL_BUTTON_LMASK) != 0;
		liveDown[static_cast<size_t>(MouseButton::Right)] = (mask & SDL_BUTTON_RMASK) != 0;
		liveDown[static_cast<size_t>(MouseButton::Middle)] = (mask & SDL_BUTTON_MMASK) != 0;

		return impl_->input.drainPointer({mx, my}, liveDown);
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
		std::vector<int> held;
		int numKeys = 0;
		const bool *kb = SDL_GetKeyboardState(&numKeys);
		if (kb && numKeys > 0)
		{
			for (const auto &entry : kScancodeMap)
			{
				if (entry.sdl < numKeys && kb[entry.sdl])
					held.push_back(entry.zenit);
			}
		}
		return impl_->input.drainEvents(std::move(held));
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
		static_assert(std::is_trivially_copyable_v<Vertex2D>,
					  "Vertex2D must be trivially copyable for fast batch upload");

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
			uint64_t uploadedProjVersion{0};
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
		GLuint ebo{0};
		GLuint whiteTex{0};
		ShaderObj defaultShader{};

		std::vector<Vertex2D> vertexBatch;
		std::vector<uint32_t> indexBatch;
		GLuint batchTex{0};
		size_t vboWriteOffset{0};
		size_t eboWriteOffset{0};

		// Cached GL state (invalidated at beginFrame).
		GLuint boundProgram{0};
		GLuint boundTexture{0};
		bool vaoBound{false};
		bool vboBound{false};
		bool activeTexUnit0{false};
		bool scissorEnabled{false};
		BlendMode blendMode{BlendMode::Alpha};
		int scissorBox[4]{0, 0, 0, 0};
		int cachedVpW{0};
		int cachedVpH{0};
		float cachedProjW{-1.0f};
		float cachedProjH{-1.0f};
		uint64_t projectionVersion{1};
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

			// Static defaults uploaded once at link time instead of on every flush.
			gl.UseProgram(prog);
			boundProgram = prog;
			if (obj.locTexture0 >= 0)
				gl.Uniform1i(obj.locTexture0, 0);
			if (obj.locColDiffuse >= 0)
			{
				constexpr float kWhite[4] = {1.0f, 1.0f, 1.0f, 1.0f};
				gl.Uniform4fv(obj.locColDiffuse, 1, kWhite);
			}
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

		ShaderObj &currentShaderObj()
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
			ShaderObj &sh = currentShaderObj();
			if (boundProgram != sh.program)
			{
				gl.UseProgram(sh.program);
				boundProgram = sh.program;
			}
			if (sh.uploadedProjVersion != projectionVersion)
			{
				if (sh.locProjection >= 0)
					gl.UniformMatrix4fv(sh.locProjection, 1, GL_FALSE, activeProjection);
				sh.uploadedProjVersion = projectionVersion;
			}
		}

		void updateProjectionAndViewport()
		{
			const int vpW = insideTarget ? currentTargetW : fbWidth;
			const int vpH = insideTarget ? currentTargetH : fbHeight;
			const float projW = insideTarget ? static_cast<float>(currentTargetW)
											 : (static_cast<float>(fbWidth) / dpiScale);
			const float projH = insideTarget ? static_cast<float>(currentTargetH)
											 : (static_cast<float>(fbHeight) / dpiScale);

			if (cachedVpW != vpW || cachedVpH != vpH)
			{
				glViewport(0, 0, vpW, vpH);
				cachedVpW = vpW;
				cachedVpH = vpH;
			}
			if (cachedProjW != projW || cachedProjH != projH)
			{
				makeOrthoTopDown(projW, projH, activeProjection);
				cachedProjW = projW;
				cachedProjH = projH;
				++projectionVersion;
			}
		}

		void flush()
		{
			if (indexBatch.empty() || vertexBatch.empty() || !ready)
				return;

			bindActiveProgramUniforms();

			const GLuint targetTex = batchTex ? batchTex : whiteTex;
			if (!activeTexUnit0)
			{
				gl.ActiveTexture(GL_TEXTURE0);
				activeTexUnit0 = true;
			}
			if (boundTexture != targetTex)
			{
				glBindTexture(GL_TEXTURE_2D, targetTex);
				boundTexture = targetTex;
			}

			if (!vaoBound)
			{
				gl.BindVertexArray(vao);
				vaoBound = true;
			}
			if (!vboBound)
			{
				gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
				vboBound = true;
			}

			const size_t vCount = vertexBatch.size();
			if (vboWriteOffset + vCount > kVboRingVertices)
			{
				gl.BufferData(GL_ARRAY_BUFFER,
							  static_cast<GLsizeiptr>(kVboRingVertices * sizeof(Vertex2D)),
							  nullptr,
							  GL_STREAM_DRAW);
				vboWriteOffset = 0;
			}
			gl.BufferSubData(GL_ARRAY_BUFFER,
							 static_cast<GLsizeiptr>(vboWriteOffset * sizeof(Vertex2D)),
							 static_cast<GLsizeiptr>(vCount * sizeof(Vertex2D)),
							 vertexBatch.data());

			const size_t iCount = indexBatch.size();
			if (eboWriteOffset + iCount > kEboRingIndices)
			{
				gl.BufferData(GL_ELEMENT_ARRAY_BUFFER,
							  static_cast<GLsizeiptr>(kEboRingIndices * sizeof(uint32_t)),
							  nullptr,
							  GL_STREAM_DRAW);
				eboWriteOffset = 0;
			}
			gl.BufferSubData(GL_ELEMENT_ARRAY_BUFFER,
							 static_cast<GLsizeiptr>(eboWriteOffset * sizeof(uint32_t)),
							 static_cast<GLsizeiptr>(iCount * sizeof(uint32_t)),
							 indexBatch.data());

			gl.DrawElementsBaseVertex(
				GL_TRIANGLES,
				static_cast<GLsizei>(iCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<const void *>(eboWriteOffset * sizeof(uint32_t)),
				static_cast<GLint>(vboWriteOffset));

			vboWriteOffset += vCount;
			eboWriteOffset += iCount;
			vertexBatch.clear();
			indexBatch.clear();
		}

		void ensureBatch(GLuint tex, size_t addVerts, size_t addIndices)
		{
			const GLuint resolved = tex ? tex : whiteTex;
			if (batchTex != resolved ||
				vertexBatch.size() + addVerts > kMaxBatchVertices ||
				indexBatch.size() + addIndices > kMaxBatchIndices)
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
			vertexBatch.push_back(Vertex2D{p, {u, v}, c});
		}

		void pushQuad(Rect dst, float u0, float v0, float u1, float v1, Color c, GLuint tex)
		{
			if (dst.width <= 0.0f || dst.height <= 0.0f || c.a == 0)
				return;
			ensureBatch(tex, 4, 6);

			const uint32_t base = static_cast<uint32_t>(vertexBatch.size());
			pushVertex({dst.x, dst.y}, u0, v0, c);
			pushVertex({dst.x + dst.width, dst.y}, u1, v0, c);
			pushVertex({dst.x + dst.width, dst.y + dst.height}, u1, v1, c);
			pushVertex({dst.x, dst.y + dst.height}, u0, v1, c);

			indexBatch.push_back(base + 0);
			indexBatch.push_back(base + 3);
			indexBatch.push_back(base + 2);
			indexBatch.push_back(base + 0);
			indexBatch.push_back(base + 2);
			indexBatch.push_back(base + 1);
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

		void disableScissor()
		{
			if (scissorEnabled)
			{
				glDisable(GL_SCISSOR_TEST);
				scissorEnabled = false;
			}
		}

		void applyScissor(const Rect &clip)
		{
			const float scale = insideTarget ? 1.0f : dpiScale;
			const int targetH = insideTarget ? currentTargetH : fbHeight;

			const int x = static_cast<int>(std::floor(clip.x * scale));
			const int y = static_cast<int>(std::floor(clip.y * scale));
			const int w = static_cast<int>(std::ceil(clip.width * scale));
			const int h = static_cast<int>(std::ceil(clip.height * scale));
			const int sx = (w <= 0 || h <= 0) ? 0 : x;
			const int sy = (w <= 0 || h <= 0) ? 0 : (targetH - (y + h));
			const int sw = (w <= 0 || h <= 0) ? 0 : w;
			const int sh = (w <= 0 || h <= 0) ? 0 : h;

			if (scissorEnabled &&
				scissorBox[0] == sx && scissorBox[1] == sy &&
				scissorBox[2] == sw && scissorBox[3] == sh)
				return;

			flush();
			if (!scissorEnabled)
			{
				glEnable(GL_SCISSOR_TEST);
				scissorEnabled = true;
			}
			glScissor(sx, sy, sw, sh);
			scissorBox[0] = sx;
			scissorBox[1] = sy;
			scissorBox[2] = sw;
			scissorBox[3] = sh;
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
		impl_->gl.GenBuffers(1, &impl_->ebo);

		impl_->gl.BindVertexArray(impl_->vao);
		impl_->gl.BindBuffer(GL_ARRAY_BUFFER, impl_->vbo);
		impl_->gl.BufferData(GL_ARRAY_BUFFER,
							 static_cast<GLsizeiptr>(kVboRingVertices * sizeof(Vertex2D)),
							 nullptr,
							 GL_STREAM_DRAW);

		impl_->gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl_->ebo);
		impl_->gl.BufferData(GL_ELEMENT_ARRAY_BUFFER,
							 static_cast<GLsizeiptr>(kEboRingIndices * sizeof(uint32_t)),
							 nullptr,
							 GL_STREAM_DRAW);

		impl_->gl.EnableVertexAttribArray(0);
		impl_->gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
									  reinterpret_cast<void *>(offsetof(Vertex2D, pos)));
		impl_->gl.EnableVertexAttribArray(1);
		impl_->gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
									  reinterpret_cast<void *>(offsetof(Vertex2D, uv)));
		impl_->gl.EnableVertexAttribArray(2);
		impl_->gl.VertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex2D),
									  reinterpret_cast<void *>(offsetof(Vertex2D, color)));
		impl_->gl.BindVertexArray(0);

		const uint8_t whitePixel[4] = {255, 255, 255, 255};
		glGenTextures(1, &impl_->whiteTex);
		glBindTexture(GL_TEXTURE_2D, impl_->whiteTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
		glBindTexture(GL_TEXTURE_2D, 0);

		impl_->vertexBatch.reserve(kMaxBatchVertices);
		impl_->indexBatch.reserve(kMaxBatchIndices);
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
		if (impl_->ebo)
			impl_->gl.DeleteBuffers(1, &impl_->ebo);
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

		// Reset cached GL state at frame boundary in case external code touched GL.
		impl_->boundProgram = 0;
		impl_->boundTexture = 0;
		impl_->vaoBound = false;
		impl_->vboBound = false;
		impl_->activeTexUnit0 = false;
		impl_->scissorEnabled = false;
		impl_->blendMode = BlendMode::Alpha;
		impl_->cachedVpW = 0;
		impl_->cachedVpH = 0;
		impl_->cachedProjW = -1.0f;
		impl_->cachedProjH = -1.0f;

		impl_->updateProjectionAndViewport();
	}

	void SDL3Renderer::endFrame()
	{
		if (!impl_->ready)
			return;
		impl_->flush();
		impl_->disableScissor();
		setBlendMode(BlendMode::Alpha);
	}

	// ---- 2D vector primitives ----

	void SDL3Renderer::drawGeometry(std::span<const Vertex2D> vertices,
									std::span<const uint32_t> indices,
									TextureHandle tex)
	{
		if (vertices.empty() || indices.size() < 3)
			return;

		GLuint glTex = impl_->whiteTex;
		if (tex.valid())
		{
			auto it = impl_->textures.find(tex.id);
			if (it == impl_->textures.end() || it->second.glId == 0 ||
				it->second.width <= 0 || it->second.height <= 0)
				return;
			glTex = it->second.glId;
		}

		const size_t totalIndices = indices.size() - (indices.size() % 3);

		if (vertices.size() <= kMaxBatchVertices && totalIndices <= kMaxBatchIndices)
		{
			impl_->ensureBatch(glTex, vertices.size(), totalIndices);
			const uint32_t base = static_cast<uint32_t>(impl_->vertexBatch.size());

			if (impl_->transformStack.empty())
			{
				const size_t oldSize = impl_->vertexBatch.size();
				impl_->vertexBatch.resize(oldSize + vertices.size());
				std::memcpy(impl_->vertexBatch.data() + oldSize,
							vertices.data(),
							vertices.size() * sizeof(Vertex2D));
			}
			else
			{
				for (const Vertex2D &v : vertices)
				{
					const Vec2 p = impl_->transformPoint(v.pos);
					impl_->vertexBatch.push_back(Vertex2D{p, v.uv, v.color});
				}
			}

			const uint32_t vCount = static_cast<uint32_t>(vertices.size());
			for (size_t i = 0; i < totalIndices; ++i)
			{
				const uint32_t idx = indices[i];
				impl_->indexBatch.push_back(base + (idx < vCount ? idx : 0u));
			}
			return;
		}

		// Fallback for oversized custom meshes exceeding single-batch limits.
		constexpr size_t kChunk = kMaxBatchVertices - (kMaxBatchVertices % 3);
		for (size_t offset = 0; offset < totalIndices; offset += kChunk)
		{
			const size_t count = std::min(kChunk, totalIndices - offset);
			impl_->ensureBatch(glTex, count, count);
			const uint32_t base = static_cast<uint32_t>(impl_->vertexBatch.size());

			for (size_t i = 0; i < count; ++i)
			{
				const uint32_t idx = indices[offset + i];
				const Vertex2D &v = (idx < vertices.size()) ? vertices[idx] : vertices[0];
				impl_->pushVertex(v.pos, v.uv.x, v.uv.y, v.color);
				impl_->indexBatch.push_back(base + static_cast<uint32_t>(i));
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
			const uint32_t cp = Utf8::next(s, i);
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
			const uint32_t cp = Utf8::next(s, i);
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
			impl_->disableScissor();
			return;
		}
		impl_->applyScissor(impl_->currentClip);
	}

	Rect SDL3Renderer::getClipRect() const
	{
		const Rect fbBounds{
			0.0f, 0.0f,
			static_cast<float>(impl_->fbWidth) / impl_->dpiScale,
			static_cast<float>(impl_->fbHeight) / impl_->dpiScale};
		if (impl_->insideTarget && !impl_->targetStates.empty())
		{
			const auto &root = impl_->targetStates.front();
			return root.clipActive ? root.currentClip : fbBounds;
		}
		return impl_->clipActive ? impl_->currentClip : fbBounds;
	}

	void SDL3Renderer::pushEffect(EffectHandle e)
	{
		if (impl_->shaders.find(e.id) == impl_->shaders.end())
			return;
		impl_->flush();
		impl_->effectStack.push_back(e);
	}

	void SDL3Renderer::popEffect()
	{
		if (impl_->effectStack.empty())
			return;
		impl_->flush();
		impl_->effectStack.pop_back();
	}

	void SDL3Renderer::setBlendMode(BlendMode mode)
	{
		if (!impl_->ready || impl_->blendMode == mode)
			return;
		impl_->flush();
		impl_->blendMode = mode;
		switch (mode)
		{
		case BlendMode::Alpha:
			glEnable(GL_BLEND);
			impl_->gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
										GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case BlendMode::Premultiplied:
			glEnable(GL_BLEND);
			impl_->gl.BlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
										GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case BlendMode::Replace:
			glDisable(GL_BLEND);
			break;
		}
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
		if (!impl_->effectStack.empty() && impl_->effectStack.back().id == e.id)
			impl_->flush();
		if (impl_->boundProgram != it->second.program)
		{
			impl_->gl.UseProgram(it->second.program);
			impl_->boundProgram = it->second.program;
		}
		impl_->gl.Uniform1f(loc, value);
	}

	void SDL3Renderer::setEffectVec2(EffectHandle e, const char *name, Vec2 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		if (!impl_->effectStack.empty() && impl_->effectStack.back().id == e.id)
			impl_->flush();
		const float arr[2] = {value.x, value.y};
		if (impl_->boundProgram != it->second.program)
		{
			impl_->gl.UseProgram(it->second.program);
			impl_->boundProgram = it->second.program;
		}
		impl_->gl.Uniform2fv(loc, 1, arr);
	}

	void SDL3Renderer::setEffectVec3(EffectHandle e, const char *name, Vec3 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		if (!impl_->effectStack.empty() && impl_->effectStack.back().id == e.id)
			impl_->flush();
		const float arr[3] = {value.x, value.y, value.z};
		if (impl_->boundProgram != it->second.program)
		{
			impl_->gl.UseProgram(it->second.program);
			impl_->boundProgram = it->second.program;
		}
		impl_->gl.Uniform3fv(loc, 1, arr);
	}

	void SDL3Renderer::setEffectVec4f(EffectHandle e, const char *name, Vec4 value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		if (!impl_->effectStack.empty() && impl_->effectStack.back().id == e.id)
			impl_->flush();
		const float arr[4] = {value.x, value.y, value.z, value.w};
		if (impl_->boundProgram != it->second.program)
		{
			impl_->gl.UseProgram(it->second.program);
			impl_->boundProgram = it->second.program;
		}
		impl_->gl.Uniform4fv(loc, 1, arr);
	}

	void SDL3Renderer::setEffectVec4(EffectHandle e, const char *name, Color value)
	{
		auto it = impl_->shaders.find(e.id);
		if (it == impl_->shaders.end())
			return;
		const GLint loc = impl_->uniformLoc(it->second, name);
		if (loc < 0)
			return;
		if (!impl_->effectStack.empty() && impl_->effectStack.back().id == e.id)
			impl_->flush();
		const float arr[4] = {
			value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
		if (impl_->boundProgram != it->second.program)
		{
			impl_->gl.UseProgram(it->second.program);
			impl_->boundProgram = it->second.program;
		}
		impl_->gl.Uniform4fv(loc, 1, arr);
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
		impl_->boundTexture = 0;

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
			if (impl_->batchTex == it->second.colorTex)
			{
				impl_->flush();
				impl_->batchTex = 0;
			}
			if (impl_->boundTexture == it->second.colorTex)
				impl_->boundTexture = 0;
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
		impl_->disableScissor();

		impl_->targetStates.push_back({impl_->currentTarget,
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
		impl_->disableScissor();

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
		impl_->disableScissor();

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
		impl_->boundTexture = 0;

		const uint32_t id = impl_->nextId++;
		impl_->textures[id] = {tex, width, height};
		return {id, width, height};
	}

	void SDL3Renderer::destroyTexture(TextureHandle h)
	{
		if (auto it = impl_->textures.find(h.id); it != impl_->textures.end())
		{
			if (impl_->batchTex == it->second.glId)
			{
				impl_->flush();
				impl_->batchTex = 0;
			}
			if (impl_->boundTexture == it->second.glId)
				impl_->boundTexture = 0;
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
		impl_->boundTexture = 0;

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
			const GLuint tex = it->second.glTex;
			if (impl_->batchTex == tex)
			{
				impl_->flush();
				impl_->batchTex = 0;
			}
			if (impl_->boundTexture == tex)
				impl_->boundTexture = 0;
			if (tex)
				glDeleteTextures(1, &tex);
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
			if (impl_->boundProgram == it->second.program)
				impl_->boundProgram = 0;
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