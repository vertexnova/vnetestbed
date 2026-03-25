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
# Sample 03_test_interaction: all manipulators, zoom methods, view presets.
# Mesh from testdata via vneio when VNE_TESTBED_HAVE_VNEIO. Only built when
# vne::interaction is available.
# ==============================================================================

if(NOT TARGET vne::scene OR NOT TARGET vne::interaction)
    return()
endif()

# ==============================================================================
# Source and Headers
# ==============================================================================

set(INCLUDE_FILES
    demo_test_interaction.h
)

set(SOURCE_FILES
    demo_test_interaction.cpp
)

# ==============================================================================
# Test Interaction Application
# ==============================================================================

add_executable(03_test_interaction ${SOURCE_FILES} ${INCLUDE_FILES})

add_sample_platform_sources(03_test_interaction)

target_link_libraries(03_test_interaction
    PRIVATE
    vne::glfw_sample_common
    vne::testbed
)

if(TARGET vne::io)
    target_link_libraries(03_test_interaction PRIVATE vne::io)
endif()

target_include_directories(03_test_interaction
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set(VNETESTBED_TESTDATA_ROOT "${CMAKE_SOURCE_DIR}/testdata" CACHE PATH "Testdata root for samples")

target_compile_definitions(03_test_interaction
    PRIVATE
    "VNE_DEMO_ID=\"test_interaction\""
    "VNETESTBED_TESTDATA_DIR=\"${VNETESTBED_TESTDATA_ROOT}\""
)

if(VNE_TESTBED_HAVE_VNEIO)
    target_compile_definitions(03_test_interaction PRIVATE VNE_TESTBED_VNEIO)
endif()

set_target_properties(03_test_interaction PROPERTIES
    OUTPUT_NAME "03_test_interaction"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(MSVC)
    target_compile_options(03_test_interaction PRIVATE /W4)
else()
    target_compile_options(03_test_interaction PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Mesh is drawn via testbed MeshRenderer (embedded Phong); no sample shaders needed.
