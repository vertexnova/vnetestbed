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
