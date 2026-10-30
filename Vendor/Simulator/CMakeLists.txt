cmake_minimum_required(VERSION 3.16)
project(hc32_display_simulator C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
set(HC32_DIR "${REPO_ROOT}/Vendor/HC32")

# 模拟器支持两种业务后端：Board（默认，HC32 虚拟板 + AppPlatform + 固件 HAL_Power.cpp）与 Demo（Linux 演示后端）。
set(SIM_BACKENDS Board Demo)
set(SIM_BACKEND "Board" CACHE STRING "Simulator backend to run: Board or Demo")
set_property(CACHE SIM_BACKEND PROPERTY STRINGS ${SIM_BACKENDS})
if(NOT SIM_BACKEND IN_LIST SIM_BACKENDS)
    message(FATAL_ERROR "SIM_BACKEND must be Board or Demo")
endif()

# Board 后端可选择模拟的带屏产品，默认 X1Pro。
set(SIM_PRODUCT "X1Pro" CACHE STRING "Display product to simulate: X1Pro or XBase")
set_property(CACHE SIM_PRODUCT PROPERTY STRINGS X1Pro XBase)
if(NOT SIM_PRODUCT MATCHES "^(X1Pro|XBase)$")
    message(FATAL_ERROR "SIM_PRODUCT must be X1Pro or XBase")
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
endif()
if(NOT TARGET PkgConfig::SDL2)
    find_package(SDL2 QUIET CONFIG)
endif()
if(NOT TARGET PkgConfig::SDL2 AND NOT TARGET SDL2::SDL2)
    message(FATAL_ERROR "SDL2 development files were not found. Install libsdl2-dev on Ubuntu/Debian, or provide an SDL2 CMake package.")
endif()

include(${REPO_ROOT}/cmake/options.cmake)

# 上游目标负责收集 LVGL 源文件，模拟器仅指定自己的配置。
include(${REPO_ROOT}/cmake/lvgl.cmake)
project_add_lvgl("${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h")
add_library(sim::lvgl ALIAS lvgl)
add_subdirectory(${REPO_ROOT}/App shared-app)

if(SIM_BACKEND STREQUAL "Demo")
    add_subdirectory(${REPO_ROOT}/Vendor/Linux/Apps/Display/Backends/Demo demo_backend)

    add_executable(main main.cpp)
    add_executable(sim::main ALIAS main)
    target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_compile_definitions(main PRIVATE
        LV_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/lv_conf.h"
        LVGL_SIMULATOR
        SIM_BACKEND_DEMO=1
    )
    target_link_libraries(main PRIVATE linux::demo_backend)
else()
    set(SIM_PRODUCT_DIR ${HC32_DIR}/Products/${SIM_PRODUCT})
    file(STRINGS "${SIM_PRODUCT_DIR}/product.cmake" PROJECT_VERSION_LINE
        REGEX "^set\\(PRODUCT_VERSION [0-9.]+\\)")
    string(REGEX REPLACE "^set\\(PRODUCT_VERSION ([0-9.]+)\\)" "\\1" PROJECT_VERSION "${PROJECT_VERSION_LINE}")
    # 与固件相同的生成位置：产品配置目标（Products/config.cmake）只公开该目录。
    configure_file(
        ${HC32_DIR}/Platform/Config/mcu_version_config.h.in
        ${CMAKE_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
        @ONLY
    )

    add_library(sim_hardware_port INTERFACE)
    add_library(sim::hardware_port ALIAS sim_hardware_port)
    # LVGL_SIMULATOR 经端口传给 hc32::app_platform 等共享目标，使其走模拟器日志与 SDL 输入分支。
    target_compile_definitions(sim_hardware_port INTERFACE
        LVGL_SIMULATOR
        SIM_BACKEND_DEMO=0
    )
    target_include_directories(sim_hardware_port INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/HAL
    )
    target_link_libraries(sim_hardware_port INTERFACE proj::options)

    include(${HC32_DIR}/cmake/portable.cmake)
    hc32_add_portable("" ${SIM_PRODUCT} sim::hardware_port DISPLAY)

    add_library(sim_hal OBJECT HAL/HAL.cpp HAL/HAL_Board.cpp)
    add_library(sim::hal ALIAS sim_hal)
    target_link_libraries(sim_hal PUBLIC sim::hardware_port proj::options app::ui hc32::state hc32::power_policy)

    add_executable(main main.cpp)
    add_executable(sim::main ALIAS main)
    target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_compile_definitions(main PRIVATE
        LV_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/lv_conf.h"
        LVGL_SIMULATOR
        SIM_BACKEND_DEMO=0
    )
    target_link_libraries(main PRIVATE sim::hal hc32::app_platform hc32::power_policy hc32::state)
endif()
target_compile_options(main PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>)
if(TARGET PkgConfig::SDL2)
    target_link_libraries(lvgl PUBLIC PkgConfig::SDL2)
else()
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
endif()

# 输出到各自构建目录的 bin/main：X1Pro 与 XBase 可用两个构建目录并存，互不覆盖。
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_custom_target(run COMMAND $<TARGET_FILE:main>)
