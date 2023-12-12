cmake_minimum_required(VERSION 3.15)

project(ALGLOG)

option(ALGLOG_BUILD_TESTS "Build the test programs" OFF)

find_package(Threads REQUIRED)

Include(FetchContent)
FetchContent_Declare(
    FMT
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG     10.1.1
)
FetchContent_MakeAvailable(FMT)

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
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
    )

    target_include_directories(AlgLog PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${FMT_SOURCE_DIR}/include # FetchContentでXXXを取得すると、XXX_SOURCE_DIRでソースディレクトリのパスが取れる
    )
    
    target_link_libraries(AlgLog PRIVATE
        Threads::Threads
    )
endif()


# add_subdirectory("...")

# target_link_directories(AlgLog PUBLIC
#     ...
# )

# target_compile_options(AlgLog PUBLIC
#     ...
# )

