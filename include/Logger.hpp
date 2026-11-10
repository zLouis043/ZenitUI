#pragma once

/**
 * \file Logger.hpp
 * \brief Lightweight, replaceable logging facility used across ZenitUI.
 *
 * The logger is intentionally minimal: a single global `ILogger` sink that
 * receives `LogMessage` records. Applications can replace the default
 * `ConsoleLogger` with their own implementation by calling `Logger::set`.
 *
 * Free functions `logDebug`, `logInfo`, `logWarn`, `logError` are the intended
 * entry points. They take the source location as explicit parameters so the
 * macros that use them can inject `__FILE__`, `__LINE__`, etc. without
 * polluting the call site.
 */

#include "Common.hpp"
#include <cstdio>

namespace ZenitUI {

    /** \brief Severity level attached to a `LogMessage`. */
    enum class LogLevel { Debug, Info, Warning, Error };

    /** \brief A single log record passed to `ILogger::log`. */
    struct LogMessage {
        LogLevel    level{ LogLevel::Info };  ///< Severity.
        std::string category;                 ///< Subsystem tag, e.g. "ZMarkup".
        std::string file;                     ///< Source file (may be empty).
        int         line{ 0 };                ///< Source line (0 if unknown).
        int         column{ 0 };              ///< Source column (0 if unknown).
        std::string text;                     ///< The message body.
    };

    /** \brief Abstract sink for `LogMessage` records. */
    class ILogger {
    public:
        virtual ~ILogger() = default;
        virtual void log(const LogMessage& msg) = 0;
    };

    /** \brief Default `ILogger` that prints to `stderr`. */
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

    /** \brief Global registry for the active logger. */
    class Logger {
    public:
        /** \brief Return the active logger. Never null: falls back to the default console logger. */
        static ILogger& get() { return *instance(); }

        /**
         * \brief Install \p logger as the active logger.
         * \param logger Sink to use; passing `nullptr` restores the default console logger.
         */
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

    /** \brief Internal dispatch: build a `LogMessage` and forward it to the active logger. */
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

    // ---- Convenience entry points ----

    inline void logDebug(const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Debug,   c, f, l, col, t); }
    inline void logInfo (const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Info,    c, f, l, col, t); }
    inline void logWarn (const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Warning, c, f, l, col, t); }
    inline void logError(const std::string& c, const std::string& f, int l, int col, const std::string& t) { logImpl(LogLevel::Error,   c, f, l, col, t); }

} // namespace ZenitUI