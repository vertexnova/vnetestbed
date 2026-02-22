#==============================================================================
# 01_hello_testbed Example: minimal vnetestbed (logging only)
#==============================================================================

add_executable(example_01_hello_testbed main.cpp)

target_link_libraries(example_01_hello_testbed PRIVATE vne::testbed vne::logging)

target_include_directories(example_01_hello_testbed PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_01_hello_testbed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
