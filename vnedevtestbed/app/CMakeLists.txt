#==============================================================================
# vnedevtestbed runner: minimal GLFW window + plugin loop (validates core).
# Optional; built only when glfw and glad are available.
#==============================================================================

if(NOT TARGET glfw OR NOT TARGET glad::glad)
    return()
endif()

add_executable(vnedevtestbed_runner
    main.cpp
    stub_plugin.cpp
)

target_link_libraries(vnedevtestbed_runner PRIVATE
    vne::devtestbed_core
    glfw
    glad::glad
)

target_compile_features(vnedevtestbed_runner PRIVATE cxx_std_20)

set_target_properties(vnedevtestbed_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
