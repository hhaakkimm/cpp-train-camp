# CMake generated Testfile for 
# Source directory: /Users/robot/Desktop/hkms/cpp-train-camp/tests
# Build directory: /Users/robot/Desktop/hkms/cpp-train-camp/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(KeyValueStoreTests "/Users/robot/Desktop/hkms/cpp-train-camp/build/tests/test_key_value_store")
set_tests_properties(KeyValueStoreTests PROPERTIES  _BACKTRACE_TRIPLES "/Users/robot/Desktop/hkms/cpp-train-camp/tests/CMakeLists.txt;56;add_test;/Users/robot/Desktop/hkms/cpp-train-camp/tests/CMakeLists.txt;0;")
add_test(HttpRequestTests "/Users/robot/Desktop/hkms/cpp-train-camp/build/tests/test_http_request")
set_tests_properties(HttpRequestTests PROPERTIES  _BACKTRACE_TRIPLES "/Users/robot/Desktop/hkms/cpp-train-camp/tests/CMakeLists.txt;69;add_test;/Users/robot/Desktop/hkms/cpp-train-camp/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
