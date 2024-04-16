# Copyright (C) 2022 Xilinx, Inc
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.12)

project(BasicTests)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(MPI)

include("${CMAKE_CURRENT_LIST_DIR}/../xrt_host_api/CMakeLists.txt")

add_executable(test basic_test.cpp)
target_link_libraries(test PRIVATE vnx Threads::Threads)

add_executable(latency latency_test.cpp)
target_link_libraries(latency PRIVATE vnx Threads::Threads)

if (MPI_CXX_FOUND)
    add_executable(scale scale_test.cpp)
    target_link_libraries(scale PRIVATE vnx Threads::Threads MPI::MPI_CXX)
endif(MPI_CXX_FOUND)