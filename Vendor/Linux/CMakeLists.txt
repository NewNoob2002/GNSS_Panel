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
include(${REPO_ROOT}/cmake/options.cmake)
set(SHARED_APP_DIR "${REPO_ROOT}/App")

# Modular sub-components
include(${REPO_ROOT}/cmake/lvgl.cmake)
if(RK3506_LVGL_DEMO)
    project_add_lvgl("${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h" DEMOS)
else()
    project_add_lvgl("${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h")
endif()
add_library(lvgl960 ALIAS lvgl)
target_compile_definitions(lvgl PUBLIC $<$<BOOL:${RK3506_DEBUG_MONITOR}>:__CORE_DEBUG>)
if(RK3506_LVGL_DEMO)
    set(RK3506_LVGL_DEMO_REFRESH_MS 8 CACHE STRING "LVGL demo refresh period in ms")
    set(RK3506_LVGL_DEMO_DRAW_DIVISOR 4 CACHE STRING "LVGL demo draw buffer fraction denominator")
    target_compile_definitions(lvgl PUBLIC
        RK3506_LVGL_DEMO=1
        RK3506_LVGL_DEMO_REFRESH_MS=${RK3506_LVGL_DEMO_REFRESH_MS}
        RK3506_LVGL_DEMO_DRAW_DIVISOR=${RK3506_LVGL_DEMO_DRAW_DIVISOR}
        _POSIX_C_SOURCE=200809L)
    target_compile_options(lvgl_demos PRIVATE -Wall -Wextra -Werror)
endif()
add_subdirectory(display)
add_subdirectory(platform)

enable_testing()

if(RK3506_LVGL_DEMO)
    add_subdirectory(demo)
else()
    add_subdirectory("${SHARED_APP_DIR}" shared_app)
    add_subdirectory("${REPO_ROOT}/Vendor/HostLog" host_log)
    add_subdirectory(Backends/Demo)

    # Standalone utilities and application entrypoints
    add_executable(lvgl_baseline baseline.c)
    target_link_libraries(lvgl_baseline PRIVATE lvgl::lvgl)
    add_test(NAME lvgl_baseline COMMAND lvgl_baseline)

    add_executable(display_test display_test.c)
    target_link_libraries(display_test PRIVATE linux::display linux::display_io)
    target_compile_options(display_test PRIVATE -Wall -Wextra -Werror)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_subdirectory(platform/linux)

        add_executable(button_probe button_probe.c)
        target_link_libraries(button_probe PRIVATE linux::button)
        target_compile_definitions(button_probe PRIVATE _POSIX_C_SOURCE=200809L)
        target_compile_options(button_probe PRIVATE -Wall -Wextra -Werror)
    endif()

    # Test suite
    set(APPLICATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory(tests)
endif()
