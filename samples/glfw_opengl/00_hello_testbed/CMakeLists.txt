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
# Sample 00_hello_testbed: grid + axes + Stats panel.
# Baseline demo — proves FBO viewport, orbit camera, ImGui Settings panel.
# ==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

# ==============================================================================
# Source and Headers
# ==============================================================================

set(INCLUDE_FILES
    demo_hello_testbed.h
)

set(SOURCE_FILES
    demo_hello_testbed.cpp
)

# ==============================================================================
# Hello Testbed Application
# ==============================================================================

add_executable(00_hello_testbed ${SOURCE_FILES} ${INCLUDE_FILES})

add_sample_platform_sources(00_hello_testbed)

target_link_libraries(00_hello_testbed
    PRIVATE
    vne::testbed
)

target_include_directories(00_hello_testbed
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(00_hello_testbed
    PRIVATE
    "VNE_DEMO_ID=\"hello_testbed\""
)

set_target_properties(00_hello_testbed PROPERTIES
    OUTPUT_NAME "00_hello_testbed"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(MSVC)
    target_compile_options(00_hello_testbed PRIVATE /W4)
else()
    target_compile_options(00_hello_testbed PRIVATE -Wall -Wextra -Wpedantic)
endif()
