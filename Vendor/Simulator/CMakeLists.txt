cmake_minimum_required(VERSION 3.16)
project(hc32_display_simulator C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." REALPATH)
set(HC32_DIR "${REPO_ROOT}/Vendor/HC32")

# 模拟器只运行带屏产品的界面，默认 X1Pro。
set(SIM_PRODUCT "X1Pro" CACHE STRING "Display product to simulate: X1Pro or XBase")
set_property(CACHE SIM_PRODUCT PROPERTY STRINGS X1Pro XBase)
if(NOT SIM_PRODUCT MATCHES "^(X1Pro|XBase)$")
    message(FATAL_ERROR "SIM_PRODUCT must be X1Pro or XBase")
endif()
set(SIM_PRODUCT_DIR ${HC32_DIR}/Products/${SIM_PRODUCT})
file(STRINGS "${SIM_PRODUCT_DIR}/product.cmake" PROJECT_VERSION_LINE
    REGEX "^set\\(PRODUCT_VERSION [0-9.]+\\)")
string(REGEX REPLACE "^set\\(PRODUCT_VERSION ([0-9.]+)\\)" "\\1" PROJECT_VERSION "${PROJECT_VERSION_LINE}")
configure_file(
    ${HC32_DIR}/Platform/Config/mcu_version_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
    @ONLY
)

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

# 与固件共用同一份 LVGL 源码（External/lvgl），配置用本目录的 lv_conf.h（LV_CONF_PATH 指定）。
set(LVGL_DIR ${REPO_ROOT}/External/lvgl)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS "${LVGL_DIR}/src/*.c")
# 共享 UI 通过 app_lvgl 使用模拟器显示配置；核心不依赖它。
add_library(sim_lvgl STATIC ${LVGL_SOURCES})
target_include_directories(sim_lvgl PUBLIC ${REPO_ROOT}/External ${LVGL_DIR})
target_compile_definitions(sim_lvgl PUBLIC LV_CONF_PATH=${CMAKE_CURRENT_LIST_DIR}/lv_conf.h)
add_library(app_lvgl INTERFACE)
target_link_libraries(app_lvgl INTERFACE sim_lvgl)
add_subdirectory(${REPO_ROOT}/App shared-app)
file(GLOB HAL_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/HAL/*.cpp)
# 固件电源时序原样编译：强制包含模拟器 HAL.h（屏蔽固件 HAL.h 的硬件依赖），GPIO/时基由 HAL/Arduino.h 提供
set(FIRMWARE_POWER_SOURCE ${HC32_DIR}/Platform/HAL/HAL_Power.cpp)
set_source_files_properties(${FIRMWARE_POWER_SOURCE} PROPERTIES
    COMPILE_OPTIONS "-include;${CMAKE_CURRENT_LIST_DIR}/HAL/HAL.h")
# HC32 平台层（与固件同一份）
file(GLOB APP_PLATFORM_SOURCES CONFIGURE_DEPENDS ${HC32_DIR}/Display/AppPlatform/*.cpp)

add_executable(main
    main.cpp
    ${APP_PLATFORM_SOURCES}
    ${HC32_DIR}/Platform/State/SystemState.cpp
    ${HAL_SOURCES}
    ${FIRMWARE_POWER_SOURCE}
)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/HAL
    ${HC32_DIR}/Display
    ${HC32_DIR}/Libraries
    ${REPO_ROOT}/External
    ${LVGL_DIR}
    ${HC32_DIR}/Platform/Config
    ${HC32_DIR}/Platform/State
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config
    ${SIM_PRODUCT_DIR}
)
target_compile_definitions(main PRIVATE
    # 明确指定模拟器配置，避免与固件 lv_conf.h 按包含顺序混淆
    LV_CONF_PATH=${CMAKE_CURRENT_LIST_DIR}/lv_conf.h
    LVGL_SIMULATOR
)
target_link_libraries(main PRIVATE app_ui)
target_compile_options(main PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>)
if(TARGET PkgConfig::SDL2)
    target_link_libraries(sim_lvgl PUBLIC PkgConfig::SDL2)
else()
    target_link_libraries(sim_lvgl PUBLIC SDL2::SDL2)
endif()

# 输出到各自构建目录的 bin/main：X1Pro 与 XBase 可用两个构建目录并存，互不覆盖。
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_custom_target(run COMMAND $<TARGET_FILE:main>)
