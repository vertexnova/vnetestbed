# ==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   March 2026
#
# AutoDoc:   Yes
# ==============================================================================

# ==============================================================================
# Sample 02_test_scene: camera switcher, lights, rotating cubes (MeshRenderer).
# Scene demo: vnescene cameras/lights + shared testbed mesh rendering (embedded Phong).
# ==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

# ==============================================================================
# Source and Headers
# ==============================================================================

set(INCLUDE_FILES
    demo_test_scene.h
)

set(SOURCE_FILES
    demo_test_scene.cpp
)

# ==============================================================================
# Test Scene Application
# ==============================================================================

add_executable(02_test_scene ${SOURCE_FILES} ${INCLUDE_FILES})

add_sample_platform_sources(02_test_scene)

target_link_libraries(02_test_scene
    PRIVATE
    vne::glfw_sample_common
    vne::testbed
)

target_include_directories(02_test_scene
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(02_test_scene
    PRIVATE
    "VNE_DEMO_ID=\"test_scene\""
)

set_target_properties(02_test_scene PROPERTIES
    OUTPUT_NAME "02_test_scene"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(MSVC)
    target_compile_options(02_test_scene PRIVATE /W4)
else()
    target_compile_options(02_test_scene PRIVATE -Wall -Wextra -Wpedantic)
endif()
