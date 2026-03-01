#==============================================================================
# Sample test_scene_interaction: all manipulators, zoom methods, view presets.
# Only built when vne::interaction is available.
#==============================================================================

if(NOT TARGET vne::scene OR NOT TARGET vne::interaction)
    return()
endif()

add_executable(sample_test_scene_interaction demo_test_interaction.cpp)
add_sample_platform_sources(sample_test_scene_interaction)

target_link_libraries(sample_test_scene_interaction PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_include_directories(sample_test_scene_interaction PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_test_scene_interaction PRIVATE
    "VNE_DEMO_ID=\"test_scene_interaction\""
)

set_target_properties(sample_test_scene_interaction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
