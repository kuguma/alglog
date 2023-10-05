#include "test_mylogger.h"
#include <alglog.h>
#include "test_mylogger.h"

void call_from_another_source(int i){
    auto logger = alglog::get_default_logger("main");
    logger->trace("hoge");
    LogDebugPrj1("call_from_another_source {}", i);
}