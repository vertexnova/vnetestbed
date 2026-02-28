#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Samples: GLFW+OpenGL and (future) vnecross+OpenGL sample sets.
# Each sample uses shared main + demo registration (VNETESTBED_REGISTER_DEMO).
#==============================================================================

if(NOT TARGET vne::testbed)
    return()
endif()

set(SAMPLES_GLFW_OPENGL_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/glfw_opengl/main.cpp)

# Function to add the shared main.cpp to a sample executable (desktop).
function(add_sample_platform_sources TARGET_NAME)
    target_sources(${TARGET_NAME} PRIVATE ${SAMPLES_GLFW_OPENGL_MAIN})
endfunction()

# GLFW + OpenGL sample set (requires glfw, glad, and OpenGL backend)
if(TARGET glfw AND TARGET glad::glad)
    if(VNE_TESTBED_OPENGL OR VNE_TESTBED_OPENGLES)
        add_subdirectory(glfw_opengl)
    endif()
endif()
