#==============================================================================
# Sample 03_scene: SceneDemoLayer via shared main + demo registration.
# Tests: vnescene PerspectiveCamera, SceneState, IDebugDraw.
#==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

add_executable(sample_03_scene demo_scene.cpp)
add_sample_platform_sources(sample_03_scene)

target_link_libraries(sample_03_scene PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_compile_definitions(sample_03_scene PRIVATE "VNE_DEMO_ID=\"scene\"")

set_target_properties(sample_03_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
