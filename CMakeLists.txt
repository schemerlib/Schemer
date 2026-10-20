cmake_minimum_required(VERSION 3.21)
project(Schemer VERSION 1.0.0 LANGUAGES CXX)

option(BUILD_TESTING "Build the testing tree for Schemer" ${PROJECT_IS_TOP_LEVEL})

add_library(Schemer INTERFACE)

target_include_directories(Schemer INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:.>
)

target_sources(Schemer INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/Schemer.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/Builder.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/Runner.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/FractionalScheme.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/Setup.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Schemer/Config.h>
)

target_compile_features(Schemer INTERFACE cxx_std_20)
add_library(Schemer::Schemer ALIAS Schemer)

include(FetchContent)
FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
)

set(_BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF)

FetchContent_MakeAvailable(eigen)

set(BUILD_TESTING ${_BUILD_TESTING_SAVED})

target_link_libraries(Schemer INTERFACE Eigen3::Eigen)

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif ()