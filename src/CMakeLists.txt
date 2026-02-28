#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

set(SOURCE_FILES
    vertexnova/testbed/plugin_registry.cpp
    vertexnova/testbed/layer.cpp
    vertexnova/testbed/layer_stack.cpp
    vertexnova/testbed/plugins/scene_inspector_plugin.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/layer.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/layer_stack.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/render_context.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/app_context.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/plugin.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/plugin_registry.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/render_adapter.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/render_device.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/debug_draw.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/window.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/plugins/scene_inspector_plugin.h
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/plugins/scene_inspector_layer.h
)

add_library(vnetestbed STATIC ${SOURCE_FILES} ${HEADER_FILES})
add_library(vne::testbed ALIAS vnetestbed)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})

target_include_directories(vnetestbed
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vnetestbed
    PUBLIC
        vne::testbed::Warnings
        vne::testbed::BuildSettings
)

if(TARGET vne::common)
    target_link_libraries(vnetestbed PUBLIC vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vnetestbed PUBLIC vne::logging)
endif()
if(TARGET glfw)
    target_link_libraries(vnetestbed PUBLIC glfw)
endif()
if(TARGET vne::events)
    target_link_libraries(vnetestbed PUBLIC vne::events)
endif()
# vne::math is required: debug_draw.h uses vne::math::Vec3f
if(NOT TARGET vne::math)
    message(FATAL_ERROR "vnetestbed requires vne::math. Add vne::math before vnetestbed.")
endif()
target_link_libraries(vnetestbed PUBLIC vne::math)
if(TARGET vne::scene)
    target_link_libraries(vnetestbed PUBLIC vne::scene)
endif()
if(TARGET vne::interaction)
    target_link_libraries(vnetestbed PUBLIC vne::interaction)
endif()

#==============================================================================
# OpenGL renderer + demo layers (compiled only when glad and glfw are available)
#
# The gl/ backend (opengl_context, opengl_render_adapter) uses GLFW APIs;
# both glad and glfw must be present or the OpenGL sources are not added.
# VNE_TESTBED_OPENGL and VNE_TESTBED_OPENGLES are mutually exclusive.
# Default is chosen from VNE_TARGET_PLATFORM: desktop -> OpenGL 4.1; mobile/Web -> ES 3.0.
# Override with -DVNE_TESTBED_OPENGL=ON or -DVNE_TESTBED_OPENGLES=ON.
#==============================================================================
if(TARGET glad::glad AND TARGET glfw)
    if(VNE_TARGET_PLATFORM STREQUAL "iOS" OR VNE_TARGET_PLATFORM STREQUAL "Android"
       OR VNE_TARGET_PLATFORM STREQUAL "visionOS" OR VNE_TARGET_PLATFORM STREQUAL "Web")
        set(_vne_testbed_gl_default_es ON)
    else()
        set(_vne_testbed_gl_default_es OFF)
    endif()
    if(_vne_testbed_gl_default_es)
        set(_vne_testbed_gl_default_opengl OFF)
    else()
        set(_vne_testbed_gl_default_opengl ON)
    endif()
    option(VNE_TESTBED_OPENGL "Build the OpenGL 4.1 render adapter, debug draw, and demo layers" ${_vne_testbed_gl_default_opengl})
    option(VNE_TESTBED_OPENGLES "Build OpenGL ES 3.0 backend" ${_vne_testbed_gl_default_es})
else()
    set(VNE_TESTBED_OPENGL OFF CACHE BOOL "Build OpenGL 4.1 backend (requires glad and glfw)" FORCE)
    set(VNE_TESTBED_OPENGLES OFF CACHE BOOL "Build OpenGL ES 3.0 backend (requires glad and glfw)" FORCE)
    if(TARGET glad::glad AND NOT TARGET glfw)
        message(STATUS "VNE_TESTBED_OPENGL/OPENGLES: OFF — GLFW required for OpenGL backend (glad found, glfw missing)")
    else()
        message(STATUS "VNE_TESTBED_OPENGL / VNE_TESTBED_OPENGLES: OFF — glad or glfw not found; skipping gl/ and demo layers")
    endif()
endif()

if(VNE_TESTBED_OPENGLES)
    set(VNE_TESTBED_OPENGL OFF)
endif()

if(VNE_TESTBED_OPENGL OR VNE_TESTBED_OPENGLES)
    if(VNE_TESTBED_OPENGL)
        message(STATUS "VNE_TESTBED_OPENGL: ON — adding gl/ primitives and demo layers")
        target_compile_definitions(vnetestbed PUBLIC VNE_TESTBED_OPENGL)
    else()
        message(STATUS "VNE_TESTBED_OPENGLES: ON — adding gl/ primitives and demo layers (ES 3.0)")
        target_compile_definitions(vnetestbed PUBLIC VNE_TESTBED_OPENGLES)
    endif()

    target_sources(vnetestbed PRIVATE
        # gl/ — RAII OpenGL primitives (implementation details)
        vertexnova/testbed/gl/shader.cpp
        vertexnova/testbed/gl/vertex_buffer.cpp
        vertexnova/testbed/gl/vertex_array.cpp
        vertexnova/testbed/gl/index_buffer.cpp
        vertexnova/testbed/gl/opengl_context.cpp
        vertexnova/testbed/gl/texture2d.cpp
        vertexnova/testbed/gl/sampler2d.cpp
        vertexnova/testbed/gl/framebuffer.cpp
        vertexnova/testbed/gl/draw.cpp
        vertexnova/testbed/gl/opengl_render_adapter.cpp
        vertexnova/testbed/gl/opengl_render_device.cpp
        vertexnova/testbed/gl/opengl_debug_draw.cpp
        # Demo layer plugins (use only IRenderDevice / IDebugDraw — no gl/ includes)
        vertexnova/testbed/plugins/triangle_demo_layer.cpp
        vertexnova/testbed/plugins/scene_demo_layer.cpp
        vertexnova/testbed/plugins/events_demo_layer.cpp
    )
    if(TARGET vne::interaction)
        target_compile_definitions(vnetestbed PUBLIC VNE_TESTBED_INTERACTION)
        target_sources(vnetestbed PRIVATE vertexnova/testbed/plugins/interaction_demo_layer.cpp)
        message(STATUS "VNE_TESTBED_INTERACTION: ON — building InteractionDemoLayer")
    endif()

    target_link_libraries(vnetestbed PUBLIC glad::glad)
else()
    message(STATUS "VNE_TESTBED_OPENGL / VNE_TESTBED_OPENGLES: OFF — skipping gl/ and demo layers")
endif()

target_compile_features(vnetestbed PUBLIC cxx_std_20)
