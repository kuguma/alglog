cmake_minimum_required(VERSION 3.15)
project(AlgLogUnity)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Unity plugin output directory
if(UNIX AND NOT APPLE)
    set(UNITY_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Plugins/alglog/x86_64")
elseif(WIN32)
    set(UNITY_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Plugins/alglog/x86_64")
elseif(APPLE)
    set(UNITY_PLUGIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Plugins/alglog/macOS")
endif()

# Create Unity plugin
add_library(AlgLogUnity SHARED
    src/alglog_unity.cpp
)

target_include_directories(AlgLogUnity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Set output directory
set_target_properties(AlgLogUnity PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${UNITY_PLUGIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${UNITY_PLUGIN_DIR}"
)

# Link with required libraries
target_link_libraries(AlgLogUnity PRIVATE
    Threads::Threads
)
