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
# OpenGL renderer + demo layers (compiled only when glad is available)
#
# VNE_TESTBED_OPENGL: ON  = compile gl/ primitives and demo layer plugins
#                     OFF = core-only library; examples requiring OpenGL are skipped
#==============================================================================
if(TARGET glad::glad)
    option(VNE_TESTBED_OPENGL "Build the OpenGL render adapter, debug draw, and demo layers" ON)
else()
    set(VNE_TESTBED_OPENGL OFF)
endif()

if(VNE_TESTBED_OPENGL)
    message(STATUS "VNE_TESTBED_OPENGL: ON — adding gl/ primitives and demo layers")

    target_compile_definitions(vnetestbed PUBLIC VNE_TESTBED_OPENGL)

    target_sources(vnetestbed PRIVATE
        # gl/ — RAII OpenGL primitives
        vertexnova/testbed/gl/shader.cpp
        vertexnova/testbed/gl/vertex_buffer.cpp
        vertexnova/testbed/gl/vertex_array.cpp
        vertexnova/testbed/gl/index_buffer.cpp
        vertexnova/testbed/gl/opengl_context.cpp
        vertexnova/testbed/gl/texture2d.cpp
        vertexnova/testbed/gl/sampler2d.cpp
        vertexnova/testbed/gl/draw.cpp
        vertexnova/testbed/gl/opengl_render_adapter.cpp
        vertexnova/testbed/gl/opengl_debug_draw.cpp
        # Demo layer plugins
        vertexnova/testbed/plugins/triangle_demo_layer.cpp
        vertexnova/testbed/plugins/scene_demo_layer.cpp
        vertexnova/testbed/plugins/events_demo_layer.cpp
    )
    if(TARGET vne::interaction)
        target_sources(vnetestbed PRIVATE vertexnova/testbed/plugins/interaction_demo_layer.cpp)
    endif()

    target_link_libraries(vnetestbed PUBLIC glad::glad)
else()
    message(STATUS "VNE_TESTBED_OPENGL: OFF — glad not found; skipping gl/ and demo layers")
endif()

target_compile_features(vnetestbed PUBLIC cxx_std_20)
