#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vnetestbed)
    return()
endif()

# 01 - Hello Testbed: minimal usage of vne::testbed (hello, get_version)
add_subdirectory(01_hello_testbed)

# 02 - Layer runner: GLFW window + layer lifecycle (LayerStack)
add_subdirectory(02_plugin_runner)

# 03 - OpenGL renderer: real GPU rendering + vnescene camera + vneinteraction + vneevents
# Only build when vne::interaction is available (VNE_WITH_VNEINTRACTION=ON).
if(TARGET vne::interaction)
    add_subdirectory(03_opengl_renderer)
endif()
