#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vnetestbed)
    return()
endif()

# 01 - Hello Testbed: minimal usage of vne::testbed (hello, get_version)
add_subdirectory(01_hello_testbed)

# 02 - Plugin runner: GLFW window + plugin lifecycle (PluginRegistry)
add_subdirectory(02_plugin_runner)
