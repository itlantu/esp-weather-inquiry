cmake_minimum_required(VERSION 3.25)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include("${CMAKE_SOURCE_DIR}/cmake/env_read.cmake")
set(EXTRA_COMPONENT_DIRS  "${CMAKE_SOURCE_DIR}/src")

#list(APPEND EXTRA_COMPONENT_DIRS "")
project(esp-weather-inquiry)
