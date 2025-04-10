#pragma once

#define ALGLOG_DIRECT_INCLUDE_GUARD
#include <alglog.h>

/*
    alglogでは、複数プロジェクトで採用された場合の設定の衝突を避けるため、
    グローバルロガーを配置しない代わりに、プロジェクトごとにロガーを作成することを推奨しています。
    このファイルをベースにプロジェクトロガーを作成し、alglog.hの代わりにそれをincludeするようにしてください。
    - my_project
    - MyLog
    をプロジェクトに合うように一括置換してください。
*/

#ifndef ALGLOG_PURGE

namespace my_project{

    class Logger {
    private:
        Logger() : logger(std::make_shared<alglog::logger>(true)), flusher(std::make_unique<alglog::flusher>(logger))
        {
            // modify this
            logger->connect_sink( std::make_shared<alglog::builtin::color_print_sink>() );
            logger->connect_sink( std::make_shared<alglog::builtin::file_sink>("my_project.log") );
            flusher->start();
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        std::unique_ptr<alglog::flusher> flusher;
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

#define MyLogError(...) my_project::Logger::get().logger->error(__VA_ARGS__)
#define MyLogAlert(...) my_project::Logger::get().logger->alert(__VA_ARGS__)
#define MyLogInfo(...) my_project::Logger::get().logger->info(__VA_ARGS__)
#define MyLogCritical(...) my_project::Logger::get().logger->critical(ALGLOG_SR, __VA_ARGS__)
#define MyLogWarn(...) my_project::Logger::get().logger->warn(ALGLOG_SR, __VA_ARGS__)
#define MyLogDebug(...) my_project::Logger::get().logger->debug(ALGLOG_SR, __VA_ARGS__)
#define MyLogTrace(...) my_project::Logger::get().logger->trace(ALGLOG_SR, __VA_ARGS__)

#define MyTimeCount(title) alglog::time_counter _tc(teleaudio::Logger::get().logger, title)
#define MyTimeCountLevel(title, level) alglog::time_counter _tc(teleaudio::Logger::get().logger, title, level)

#else

#define MyLogError(...) ((void)0)
#define MyLogAlert(...) ((void)0)
#define MyLogInfo(...) ((void)0)
#define MyLogCritical(...) ((void)0)
#define MyLogWarn(...) ((void)0)
#define MyLogDebug(...) ((void)0)
#define MyLogTrace(...) ((void)0)

#define MyTimeCount(title) ((void)0)
#define MyTimeCountLevel(title, level) ((void)0)

#endif
