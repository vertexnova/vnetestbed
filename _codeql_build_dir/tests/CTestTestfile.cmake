# CMake generated Testfile for 
# Source directory: /home/runner/work/vnetestbed/vnetestbed/tests
# Build directory: /home/runner/work/vnetestbed/vnetestbed/_codeql_build_dir/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[vnetestbed_tests]=] "/home/runner/work/vnetestbed/vnetestbed/_codeql_build_dir/bin/vnetestbed_tests")
set_tests_properties([=[vnetestbed_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/vnetestbed/vnetestbed/tests/CMakeLists.txt;73;add_test;/home/runner/work/vnetestbed/vnetestbed/tests/CMakeLists.txt;0;")
subdirs("../deps/external/googletest")
