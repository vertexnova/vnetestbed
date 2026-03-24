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
# Sample 02_test_scene: camera switcher, lights, rotating cubes.
# Extends 01_test_events with a Scene panel (vnescene lights + IRenderDevice).
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

add_executable(sample_02_test_scene ${SOURCE_FILES} ${INCLUDE_FILES})

add_sample_platform_sources(sample_02_test_scene)

target_link_libraries(sample_02_test_scene
    PRIVATE
    vne::testbed
)

target_include_directories(sample_02_test_scene
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_02_test_scene
    PRIVATE
    "VNE_DEMO_ID=\"test_scene\""
)

set_target_properties(sample_02_test_scene PROPERTIES
    OUTPUT_NAME "sample_02_test_scene"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(MSVC)
    target_compile_options(sample_02_test_scene PRIVATE /W4)
else()
    target_compile_options(sample_02_test_scene PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# Post-build: copy shaders (OpenGL and OpenGL ES; runtime picks by backend)
# ==============================================================================

add_custom_command(TARGET sample_02_test_scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene_vert.glsl"
        "$<TARGET_FILE_DIR:sample_02_test_scene>/scene_vert.glsl"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene_frag.glsl"
        "$<TARGET_FILE_DIR:sample_02_test_scene>/scene_frag.glsl"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene_vert_es.glsl"
        "$<TARGET_FILE_DIR:sample_02_test_scene>/scene_vert_es.glsl"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene_frag_es.glsl"
        "$<TARGET_FILE_DIR:sample_02_test_scene>/scene_frag_es.glsl"
    COMMENT "Copying 02_test_scene shaders (GL + ES) to output directory"
)
