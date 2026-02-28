#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

if(NOT TARGET vnetestbed)
    return()
endif()

#==============================================================================
# Setup Google Test
#==============================================================================
if(EXISTS "${PROJECT_SOURCE_DIR}/deps/external/googletest/CMakeLists.txt")
    message(STATUS "Using Google Test from deps/external/googletest (v1.17.0)")
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/external/googletest ${CMAKE_BINARY_DIR}/deps/external/googletest)
    include(GoogleTest)
    set(GTEST_LIBRARIES gtest_main gmock)
    set(GTEST_INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/deps/external/googletest/googletest/include
        ${PROJECT_SOURCE_DIR}/deps/external/googletest/googlemock/include
    )
else()
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Using system Google Test")
        set(GTEST_LIBRARIES GTest::gtest_main GTest::gmock)
        set(GTEST_INCLUDE_DIRS "")
    else()
        message(STATUS "deps/external/googletest not found, using FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.17.0
        )
        FetchContent_MakeAvailable(googletest)
        set(GTEST_LIBRARIES gtest_main gmock)
        set(GTEST_INCLUDE_DIRS ${googletest_SOURCE_DIR}/googletest/include ${googletest_SOURCE_DIR}/googlemock/include)
    endif()
endif()

#==============================================================================
# Test sources — one file per logical unit
#   recording_layer.h   shared header-only test helper (no .cpp)
#   smoke_test.cpp      basic sanity check
#   layer_test.cpp      ILayer state machine (standalone, no LayerStack)
#   layer_stack_test.cpp    LayerStack lifecycle, dispatch, ordering
#   plugin_registry_test.cpp    PluginRegistry registration and layer creation
#   scene_inspector_test.cpp    SceneInspectorLayer and SceneInspectorPlugin
#   context_test.cpp    AppContext, RenderContext, FrameInfo, interface traits
#==============================================================================
set(TEST_SOURCES
    smoke_test.cpp
    layer_test.cpp
    layer_stack_test.cpp
    plugin_registry_test.cpp
    scene_inspector_test.cpp
    context_test.cpp
)

#==============================================================================
# Build test executable
#==============================================================================
add_executable(vnetestbed_tests ${TEST_SOURCES})

target_include_directories(vnetestbed_tests
    PRIVATE
        ${GTEST_INCLUDE_DIRS}
        ${VNE_INCLUDE_DIR}
        ${VNE_SRC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}   # exposes recording_layer.h to all test TUs and IDE
)

target_link_libraries(vnetestbed_tests
    PRIVATE
        ${GTEST_LIBRARIES}
        vne::testbed
)

add_test(NAME vnetestbed_tests COMMAND vnetestbed_tests)
