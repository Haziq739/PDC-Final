# CMake generated Testfile for 
# Source directory: /home/haziq/Desktop/PDC-Final
# Build directory: /home/haziq/Desktop/PDC-Final/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_sosp "/home/haziq/Desktop/PDC-Final/build/test_sosp")
set_tests_properties(test_sosp PROPERTIES  _BACKTRACE_TRIPLES "/home/haziq/Desktop/PDC-Final/CMakeLists.txt;49;add_test;/home/haziq/Desktop/PDC-Final/CMakeLists.txt;0;")
add_test(test_mpi_distributor "/usr/bin/mpiexec" "-n" "2" "/home/haziq/Desktop/PDC-Final/build/test_mpi_distributor")
set_tests_properties(test_mpi_distributor PROPERTIES  _BACKTRACE_TRIPLES "/home/haziq/Desktop/PDC-Final/CMakeLists.txt;151;add_test;/home/haziq/Desktop/PDC-Final/CMakeLists.txt;0;")
