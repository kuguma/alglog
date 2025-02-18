#include <alglog.h>
#include <string>

#ifdef _MSC_VER
    #define EXPORT_API extern "C" __declspec(dllexport)
#else
    #define EXPORT_API extern "C" __attribute__((visibility("default")))
#endif

EXPORT_API void* CreateLogger(bool async_mode) {
    return new alglog::logger(async_mode);
}

EXPORT_API void DestroyLogger(void* logger_ptr) {
    if (logger_ptr) {
        delete static_cast<alglog::logger*>(logger_ptr);
    }
}

EXPORT_API void LogMessage(void* logger_ptr, int level, const char* message) {
    if (!logger_ptr) return;
    
    auto logger = static_cast<alglog::logger*>(logger_ptr);
    switch (level) {
        case 0: logger->error(message); break;
        case 1: logger->alert(message); break;
        case 2: logger->info(message); break;
        case 3: logger->critical(message); break;
        case 4: logger->warn(message); break;
        case 5: logger->debug(message); break;
        case 6: logger->trace(message); break;
    }
}

EXPORT_API void FlushLogger(void* logger_ptr) {
    if (!logger_ptr) return;
    static_cast<alglog::logger*>(logger_ptr)->flush();
}
