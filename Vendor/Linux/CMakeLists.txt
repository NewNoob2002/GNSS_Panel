cmake_minimum_required(VERSION 3.16)
project(rk3506_display LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RK3506_DEBUG_MONITOR "Enable LVGL FPS/CPU and memory monitors" OFF)
option(RK3506_LVGL_DEMO "Build standalone LVGL benchmark with on-screen FPS/CPU monitor" OFF)

# Locate repository root and shared components
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
set(SHARED_APP_DIR "${REPO_ROOT}/App")
set(EXTERNAL_DIR "${REPO_ROOT}/External")

# Modular sub-components
include(cmake/lvgl.cmake)
add_subdirectory(display)

enable_testing()

if(RK3506_LVGL_DEMO)
    add_subdirectory(demo)
else()
    # The shared module consumes this display contract; core/framework targets never link it.
    add_library(app_lvgl INTERFACE)
    target_link_libraries(app_lvgl INTERFACE lvgl8311)
    target_include_directories(app_lvgl INTERFACE "${EXTERNAL_DIR}")
    add_subdirectory("${SHARED_APP_DIR}" shared_app)

    add_library(demo_backend OBJECT
        Backends/Demo/DemoStatus.cpp Backends/Demo/SimulatedSystem.cpp)
    target_include_directories(demo_backend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Backends/Demo")
    target_link_libraries(demo_backend PUBLIC app_ui)
    target_compile_options(demo_backend PRIVATE -Wall -Wextra -Werror)

    # Standalone utilities and application entrypoints
    add_executable(lvgl_baseline baseline.c)
    target_link_libraries(lvgl_baseline PRIVATE lvgl8311)
    add_test(NAME lvgl_baseline COMMAND lvgl_baseline)

    add_executable(display_test display_test.c)
    target_link_libraries(display_test PRIVATE lv_display)
    target_compile_options(display_test PRIVATE -Wall -Wextra -Werror)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(rk3506_app platform/linux/main.cpp platform/linux_button.c)
        target_include_directories(rk3506_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${REPO_ROOT}")
        target_link_libraries(rk3506_app PRIVATE demo_backend lv_display)
        target_compile_options(rk3506_app PRIVATE -Wall -Wextra -Werror)

        add_executable(button_probe button_probe.c platform/linux_button.c)
        target_include_directories(button_probe PRIVATE platform)
        target_compile_definitions(button_probe PRIVATE _POSIX_C_SOURCE=200809L)
        target_compile_options(button_probe PRIVATE -Wall -Wextra -Werror)
    endif()

    # Test suite
    set(APPLICATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory(tests)
endif()
