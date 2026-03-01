#==============================================================================
# Sample 00_hello_testbed: grid + axes + Stats panel.
# Baseline demo — proves FBO viewport, orbit camera, ImGui Settings panel.
#==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

add_executable(sample_00_hello_testbed demo_hello_testbed.cpp)
add_sample_platform_sources(sample_00_hello_testbed)

target_link_libraries(sample_00_hello_testbed PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_include_directories(sample_00_hello_testbed PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_00_hello_testbed PRIVATE
    "VNE_DEMO_ID=\"hello_testbed\""
)

set_target_properties(sample_00_hello_testbed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
