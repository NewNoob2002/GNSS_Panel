cmake_minimum_required(VERSION 3.10)
project(lvgl)
set(CMAKE_C_STANDARD 11) #C11
set(CMAKE_CXX_STANDARD 17) #C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 模拟器只运行带屏产品的界面，默认 X1Pro。
set(SIM_PRODUCT "X1Pro" CACHE STRING "Display product to simulate: X1Pro or XBase")
set_property(CACHE SIM_PRODUCT PROPERTY STRINGS X1Pro XBase)
if(NOT SIM_PRODUCT MATCHES "^(X1Pro|XBase)$")
    message(FATAL_ERROR "SIM_PRODUCT must be X1Pro or XBase")
endif()
set(SIM_PRODUCT_DIR ${CMAKE_CURRENT_LIST_DIR}/../Products/${SIM_PRODUCT})
file(STRINGS "${SIM_PRODUCT_DIR}/product.cmake" PROJECT_VERSION_LINE
    REGEX "^set\\(PRODUCT_VERSION [0-9.]+\\)")
string(REGEX REPLACE "^set\\(PRODUCT_VERSION ([0-9.]+)\\)" "\\1" PROJECT_VERSION "${PROJECT_VERSION_LINE}")
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/../Platform/Config/mcu_version_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
    @ONLY
)

# 设置 CMake 策略以使用 <PackageName>_ROOT 变量
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

include_directories(${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR}/../Libraries)

# 与固件共用同一份 LVGL 源码（Libraries/lvgl），配置见 lv_conf.h。
set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/../Libraries/lvgl)
file(GLOB_RECURSE INCLUDES "lv_drivers/*.h" "./*.h")
file(GLOB_RECURSE SOURCES "lv_drivers/*.c" "${LVGL_DIR}/src/*.c")

file(GLOB_RECURSE UI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/App/*.c
    ${CMAKE_CURRENT_LIST_DIR}/App/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/App/Utils/*.hpp
    ${CMAKE_CURRENT_LIST_DIR}/HAL/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../Platform/support/support.c
    ${CMAKE_CURRENT_LIST_DIR}/../Libraries/easylogger/src/elog.c
    ${CMAKE_CURRENT_LIST_DIR}/../Libraries/easylogger/src/elog_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/../Libraries/easylogger/port/*.c
)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/App
    ${CMAKE_CURRENT_LIST_DIR}/App/Pages
    ${CMAKE_CURRENT_LIST_DIR}/App/Utils
    ${CMAKE_CURRENT_LIST_DIR}/App/Resource
    ${CMAKE_CURRENT_LIST_DIR}/App/config
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config
    ${CMAKE_CURRENT_LIST_DIR}/../Platform/support
    ${CMAKE_CURRENT_LIST_DIR}/../Platform/Config
    ${SIM_PRODUCT_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/HAL
    ${CMAKE_CURRENT_LIST_DIR}/../Libraries/easylogger/inc
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

# SDL2 配置
if(WIN32)
    # Windows 版本保留手动 SDL2 路径配置，便于兼容没有 SDL2Config.cmake 的环境。
    set(SDL2_ROOT "E:/SDL2-2.32.10" CACHE PATH "SDL2 root directory on Windows")
    set(SDL2_INCLUDE_DIRS ${SDL2_ROOT}/include)
    set(SDL2_LIB_DIR ${SDL2_ROOT}/lib/x64)
    set(SDL2_LIBRARIES ${SDL2_LIB_DIR}/SDL2.lib)
    link_directories(${SDL2_LIB_DIR})
else()
    # Linux/macOS 使用系统安装的 SDL2。Ubuntu/Debian: sudo apt install libsdl2-dev
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
    endif()
    if(NOT TARGET PkgConfig::SDL2)
        find_package(SDL2 QUIET CONFIG)
    endif()
    if(NOT TARGET PkgConfig::SDL2 AND NOT TARGET SDL2::SDL2 AND NOT SDL2_FOUND)
        message(FATAL_ERROR "SDL2 development files were not found. Install libsdl2-dev on Ubuntu/Debian, or provide an SDL2 CMake package.")
    endif()
endif()

add_executable(main main.cpp mouse_cursor_icon.c ${SOURCES} ${INCLUDES} ${UI_SOURCES}
    ../Platform/Config/mcu_define.h
    ../Platform/Config/mcu_define.h)
target_compile_definitions(main PRIVATE
    # 明确指定模拟器配置，避免与 Libraries/lvgl/lv_conf.h（固件配置）按包含顺序混淆
    LV_CONF_PATH=${CMAKE_CURRENT_LIST_DIR}/lv_conf.h
    LV_INCLUDE_SIMPLE
    LVGL_SIMULATOR
    MIDDLE_LOGO_USE
)
target_include_directories(main PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${LVGL_DIR}
)


add_compile_options(-Wno-deprecated-declarations)

if(WIN32)
    target_link_libraries(main PRIVATE ${SDL2_LIBRARIES})
elseif(TARGET PkgConfig::SDL2)
    target_link_libraries(main PRIVATE PkgConfig::SDL2)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(main PRIVATE SDL2::SDL2)
else()
    target_include_directories(main PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(main PRIVATE ${SDL2_LIBRARIES})
endif()
add_custom_target(run COMMAND $<TARGET_FILE:main>)
