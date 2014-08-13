cmake_minimum_required (VERSION 2.6)

project(molly)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(molly-test molly-test.cc device.cc)

add_executable(mollyd molly-daemon.cc device.cc)

