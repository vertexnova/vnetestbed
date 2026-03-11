#==============================================================================
# Sample 03_test_interaction: all manipulators, zoom methods, view presets.
# Mesh from testdata via vneio when VNE_TESTBED_HAVE_VNEIO. Only built when
# vne::interaction is available.
#==============================================================================

if(NOT TARGET vne::scene OR NOT TARGET vne::interaction)
    return()
endif()

add_executable(sample_03_test_interaction demo_test_interaction.cpp demo_test_interaction.h)
add_sample_platform_sources(sample_03_test_interaction)

target_link_libraries(sample_03_test_interaction PRIVATE
    vne::testbed
)
if(TARGET vne::io)
    target_link_libraries(sample_03_test_interaction PRIVATE vne::io)
endif()

target_include_directories(sample_03_test_interaction PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

set(VNETESTBED_TESTDATA_ROOT "${CMAKE_SOURCE_DIR}/testdata" CACHE PATH "Testdata root for samples")
target_compile_definitions(sample_03_test_interaction PRIVATE
    "VNE_DEMO_ID=\"test_interaction\""
    "VNETESTBED_TESTDATA_DIR=\"${VNETESTBED_TESTDATA_ROOT}\""
)
if(VNE_TESTBED_HAVE_VNEIO)
    target_compile_definitions(sample_03_test_interaction PRIVATE VNE_TESTBED_HAVE_VNEIO)
endif()

set_target_properties(sample_03_test_interaction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

# Mesh is drawn via testbed MeshRenderer (embedded Phong); no sample shaders needed.
