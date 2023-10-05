#pragma once

#include "alglog.h"

namespace prj1{

    class Logger {
    private:
        Logger(){
            logger = alglog::get_default_logger("prj1");
            auto snk = std::make_shared<alglog::builtin::file_sink>("test_prj1.log");
            logger->connect_sink(snk);
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& inst() {
            static Logger instance;
            return instance;
        }
    };
}

namespace prj2{

    class Logger {
    private:
        Logger(){
            logger = alglog::get_default_logger("prj2");
            auto snk = std::make_shared<alglog::builtin::file_sink>("test_prj2.log");
            logger->connect_sink(snk);
        };
        ~Logger() = default;

    public:
        std::shared_ptr<alglog::logger> logger;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& inst() {
            static Logger instance;
            return instance;
        }
    };

}


#define LogDebugPrj1(...) prj1::Logger::inst().logger->debug(__VA_ARGS__)
#define LogDebugPrj2(...) prj2::Logger::inst().logger->debug(__VA_ARGS__)

