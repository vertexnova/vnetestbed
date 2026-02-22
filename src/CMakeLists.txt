#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

add_library(vnetestbed INTERFACE)
add_library(vne::testbed ALIAS vnetestbed)

target_include_directories(vnetestbed
    INTERFACE
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vnetestbed
    INTERFACE
        vne::testbed::Warnings
        vne::testbed::BuildSettings
)

if(TARGET vne::common)
    target_link_libraries(vnetestbed INTERFACE vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vnetestbed INTERFACE vne::logging)
endif()
if(TARGET glfw)
    target_link_libraries(vnetestbed INTERFACE glfw)
endif()
if(TARGET glad::glad)
    target_link_libraries(vnetestbed INTERFACE glad::glad)
endif()
if(TARGET vne::events)
    target_link_libraries(vnetestbed INTERFACE vne::events)
endif()
if(TARGET vne::math)
    target_link_libraries(vnetestbed INTERFACE vne::math)
endif()
if(TARGET vne::scene)
    target_link_libraries(vnetestbed INTERFACE vne::scene)
endif()
if(TARGET vne::interaction)
    target_link_libraries(vnetestbed INTERFACE vne::interaction)
endif()

target_compile_features(vnetestbed INTERFACE cxx_std_20)
