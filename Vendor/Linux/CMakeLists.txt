cmake_minimum_required(VERSION 3.16)
project(linux_apps LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LINUX_APP_DISPLAY "Build the RK3506 display application" ON)
option(LINUX_APP_SURVEY "Build the independent Survey application" ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
enable_testing()

if(NOT LINUX_APP_DISPLAY AND NOT LINUX_APP_SURVEY)
    message(FATAL_ERROR "At least one of LINUX_APP_DISPLAY and LINUX_APP_SURVEY must be ON")
endif()

if(LINUX_APP_DISPLAY)
    add_subdirectory(Apps/Display)
endif()
if(LINUX_APP_SURVEY)
    add_subdirectory(Apps/Survey)
endif()
