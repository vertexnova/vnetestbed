#==============================================================================
# vnedevtestbed - Dev testbed core (plugin system, app context).
# No GLFW/OpenGL/CrossGL dependency; runner supplies concrete backends.
#==============================================================================

cmake_minimum_required(VERSION 3.19)

set(VNEDEVTESTBED_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/core)
set(VNEDEVTESTBED_CORE_INCLUDE ${VNEDEVTESTBED_CORE_DIR}/include)
set(VNEDEVTESTBED_CORE_SRC ${VNEDEVTESTBED_CORE_DIR}/src)

add_library(vnedevtestbed_core STATIC
    ${VNEDEVTESTBED_CORE_SRC}/plugin_registry.cpp
)

target_include_directories(vnedevtestbed_core
    PUBLIC
        $<BUILD_INTERFACE:${VNEDEVTESTBED_CORE_INCLUDE}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(vnedevtestbed_core PUBLIC cxx_std_20)

add_library(vne::devtestbed_core ALIAS vnedevtestbed_core)

# Optional runner (GLFW + plugin loop); built only when glfw/glad exist
add_subdirectory(app)
