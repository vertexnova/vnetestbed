#==============================================================================
# Sample 01_triangle: TriangleDemoLayer via shared main + demo registration.
#==============================================================================

add_executable(sample_01_triangle demo_triangle.cpp)
add_sample_platform_sources(sample_01_triangle)

target_link_libraries(sample_01_triangle PRIVATE
    vne::testbed
    glfw
    glad::glad
)

# Demo id must be a single C++ token (no leading digits); use "triangle" for 01_triangle sample.
target_compile_definitions(sample_01_triangle PRIVATE VNE_DEMO_ID=triangle)

set_target_properties(sample_01_triangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
