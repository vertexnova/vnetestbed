#==============================================================================
# Sample 02_test_scene: camera switcher, lights, rotating cubes.
# Extends 01_test_events with a Scene panel (vnescene lights + IRenderDevice).
#==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

add_executable(sample_02_test_scene demo_test_scene.cpp)
add_sample_platform_sources(sample_02_test_scene)

target_link_libraries(sample_02_test_scene PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_include_directories(sample_02_test_scene PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_02_test_scene PRIVATE
    "VNE_DEMO_ID=\"test_scene\""
)

set_target_properties(sample_02_test_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
