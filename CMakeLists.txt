cmake_minimum_required(VERSION 3.15)

project(alglog
    VERSION 0.3.0
    DESCRIPTION "A single header logging library for comerical use project"
    HOMEPAGE_URL "https://github.com/kuguma/alglog"
    LANGUAGES CXX
)

option(ALGLOG_BUILD_TESTS "Build the test programs" OFF)
option(ALGLOG_DEFAULT_LOG_SWITCH "Enable default log switch" ON) # デフォルトではリリースでERROR,ALERT,INFOが残る
option(ALGLOG_GETPID "Enable process ID retrieval" ON)
option(ALGLOG_GETTID "Enable thread ID retrieval" ON)
option(ALGLOG_AUTO_THREAD_PRIORITY "Enable automatic thread priority adjustment for flusher thread" ON)

add_library(alglog INTERFACE)
add_library(alglog::alglog ALIAS alglog)

target_compile_features(alglog INTERFACE cxx_std_14)
target_include_directories(alglog INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(Threads REQUIRED)

Include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG     10.1.1
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(alglog INTERFACE fmt::fmt)

if (ALGLOG_BUILD_TESTS)
    add_subdirectory(test)
endif()

target_compile_definitions(alglog INTERFACE
    $<$<BOOL:${ALGLOG_DEFAULT_LOG_SWITCH}>:ALGLOG_DEFAULT_LOG_SWITCH>
    $<$<BOOL:${ALGLOG_GETPID}>:ALGLOG_GETPID>
    $<$<BOOL:${ALGLOG_GETTID}>:ALGLOG_GETTID>
    $<$<BOOL:${ALGLOG_AUTO_THREAD_PRIORITY}>:ALGLOG_AUTO_THREAD_PRIORITY>
)
