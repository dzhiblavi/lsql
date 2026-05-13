cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(
    logsql
    VERSION 0.0.1
    LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)

# Global settings
include(./cmake/Ccache.cmake)
include(./cmake/CheckCompiler.cmake)
include(./cmake/Development.cmake)
include(./cmake/Sanitizers.cmake)
include(./cmake/Flink.cmake)
include(./cmake/IPO.cmake)

# Third-party libraries
include(./cmake/thirdparty/re-flex.cmake)
include(./cmake/thirdparty/libcoro.cmake)
include(./cmake/thirdparty/llog-spdlog.cmake)

# Enable warnings for core of the project
include(./cmake/CompileOptions.cmake)
include(./cmake/Werror.cmake)

include(./cmake/Testing.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(tests)
