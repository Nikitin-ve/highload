# CMake generated Testfile for 
# Source directory: /home/niki-ve/highload
# Build directory: /home/niki-ve/highload/build_release
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(testsuite-highload "/home/niki-ve/highload/build_release/venv-userver-default/bin/python" "/home/niki-ve/highload/build_release/runtests-highload" "--service-logs-pretty" "-vv" "/home/niki-ve/highload/tests")
set_tests_properties(testsuite-highload PROPERTIES  _BACKTRACE_TRIPLES "/usr/lib/cmake/userver/UserverTestsuite.cmake;270;add_test;/usr/lib/cmake/userver/UserverTestsuite.cmake;467;userver_testsuite_add;/home/niki-ve/highload/CMakeLists.txt;20;userver_testsuite_add_simple;/home/niki-ve/highload/CMakeLists.txt;0;")
