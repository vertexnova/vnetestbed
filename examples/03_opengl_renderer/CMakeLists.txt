#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

# Requires GLFW, glad, and the OpenGL adapter (VNE_TESTBED_OPENGL).
if(NOT TARGET glfw OR NOT TARGET glad::glad)
    return()
endif()

add_executable(example_03_opengl_renderer main.cpp)

target_include_directories(example_03_opengl_renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(example_03_opengl_renderer
    PRIVATE
        vne::testbed
        glfw
)

if(TARGET vne::logging)
    target_link_libraries(example_03_opengl_renderer PRIVATE vne::logging)
endif()

set_target_properties(example_03_opengl_renderer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
