#==============================================================================
# Sample 02_events: EventsDemoLayer via shared main + demo registration.
# Tests: vneevents EventManager, EventListener, event dispatch.
#==============================================================================

if(NOT TARGET vne::events)
    return()
endif()

add_executable(sample_02_events demo_events.cpp)
add_sample_platform_sources(sample_02_events)

target_link_libraries(sample_02_events PRIVATE
    vne::testbed
    glfw
    glad::glad
)

target_compile_definitions(sample_02_events PRIVATE "VNE_DEMO_ID=\"events\"")

set_target_properties(sample_02_events PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
