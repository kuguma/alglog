cmake_minimum_required(VERSION 3.15)
project(alglog_tests)

add_executable(AlglogTest)

target_sources(AlglogTest PRIVATE
    test.cpp
    test_multi_include.cpp
)

target_compile_features(AlglogTest PUBLIC cxx_std_14)
target_compile_definitions(AlglogTest PUBLIC
    FMT_HEADER_ONLY # fmtライブラリをヘッダオンリーで用いる
)
target_compile_options(AlglogTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd"4819" /wd"4100">
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror -Wno-unused-parameter>
)

target_link_libraries(AlglogTest PRIVATE
    alglog::alglog
    Threads::Threads
)