#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

cmake_minimum_required(VERSION 3.19)

# Single source of truth for version (updated by release-please on release)
set(VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/VERSION")
if(EXISTS "${VERSION_FILE}")
    file(READ "${VERSION_FILE}" VERSION_FROM_FILE)
    string(STRIP "${VERSION_FROM_FILE}" VERSION_FROM_FILE)
    if(VERSION_FROM_FILE)
        set(PROJECT_VERSION_INIT "${VERSION_FROM_FILE}")
    endif()
endif()
if(NOT DEFINED PROJECT_VERSION_INIT)
    set(PROJECT_VERSION_INIT "1.0.0")
endif()

project(vnetestbed VERSION ${PROJECT_VERSION_INIT} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake modules from vnecmake submodule (cmake/vnecmake)
set(VNE_DEPS_INTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/internal)
set(VNE_DEPS_EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/deps/external)
set(VNECMAKE_MODULES "${PROJECT_SOURCE_DIR}/cmake/vnecmake/modules")
if(NOT EXISTS "${VNECMAKE_MODULES}/VneUseDep.cmake")
    message(FATAL_ERROR "vnecmake not found at cmake/vnecmake. Run: git submodule update --init --recursive")
endif()
list(APPEND CMAKE_MODULE_PATH "${VNECMAKE_MODULES}")
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(ProjectSetup)

#==============================================================================
# Platform Detection
#==============================================================================
if(DEFINED VNE_TARGET_PLATFORM)
    message(STATUS "Using manually specified target platform: ${VNE_TARGET_PLATFORM}")
else()
    if(EMSCRIPTEN)
        set(VNE_TARGET_PLATFORM "Web")
    elseif(WIN32)
        set(VNE_TARGET_PLATFORM "Windows")
    elseif(APPLE)
        if(VISIONOS OR ${CMAKE_SYSTEM_NAME} STREQUAL "visionOS")
            set(VNE_TARGET_PLATFORM "visionOS")
        elseif(IOS OR ${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
            set(VNE_TARGET_PLATFORM "iOS")
        else()
            set(VNE_TARGET_PLATFORM "macOS")
        endif()
    elseif(UNIX)
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(VNE_TARGET_PLATFORM "Linux")
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
            set(VNE_TARGET_PLATFORM "Android")
        else()
            message(FATAL_ERROR "Unsupported UNIX platform: ${CMAKE_SYSTEM_NAME}")
        endif()
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()
message(STATUS "Detected platform: ${VNE_TARGET_PLATFORM}")

#==============================================================================
# Platform-specific compile definitions
#==============================================================================
if(VNE_TARGET_PLATFORM STREQUAL "macOS")
    add_compile_definitions(VNE_PLATFORM_MACOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "iOS")
    add_compile_definitions(VNE_PLATFORM_IOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "visionOS")
    add_compile_definitions(VNE_PLATFORM_VISIONOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "Windows")
    add_compile_definitions(VNE_PLATFORM_WIN)
elseif(VNE_TARGET_PLATFORM STREQUAL "Linux")
    add_compile_definitions(VNE_PLATFORM_LINUX)
elseif(VNE_TARGET_PLATFORM STREQUAL "Android")
    add_compile_definitions(VNE_PLATFORM_ANDROID)
elseif(VNE_TARGET_PLATFORM STREQUAL "Web")
    add_compile_definitions(VNE_PLATFORM_WEB)
endif()

#==============================================================================
# Internal Libraries (deps/internal) - use if already in build, else add
#==============================================================================
# vneinteraction is optional; only added when VNE_WITH_VNEINTRACTION=ON (see VNEPrivateDeps.cmake).
option(VNE_WITH_VNEINTRACTION "Enable private dependency vneinteraction" OFF)
option(VNE_TESTBED_IMGUI "Build ImGui layer (docking + viewports; requires OpenGL + GLFW)" ON)
option(VNE_TESTBED_VNEIO "Enable vneio for logo (ImGui + GLFW window icon); requires nrrdio in vneio deps" ON)
include(VneUseDep)
include(VneTestbedDeps)

vnetestbed_use_dep(TARGET vne::common SUBDIR vnecommon DEPS_DIR INTERNAL
  CACHE_VARS BUILD_EXAMPLES OFF SAVE_RESTORE BUILD_EXAMPLES)
vnetestbed_use_dep(TARGET vne::logging SUBDIR vnelogging DEPS_DIR INTERNAL
  CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF SAVE_RESTORE BUILD_TESTS BUILD_EXAMPLES)

# External Libraries (deps/external)
vnetestbed_use_dep(TARGET glfw SUBDIR glfw DEPS_DIR EXTERNAL
  CACHE_VARS GLFW_BUILD_TESTS OFF GLFW_BUILD_EXAMPLES OFF GLFW_BUILD_DOCS OFF
  SAVE_RESTORE GLFW_BUILD_DOCS)
vnetestbed_use_dep(TARGET glad::glad SUBDIR glad DEPS_DIR INTERNAL)

# ImGui (docking branch) - optional, requires glfw + glad + imgui submodule
set(VNE_TESTBED_IMGUI_AVAILABLE OFF)
if(VNE_TESTBED_IMGUI AND TARGET glfw AND TARGET glad::glad)
    if(EXISTS "${VNE_DEPS_EXTERNAL_DIR}/imgui/imgui.h")
        include(${CMAKE_CURRENT_LIST_DIR}/cmake/AddImgui.cmake)
        set(VNE_TESTBED_IMGUI_AVAILABLE ON)
        message(STATUS "VNE_TESTBED_IMGUI: available — imgui submodule found")
    else()
        message(STATUS "VNE_TESTBED_IMGUI: OFF — imgui submodule not initialized")
        message(STATUS "  Run: git submodule update --init deps/external/imgui")
    endif()
elseif(VNE_TESTBED_IMGUI AND (NOT TARGET glfw OR NOT TARGET glad::glad))
    message(STATUS "VNE_TESTBED_IMGUI: OFF — requires glfw and glad")
endif()

vnetestbed_use_dep(TARGET vne::events SUBDIR vneevents DEPS_DIR INTERNAL
  CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF SAVE_RESTORE BUILD_TESTS BUILD_EXAMPLES)
vnetestbed_use_dep(TARGET vne::math SUBDIR vnemath DEPS_DIR INTERNAL
  CACHE_VARS VNE_MATH_TESTS OFF BUILD_TESTS OFF BUILD_EXAMPLES OFF
  SAVE_RESTORE BUILD_TESTS BUILD_EXAMPLES)
vnetestbed_use_dep(TARGET vne::scene SUBDIR vnescene DEPS_DIR INTERNAL
  CACHE_VARS VNE_SCENE_TESTS OFF BUILD_TESTS OFF BUILD_EXAMPLES OFF
  SAVE_RESTORE BUILD_TESTS BUILD_EXAMPLES)
# Optional: vneio for image loading (logo in ImGui + GLFW window icon).
# vneio's image component requires nrrdio; only add vneio when VNE_TESTBED_VNEIO=ON and nrrdio is available.
set(VNE_TESTBED_HAVE_VNEIO OFF)
if(VNE_TESTBED_IMGUI_AVAILABLE AND VNE_TESTBED_VNEIO AND EXISTS "${VNE_DEPS_INTERNAL_DIR}/vneio/CMakeLists.txt")
  set(_vneio_nrrdio_ok OFF)
  if(EXISTS "${VNE_DEPS_INTERNAL_DIR}/vneio/deps/external/nrrdio/CMakeLists.txt" OR
     EXISTS "${VNE_DEPS_INTERNAL_DIR}/vneio/deps/external/nrrdio/nrrdio/CMakeLists.txt")
    set(_vneio_nrrdio_ok ON)
  endif()
  find_package(nrrdio QUIET)
  if(nrrdio_FOUND)
    set(_vneio_nrrdio_ok ON)
  endif()
  if(_vneio_nrrdio_ok)
    vnetestbed_use_dep(TARGET vne::io SUBDIR vneio DEPS_DIR INTERNAL
      CACHE_VARS VNEIO_BUILD_TESTS OFF VNEIO_BUILD_EXAMPLES OFF
      SAVE_RESTORE VNEIO_BUILD_TESTS VNEIO_BUILD_EXAMPLES)
    if(TARGET vne::io)
      set(VNE_TESTBED_HAVE_VNEIO ON)
      message(STATUS "VneTestbed: VNE_TESTBED_VNEIO=ON — vneio linked, logo in ImGui + GLFW window icon")
    endif()
  else()
    message(STATUS "VneTestbed: VNE_TESTBED_VNEIO=ON but vneio skipped (nrrdio required; add deps/external/nrrdio under vneio for logo)")
  endif()
endif()
include(VNEPrivateDeps)

# Include dirs and config
set(VNE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(VNE_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

# Logo PNG path for ImGui + GLFW: generate from SVG if rsvg-convert found, else use source PNG if present.
# Keep VNETESTBED_LOGO_PNG_PATH as "" or path (no quotes) so if(NOT ...) and if(...) work; config.h.in adds quotes.
set(VNETESTBED_LOGO_PNG_PATH "")
set(_logo_svg "${PROJECT_SOURCE_DIR}/icons/vertexnova_logo_medallion_only.svg")
set(_logo_png_build "${CMAKE_BINARY_DIR}/bin/icons/vertexnova_logo_medallion_only.png")
set(_logo_png_src "${PROJECT_SOURCE_DIR}/icons/vertexnova_logo_medallion_only.png")
if(VNE_TESTBED_HAVE_VNEIO AND EXISTS "${_logo_svg}")
  find_program(RSVG_CONVERT rsvg-convert)
  if(RSVG_CONVERT)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/bin/icons")
    execute_process(COMMAND ${RSVG_CONVERT} -w 128 -h 128 -o "${_logo_png_build}" "${_logo_svg}"
      RESULT_VARIABLE _rsvg_ret)
    if(_rsvg_ret EQUAL 0)
      set(VNETESTBED_LOGO_PNG_PATH "${_logo_png_build}")
      message(STATUS "VneTestbed: logo PNG generated at ${VNETESTBED_LOGO_PNG_PATH}")
    endif()
  endif()
  if(NOT VNETESTBED_LOGO_PNG_PATH AND EXISTS "${_logo_png_src}")
    set(VNETESTBED_LOGO_PNG_PATH "${_logo_png_src}")
    message(STATUS "VneTestbed: using logo PNG from source icons/")
  endif()
elseif(VNE_TESTBED_HAVE_VNEIO)
  set(VNETESTBED_LOGO_PNG_PATH "")
endif()
if(NOT DEFINED VNETESTBED_LOGO_PNG_PATH)
  set(VNETESTBED_LOGO_PNG_PATH "")
endif()

# macOS app bundle icon (.icns) for Dock and application menu (glfwSetWindowIcon does not work on macOS)
set(VNETESTBED_MACOS_ICON_ICNS "")
if(APPLE AND VNE_TESTBED_IMGUI_AVAILABLE AND EXISTS "${_logo_svg}")
  find_program(RSVG_CONVERT rsvg-convert)
  if(RSVG_CONVERT)
    set(_icns_dir "${CMAKE_BINARY_DIR}/icons/vertexnova_logo.iconset")
    set(_icns_out "${CMAKE_BINARY_DIR}/icons/vertexnova_logo.icns")
    file(MAKE_DIRECTORY "${_icns_dir}")
    set(_sizes 16 32 128 256 512)
    set(_icns_ok ON)
    foreach(_s IN LISTS _sizes)
      execute_process(COMMAND ${RSVG_CONVERT} -w ${_s} -h ${_s} -o "${_icns_dir}/icon_${_s}x${_s}.png" "${_logo_svg}"
        RESULT_VARIABLE _ret)
      if(NOT _ret EQUAL 0)
        set(_icns_ok OFF)
      endif()
    endforeach()
    if(_icns_ok)
      execute_process(COMMAND iconutil -c icns "${_icns_dir}" -o "${_icns_out}" RESULT_VARIABLE _iconutil_ret)
      if(_iconutil_ret EQUAL 0)
        set(VNETESTBED_MACOS_ICON_ICNS "${_icns_out}")
        message(STATUS "VneTestbed: macOS app icon generated at ${VNETESTBED_MACOS_ICON_ICNS}")
      endif()
    endif()
  endif()
endif()

configure_file(${PROJECT_SOURCE_DIR}/configs/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

#==============================================================================
# Build Options
#==============================================================================
# Option naming and deps:
# - Use VNE_TESTBED_* for vnetestbed-only options so there is no conflict when
#   this project is used as a submodule (parent can set VNE_TESTBED_TESTS=OFF
#   without affecting BUILD_TESTS used by other deps).
# - BUILD_TESTS / BUILD_EXAMPLES are global; we force them (via VneUseDep
#   CACHE_VARS) before add_subdirectory for each dep so deps do not build their
#   own tests/examples when used as internal libs. We restore after each dep.
# - ENABLE_COVERAGE, ENABLE_DOXYGEN, WARNINGS_AS_ERRORS are global: if multiple
#   libs in deps/internal define the same option name, the first to run wins the
#   default; -D from the command line applies to all. No duplicate option()
#   definition (CMake uses cache), but behavior is "one value for the whole tree."
#==============================================================================
if(CMAKE_PROJECT_NAME STREQUAL "vnetestbed")
    set(VNE_TESTBED_DEV_DEFAULT ON)
else()
    set(VNE_TESTBED_DEV_DEFAULT OFF)
endif()
option(VNE_TESTBED_DEV "Dev build: enable tests (local development)" ${VNE_TESTBED_DEV_DEFAULT})
option(VNE_TESTBED_CI "CI build: enable tests (e.g. -DVNE_TESTBED_CI=ON in pipelines)" OFF)

if(VNE_TESTBED_CI)
    set(VNE_TESTBED_DEV OFF CACHE BOOL "" FORCE)
endif()

option(VNE_TESTBED_TESTS "Build vnetestbed test suite (turn OFF when used as submodule)" ON)
option(VNE_TESTBED_SAMPLES "Build vnetestbed sample programs (samples/; turn OFF when used as submodule)" OFF)
option(ENABLE_DOXYGEN "Enable Doxygen documentation builds" OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

#==============================================================================
# Coverage Support (match vnelogging: include FindCoverage only when enabled)
#==============================================================================
if(ENABLE_COVERAGE)
    include(FindCoverage)
endif()

if(VNE_TESTBED_CI)
    set(VNE_TESTBED_TESTS ON CACHE BOOL "" FORCE)
    set(VNE_TESTBED_SAMPLES OFF CACHE BOOL "" FORCE)
    message(STATUS "VneTestbed: CI build -> VNE_TESTBED_TESTS=ON, VNE_TESTBED_SAMPLES=OFF")
elseif(VNE_TESTBED_DEV)
    set(VNE_TESTBED_TESTS ON CACHE BOOL "" FORCE)
    set(VNE_TESTBED_SAMPLES ON CACHE BOOL "" FORCE)
    message(STATUS "VneTestbed: DEV build -> VNE_TESTBED_TESTS=ON, VNE_TESTBED_SAMPLES=ON")
endif()

#==============================================================================
# Compiler Warnings
#==============================================================================
add_library(TestbedWarnings INTERFACE)
include(ProjectWarnings)
set_project_warnings(TestbedWarnings)
add_library(vne::testbed::Warnings ALIAS TestbedWarnings)

#==============================================================================
# Build Settings Interface Library
#==============================================================================
add_library(TestbedBuildSettings INTERFACE)
target_compile_features(TestbedBuildSettings INTERFACE cxx_std_20)
add_library(vne::testbed::BuildSettings ALIAS TestbedBuildSettings)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_BUILD_TYPE_DEBUG VNE_DEVELOPER_BUILD)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_BUILD_TYPE_RELEASE)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_BUILD_TYPE_RELWITHDEBINFO)
elseif(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_BUILD_TYPE_MINSIZEREL)
endif()

if(VNE_TARGET_PLATFORM STREQUAL "Windows")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_WIN)
elseif(VNE_TARGET_PLATFORM STREQUAL "Linux")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_LINUX)
elseif(VNE_TARGET_PLATFORM STREQUAL "macOS")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_MACOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "iOS")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_IOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "visionOS")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_VISIONOS)
elseif(VNE_TARGET_PLATFORM STREQUAL "Android")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_ANDROID)
elseif(VNE_TARGET_PLATFORM STREQUAL "Web")
    target_compile_definitions(TestbedBuildSettings INTERFACE VNE_PLATFORM_WEB)
endif()

#==============================================================================
# Add Library
#==============================================================================
add_subdirectory(src)

#==============================================================================
# Tests
#==============================================================================
if(VNE_TESTBED_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

#==============================================================================
# Samples
#==============================================================================
if(VNE_TESTBED_SAMPLES)
    add_subdirectory(samples)
endif()

#==============================================================================
# Documentation
#==============================================================================
if(ENABLE_DOXYGEN)
    find_package(Doxygen QUIET OPTIONAL_COMPONENTS dot)
    if(Doxygen_FOUND)
        set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/docs/doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/doxyfile)
        set(DOXYGEN_HAVE_DOT ${DOXYGEN_DOT_FOUND})
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        add_custom_target(vnetestbed_doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        message(STATUS "Doxygen target: vnetestbed_doc_doxygen")
    else()
        message(STATUS "Doxygen not found. Install doxygen to build documentation.")
    endif()
endif()

#==============================================================================
# Installation
#==============================================================================
include(GNUInstallDirs)
install(TARGETS vnetestbed TestbedWarnings TestbedBuildSettings
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/vertexnova/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src/vertexnova/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova FILES_MATCHING PATTERN "*.h")
install(FILES ${CMAKE_BINARY_DIR}/config.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vertexnova/testbed)
