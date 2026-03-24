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
# Sample 01_test_events: event log + input polling panel.
# Extends 00_hello_testbed with a live Events panel in Settings.
# ==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

# ==============================================================================
# Source and Headers
# ==============================================================================

set(INCLUDE_FILES
    demo_test_events.h
)

set(SOURCE_FILES
    demo_test_events.cpp
)

# ==============================================================================
# Test Events Application
# ==============================================================================

add_executable(01_test_events ${SOURCE_FILES} ${INCLUDE_FILES})

add_sample_platform_sources(01_test_events)

target_link_libraries(01_test_events
    PRIVATE
    vne::glfw_sample_common
    vne::testbed
)

target_include_directories(01_test_events
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(01_test_events
    PRIVATE
    "VNE_DEMO_ID=\"test_events\""
)

set_target_properties(01_test_events PROPERTIES
    OUTPUT_NAME "01_test_events"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(MSVC)
    target_compile_options(01_test_events PRIVATE /W4)
else()
    target_compile_options(01_test_events PRIVATE -Wall -Wextra -Wpedantic)
endif()
