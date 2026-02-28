#==============================================================================
# Sample 04_scene_interaction: SceneDemoLayer + InteractionDemoLayer.
# Tests: vnescene PerspectiveCamera, vneinteraction CameraSystemController,
#        vneevents EventManager.
# Only built when vne::interaction is available.
#==============================================================================

if(NOT TARGET vne::scene OR NOT TARGET vne::interaction)
    return()
endif()

add_executable(sample_04_scene_interaction demo_scene_interaction.cpp)
add_sample_platform_sources(sample_04_scene_interaction)

target_link_libraries(sample_04_scene_interaction PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_compile_definitions(sample_04_scene_interaction PRIVATE "VNE_DEMO_ID=\"scene_interaction\"")

set_target_properties(sample_04_scene_interaction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
