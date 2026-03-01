#==============================================================================
# Sample 01_test_events: event log + input polling panel.
# Extends 00_hello_testbed with a live Events panel in Settings.
#==============================================================================

if(NOT TARGET vne::scene)
    return()
endif()

add_executable(sample_01_test_events demo_test_events.cpp)
add_sample_platform_sources(sample_01_test_events)

target_link_libraries(sample_01_test_events PRIVATE
    vne::testbed
)

target_include_directories(sample_01_test_events PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_definitions(sample_01_test_events PRIVATE
    "VNE_DEMO_ID=\"test_events\""
)

set_target_properties(sample_01_test_events PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
