#pragma once

#include <alglog.h>

namespace my_project{

    class Logger {
    private:
        Logger() : logger(std::make_unique<alglog::logger>())
        {
            // modify this
            logger->connect_sink( std::make_shared<alglog::builtin::print_sink>() );
            logger->connect_sink( std::make_shared<alglog::builtin::file_sink>("my_project.log") );
            logger->flush_every(std::chrono::milliseconds(500));
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& get() {
            static Logger instance;
            return instance;
        }
    };

}

#define MyLogError(...) AlgLogError(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogAlart(...) AlgLogAlart(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogInfo(...) AlgLogInfo(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogCritical(...) AlgLogCritical(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogWarn(...) AlgLogWarn(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogDebug(...) AlgLogDebug(my_project::Logger::get().logger->, __VA_ARGS__)
#define MyLogTrace(...) AlgLogTrace(my_project::Logger::get().logger->, __VA_ARGS__)
