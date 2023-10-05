cmake_minimum_required(VERSION 3.15)

Include(FetchContent)

project(ALGLOG)

add_executable(AlgLog
    
)

FetchContent_Declare(
    FMT
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG     8.0.1
)
FetchContent_MakeAvailable(FMT)

target_compile_features(AlgLog PUBLIC cxx_std_17)

target_compile_definitions(AlgLog PUBLIC
    FMT_HEADER_ONLY # fmtライブラリをヘッダオンリーで用いる
)

# add_subdirectory("...")

target_sources(AlgLog PRIVATE
    test.cpp
)

target_include_directories(AlgLog PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# target_link_libraries(AlgLog PUBLIC
#     ...
# )

# target_link_directories(AlgLog PUBLIC
#     ...
# )

# target_compile_options(AlgLog PUBLIC
#     ...
# )

