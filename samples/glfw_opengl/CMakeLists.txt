#==============================================================================
# GLFW + OpenGL sample set. Each sample: one executable, shared main, one demo.
#==============================================================================

if(NOT TARGET vne::testbed OR NOT TARGET glfw OR NOT TARGET glad::glad)
    return()
endif()
if(NOT VNE_TESTBED_OPENGL AND NOT VNE_TESTBED_OPENGLES)
    return()
endif()

add_subdirectory(01_triangle)
add_subdirectory(02_events)
add_subdirectory(03_scene)
add_subdirectory(04_scene_interaction)

# Testbed integration samples (grid + axes baseline, progressive features)
add_subdirectory(00_hello_testbed)
add_subdirectory(01_test_events)
add_subdirectory(02_test_scene)
add_subdirectory(03_test_interaction)
