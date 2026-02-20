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
    vertexnova/template/template.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/template/template.h
)

add_library(vnetemplate STATIC ${SOURCE_FILES} ${HEADER_FILES})

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})

add_library(vne::template ALIAS vnetemplate)

target_include_directories(vnetemplate
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(vnetemplate
    PUBLIC
        vne::template::Warnings
        vne::template::BuildSettings
)

if(TARGET vne::common)
    target_link_libraries(vnetemplate PUBLIC vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vnetemplate PUBLIC vne::logging)
endif()

target_compile_features(vnetemplate PUBLIC cxx_std_20)
