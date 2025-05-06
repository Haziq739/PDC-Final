cmake_minimum_required(VERSION 3.10)
project(MOSP_Project)

# Find required packages
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

# METIS configuration
find_path(METIS_INCLUDE_DIR metis.h)
find_library(METIS_LIBRARY metis)

if(NOT METIS_INCLUDE_DIR OR NOT METIS_LIBRARY)
    message(FATAL_ERROR "METIS not found. Install with: sudo apt-get install libmetis-dev")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-----------------------------------------------------------------------------
# Compiler flags configuration
#-----------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=native)
    endif()
endif()

# SOSP Engine Test (Phase 2)
add_executable(test_sosp
    test/test_sosp.cpp
    src/sosp_engine.cpp
    src/graph.cpp
)

target_include_directories(test_sosp
    PRIVATE 
    include
    ${METIS_INCLUDE_DIR}
)

target_link_libraries(test_sosp
    PRIVATE
    OpenMP::OpenMP_CXX
    ${METIS_LIBRARY}
)

add_test(NAME test_sosp COMMAND test_sosp)

#-----------------------------------------------------------------------------
# Main MPI+OpenMP Hybrid Executable
#-----------------------------------------------------------------------------
add_executable(mosp_hybrid
    src/main_hybrid.cpp
    src/hybrid_engine.cpp
    src/mpi_distributor.cpp
    src/graph.cpp
    src/metis_utils.cpp
)

target_include_directories(mosp_hybrid
    PRIVATE
    include
    ${METIS_INCLUDE_DIR}
)

target_link_libraries(mosp_hybrid
    PRIVATE
    MPI::MPI_CXX
    ${METIS_LIBRARY}
    OpenMP::OpenMP_CXX
)

#-----------------------------------------------------------------------------
# Original MPI Executable (Legacy)
#-----------------------------------------------------------------------------
add_executable(mosp_proj
    src/main.cpp
    src/graph.cpp
    src/metis_utils.cpp
    src/mpi_distributor.cpp
    src/sosp_engine.cpp
)

target_include_directories(mosp_proj 
    PRIVATE 
    include 
    ${METIS_INCLUDE_DIR}
)

target_link_libraries(mosp_proj 
    PRIVATE 
    MPI::MPI_CXX 
    ${METIS_LIBRARY}
    OpenMP::OpenMP_CXX
)

#-----------------------------------------------------------------------------
# Test Executables
#-----------------------------------------------------------------------------
enable_testing()

# Regular graph tests (non-MPI)
add_executable(test_graph 
    test/test_graph.cpp 
    src/graph.cpp 
    src/metis_utils.cpp
)

target_include_directories(test_graph
    PRIVATE 
    include
    ${METIS_INCLUDE_DIR}
)

target_link_libraries(test_graph 
    PRIVATE 
    ${METIS_LIBRARY}
)

# MPI test executable - simplified Catch2 configuration
add_executable(test_mpi_distributor
    test/test_mpi_distributor.cpp
    src/mpi_distributor.cpp
    src/graph.cpp
    src/metis_utils.cpp
)

target_include_directories(test_mpi_distributor
    PRIVATE 
    include
    ${METIS_INCLUDE_DIR}
)

# Simplified Catch2 configuration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/catch.hpp")
    message(STATUS "Using bundled catch.hpp")
    target_compile_definitions(test_mpi_distributor PRIVATE CATCH_CONFIG_MAIN)
else()
    message(FATAL_ERROR "catch.hpp not found in include directory")
endif()

target_link_libraries(test_mpi_distributor
    PRIVATE
    MPI::MPI_CXX
    ${METIS_LIBRARY}
    OpenMP::OpenMP_CXX
)

add_test(NAME test_mpi_distributor 
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 $<TARGET_FILE:test_mpi_distributor>
)