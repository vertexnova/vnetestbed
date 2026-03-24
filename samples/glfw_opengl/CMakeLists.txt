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
# GLFW + OpenGL sample set
# ==============================================================================
# Each sample: one executable, shared main, one demo registration.

if(NOT TARGET vne::testbed OR NOT TARGET glfw OR NOT TARGET glad::glad)
    return()
endif()
if(NOT VNE_TESTBED_OPENGL AND NOT VNE_TESTBED_OPENGLES)
    return()
endif()

# ==============================================================================
# Subdirectories (baseline → events → scene → interaction)
# ==============================================================================

add_subdirectory(00_hello_testbed)
add_subdirectory(01_test_events)
add_subdirectory(02_test_scene)
add_subdirectory(03_test_interaction)
