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
    vertexnova/testbed/testbed.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/testbed/testbed.h
)

add_library(vnetestbed STATIC ${SOURCE_FILES} ${HEADER_FILES})

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})

add_library(vne::testbed ALIAS vnetestbed)

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

target_compile_features(vnetestbed PUBLIC cxx_std_20)
