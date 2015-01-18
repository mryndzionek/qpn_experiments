cmake_minimum_required(VERSION 2.6)
project(qpn_experiments)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 2)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-attributes -fno-strict-aliasing")

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
  )

set(PROJECT_INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/include)

include_directories("${PROJECT_INCLUDES}")

add_subdirectory(examples)
