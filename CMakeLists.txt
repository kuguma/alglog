cmake_minimum_required(VERSION 3.15)

Include(FetchContent)

project(ALGLOG)

find_package(Threads REQUIRED)

add_executable(AlgLog
    
)

FetchContent_Declare(
    FMT
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG     8.0.1
)
FetchContent_MakeAvailable(FMT)

target_compile_features(AlgLog PUBLIC cxx_std_11)

target_compile_definitions(AlgLog PUBLIC
    FMT_HEADER_ONLY # fmtライブラリをヘッダオンリーで用いる
)

# add_subdirectory("...")

target_sources(AlgLog PRIVATE
    test/test.cpp
    test/test_multi_include.cpp
)

target_include_directories(AlgLog PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${FMT_SOURCE_DIR}/include # FetchContentでXXXを取得すると、XXX_SOURCE_DIRでソースディレクトリのパスが取れる
)

target_link_libraries(AlgLog PRIVATE
    Threads::Threads
)

# target_link_directories(AlgLog PUBLIC
#     ...
# )

# target_compile_options(AlgLog PUBLIC
#     ...
# )

