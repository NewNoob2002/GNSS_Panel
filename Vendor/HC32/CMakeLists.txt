cmake_minimum_required(VERSION 3.20)

project(HC32F460xE_Arduino LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." REALPATH)

# 同一 MCU 的三个产品共用平台层与通信层，每次配置只构建一个产品。
set(HC32_PRODUCTS X1 X1Pro XBase)
set(PRODUCT "X1Pro" CACHE STRING "Firmware product to build: X1, X1Pro or XBase")
set_property(CACHE PRODUCT PROPERTY STRINGS ${HC32_PRODUCTS})
if(NOT PRODUCT IN_LIST HC32_PRODUCTS)
    message(FATAL_ERROR "Unknown PRODUCT '${PRODUCT}', expected one of: ${HC32_PRODUCTS}")
endif()
set(PRODUCT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Products/${PRODUCT})
include(${PRODUCT_DIR}/product.cmake)
message(STATUS "Product: ${PRODUCT} ${PRODUCT_VERSION} (display: ${PRODUCT_HAS_DISPLAY})")

set(PROJECT_VERSION ${PRODUCT_VERSION})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Config/mcu_version_config.h.in
    ${CMAKE_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
    @ONLY
)
set(TARGET_NAME ${PRODUCT_IMAGE_PREFIX}_${PRODUCT_VERSION})
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/HC32F460xE.ld)

include(${REPO_ROOT}/cmake/ProjectOptions.cmake)

add_subdirectory(Platform)
add_subdirectory(Arduino)
add_subdirectory(Libraries)

# 三产品共用：与 Linux 主控的 I2C 从机通信及面板帧协议核心
add_library(hc32_shared_comm OBJECT
    Comm/slave_i2c.cpp
    Comm/panel_protocol.cpp
)
target_include_directories(hc32_shared_comm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Comm
)
target_link_libraries(hc32_shared_comm PUBLIC
    hc32_project_options
    hc32_platform_headers
    hc32_parser
)

if(PRODUCT_HAS_DISPLAY)
    add_library(app_build_options INTERFACE)
    target_link_libraries(app_build_options INTERFACE hc32_project_options)
    add_library(app_lvgl INTERFACE)
    target_link_libraries(app_lvgl INTERFACE hc32_lvgl)
    add_subdirectory(${REPO_ROOT}/App ${CMAKE_CURRENT_BINARY_DIR}/App)

    # 共享应用的 HC32 平台层：入口 + 各方向的 *Bridge + 初始化步骤
    file(GLOB APP_PLATFORM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Display/AppPlatform/*.cpp)
    # X1Pro / XBase 共用：带屏入口、共享 LVGL 应用的平台层、显示协议表、显示 HAL、LVGL 内存分配与显示端口
    add_library(hc32_shared_display OBJECT
        ${APP_PLATFORM_SOURCES}
        Display/display_main.cpp
        Display/display_protocol.cpp
        Display/HAL_Display.cpp
        Display/lvgl_new.cpp
        Display/lv_port.cpp
        Display/lv_port_disp.cpp
    )
    target_include_directories(hc32_shared_display PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Display
    )
    target_link_libraries(hc32_shared_display PRIVATE hc32_arduino_headers)
    target_link_libraries(hc32_shared_display PUBLIC
        hc32_shared_comm
        app_ui
        hc32_display_libraries
    )

    # 产品目标通过它取得显示层全部目标文件（OBJECT 库的目标文件不会经 INTERFACE 库传递）
    add_library(hc32_display_stack INTERFACE)
    target_sources(hc32_display_stack INTERFACE
        $<TARGET_OBJECTS:hc32_shared_display>
    )
    target_link_libraries(hc32_display_stack INTERFACE
        hc32_shared_display
        hc32_display_libraries
    )
endif()
add_subdirectory(${PRODUCT_DIR})

add_executable(${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_link_libraries(${TARGET_NAME} PRIVATE
    hc32_platform
    hc32_arduino
    hc32_libraries
    hc32_shared_comm
    hc32_product
)
target_link_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--defsym=FLASH_ORIGIN=0x00008000
    -Wl,--defsym=FLASH_LENGTH=0x00072000
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/generated/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_BINARY_DIR}/generated/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generating HEX/BIN and size report"
)
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})
