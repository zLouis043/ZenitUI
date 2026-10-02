#pragma once

#include "Common.hpp"
#include <cstdio>

namespace ZenitUI {

	enum class LogLevel { Debug, Info, Warning, Error };

	struct LogMessage {
		LogLevel    level{ LogLevel::Info };
		std::string category;   // "StyleParser", "Layout", ...
		std::string file;       // file sorgente (es. "assets/game.zstyle")
		int         line{ 0 };  // 0 = non disponibile
		int         column{ 0 };
		std::string text;
	};

	class ILogger {
	public:
		virtual ~ILogger() = default;
		virtual void log(const LogMessage& msg) = 0;
	};

	class ConsoleLogger : public ILogger {
	public:
		void log(const LogMessage& msg) override {
			const char* lvl = "INFO ";
			switch (msg.level) {
			case LogLevel::Debug:   lvl = "DEBUG"; break;
			case LogLevel::Info:    lvl = "INFO "; break;
			case LogLevel::Warning: lvl = "WARN "; break;
			case LogLevel::Error:   lvl = "ERROR"; break;
			}

			std::fprintf(stderr, "[%s] [%s] ", lvl, msg.category.c_str());
			if (!msg.file.empty()) {
				if (msg.line > 0)
					std::fprintf(stderr, "%s(%d:%d): ", msg.file.c_str(), msg.line, msg.column);
				else
					std::fprintf(stderr, "%s: ", msg.file.c_str());
			}
			std::fprintf(stderr, "%s\n", msg.text.c_str());
		}
	};

	class Logger {
	public:
		// Ritorna il logger corrente. Default: ConsoleLogger su stderr.
		static ILogger& get() { return *instance(); }

		// Imposta un logger custom. Passare nullptr ripristina il default.
		// L'utente è responsabile del lifetime del logger custom.
		static void set(ILogger* logger) {
			instance() = logger ? logger : defaultLogger();
		}

	private:
		static ConsoleLogger* defaultLogger() {
			static ConsoleLogger l;
			return &l;
		}
		static ILogger*& instance() {
			static ILogger* current = defaultLogger();
			return current;
		}
	};

	inline void logImpl(LogLevel level, const std::string& cat,
	                    const std::string& file, int line, int col,
	                    const std::string& text) {
		LogMessage m;
		m.level    = level;
		m.category = cat;
		m.file     = file;
		m.line     = line;
		m.column   = col;
		m.text     = text;
		Logger::get().log(m);
	}

	inline void logDebug(const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Debug,   c, f, l, col, t); }
	inline void logInfo (const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Info,    c, f, l, col, t); }
	inline void logWarn (const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Warning, c, f, l, col, t); }
	inline void logError(const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Error,   c, f, l, col, t); }

} // namespace ZenitUI