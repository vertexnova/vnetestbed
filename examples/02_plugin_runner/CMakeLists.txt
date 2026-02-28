#==============================================================================
# 02_plugin_runner Example: GLFW window + layer lifecycle (LayerStack)
#==============================================================================
# Uses window::GlfwWindow from the library; only build when OpenGL backend is enabled.
if(NOT TARGET glfw OR NOT TARGET glad::glad)
    return()
endif()
if(NOT VNE_TESTBED_OPENGL AND NOT VNE_TESTBED_OPENGLES)
    return()
endif()

add_executable(example_02_plugin_runner
    main.cpp
)

target_link_libraries(example_02_plugin_runner PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_include_directories(example_02_plugin_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(example_02_plugin_runner PRIVATE cxx_std_20)

set_target_properties(example_02_plugin_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
