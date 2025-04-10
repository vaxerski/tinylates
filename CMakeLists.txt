cmake_minimum_required(VERSION 3.19)

project(
  tinylates
  VERSION 0.1
  DESCRIPTION "Tiny templates for C++")

set(CMAKE_CXX_STANDARD 23)
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wno-unused-parameter
  -Wno-unused-value
  -Wno-missing-field-initializers
  -Wno-narrowing
  -Wno-pointer-arith)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
  message(STATUS "Configuring tinylates in Debug")
else()
  add_compile_options(-O3)
  message(STATUS "Configuring tinylates in Release")
endif()

file(
  GLOB_RECURSE
  SRCFILES
  CONFIGURE_DEPENDS
  "src/*.cpp"
  "include/tinylates.hpp"
)

add_library(tinylates ${SRCFILES})
target_include_directories(
    tinylates
  PUBLIC "./include"
  PRIVATE "./src")

