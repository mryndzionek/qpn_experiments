cmake_minimum_required(VERSION 2.6)
project(qpn_experiments)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 2)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-attributes")

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
  )

set(PROJECT_INCLUDES ${CMAKE_PREFIX_PATH}/include)

include_directories("${PROJECT_INCLUDES}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/qpn/include")

add_subdirectory(examples)
