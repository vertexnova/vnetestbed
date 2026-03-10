#==============================================================================
# Sample 03_test_interaction: all manipulators, zoom methods, view presets.
# Only built when vne::interaction is available.
#==============================================================================

if(NOT TARGET vne::scene OR NOT TARGET vne::interaction)
    return()
endif()

add_executable(sample_03_test_interaction demo_test_interaction.cpp demo_test_interaction.h)
add_sample_platform_sources(sample_03_test_interaction)

target_link_libraries(sample_03_test_interaction PRIVATE
    vne::testbed
)

target_include_directories(sample_03_test_interaction PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_03_test_interaction PRIVATE
    "VNE_DEMO_ID=\"test_interaction\""
)

set_target_properties(sample_03_test_interaction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
