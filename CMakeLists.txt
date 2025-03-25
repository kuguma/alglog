cmake_minimum_required(VERSION 3.15)

project(alglog
    VERSION 0.3.0
    DESCRIPTION "A single header logging library for comerical use project"
    HOMEPAGE_URL "https://github.com/kuguma/alglog"
    LANGUAGES CXX
)

option(ALGLOG_BUILD_TESTS "Build the test programs" OFF)

add_library(alglog INTERFACE)
add_library(alglog::alglog ALIAS alglog)

target_compile_features(alglog INTERFACE cxx_std_14)
target_include_directories(alglog INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

find_package(Threads REQUIRED)

Include(FetchContent)
FetchContent_Declare(
    FMT
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG     10.1.1
)
FetchContent_MakeAvailable(FMT)

target_link_libraries(alglog INTERFACE fmt::fmt)

if (ALGLOG_BUILD_TESTS)
    add_executable(AlgLog)

    target_sources(AlgLog PRIVATE
        test/test.cpp
        test/test_multi_include.cpp
    )
    
    target_compile_features(AlgLog PUBLIC cxx_std_14)
    target_compile_definitions(AlgLog PUBLIC
        FMT_HEADER_ONLY # fmtライブラリをヘッダオンリーで用いる
    )
    target_compile_options( AlgLog PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd"4819" /wd"4100">
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -Wno-unused-parameter>
    )

    target_include_directories(AlgLog PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${FMT_SOURCE_DIR}/include # FetchContentでXXXを取得すると、XXX_SOURCE_DIRでソースディレクトリのパスが取れる
    )
    
    target_link_libraries(AlgLog PRIVATE
        Threads::Threads
    )
endif()
