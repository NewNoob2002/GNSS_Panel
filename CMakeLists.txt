cmake_minimum_required(VERSION 3.20)

set(PLATFORMS HC32 Simulator Linux)
set(PLATFORM "HC32" CACHE STRING "Target platform to build: HC32, Simulator or Linux")
set_property(CACHE PLATFORM PROPERTY STRINGS ${PLATFORMS})
if(NOT PLATFORM IN_LIST PLATFORMS)
    message(FATAL_ERROR "Unknown PLATFORM '${PLATFORM}', expected one of: ${PLATFORMS}")
endif()

if(PLATFORM STREQUAL "HC32")
    project(GNSS_Panel LANGUAGES C CXX ASM)
else()
    project(GNSS_Panel LANGUAGES C CXX)
    enable_testing()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(Vendor/${PLATFORM})
