add_library(hc32_libraries_headers INTERFACE)
target_include_directories(hc32_libraries_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(hc32_bq40z50 OBJECT
    Bq40z50/bq40z50.cpp
)
target_include_directories(hc32_bq40z50 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Bq40z50
)
target_link_libraries(hc32_bq40z50 PUBLIC
    hc32_project_options
    hc32_platform_headers
)

add_library(hc32_millis_task_manager OBJECT
    MillisTaskManager/MillisTaskManager.cpp
)
target_include_directories(hc32_millis_task_manager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/MillisTaskManager
)
target_link_libraries(hc32_millis_task_manager PUBLIC
    hc32_project_options
    hc32_platform_headers
)

add_library(hc32_mp2762a OBJECT
    Mp2762a/mp2762a.cpp
)
target_include_directories(hc32_mp2762a PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Mp2762a
)
target_link_libraries(hc32_mp2762a PUBLIC
    hc32_project_options
    hc32_platform_headers
)

file(GLOB_RECURSE PARSER_SOURCES CONFIGURE_DEPENDS
    Parser/src/*.c
    Parser/src/*.cpp
)
add_library(hc32_parser OBJECT
    ${PARSER_SOURCES}
)
target_include_directories(hc32_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Parser/include
)
target_link_libraries(hc32_parser PUBLIC
    hc32_project_options
)

add_library(hc32_segger_rtt OBJECT
    SEGGER_RTT/SEGGER_RTT.c
    SEGGER_RTT/SEGGER_RTT_printf.c
)
target_include_directories(hc32_segger_rtt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER_RTT
)
target_link_libraries(hc32_segger_rtt PUBLIC
    hc32_project_options
)

add_library(hc32_easylogger_headers INTERFACE)
target_include_directories(hc32_easylogger_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/easylogger/inc
)
add_library(hc32_easylogger OBJECT
    easylogger/src/elog.c
    easylogger/src/elog_async.c
    easylogger/src/elog_buf.c
    easylogger/src/elog_utils.c
    easylogger/port/elog_port.c
)
target_link_libraries(hc32_easylogger PUBLIC hc32_easylogger_headers)
target_include_directories(hc32_easylogger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER_RTT
)
target_link_libraries(hc32_easylogger PUBLIC
    hc32_project_options
    hc32_platform_headers
)

# 三产品共用的设备驱动与工具库
add_library(hc32_libraries INTERFACE)
target_sources(hc32_libraries INTERFACE
    $<TARGET_OBJECTS:hc32_bq40z50>
    $<TARGET_OBJECTS:hc32_millis_task_manager>
    $<TARGET_OBJECTS:hc32_mp2762a>
    $<TARGET_OBJECTS:hc32_parser>
    $<TARGET_OBJECTS:hc32_segger_rtt>
    $<TARGET_OBJECTS:hc32_easylogger>
)
target_link_libraries(hc32_libraries INTERFACE
    hc32_libraries_headers
    hc32_bq40z50
    hc32_millis_task_manager
    hc32_mp2762a
    hc32_parser
    hc32_segger_rtt
    hc32_easylogger
)

if(PRODUCT_HAS_DISPLAY)
# 带屏产品：LVGL；原生面板由显示目标编译，SPI 主机由平台目标编译。

set(EXTERNAL_DIR ${REPO_ROOT}/External)
set(LVGL_DIR ${EXTERNAL_DIR}/lvgl)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS "${LVGL_DIR}/src/*.c")

# 三端自有显示/输入端口，软件渲染不编译硬件与第三方后端。
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/draw/(dma2d|espressif|eve|nanovg|nema_gfx|nxp|opengles|renesas|sdl|sifli|vg_lite)/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/drivers/|/libs/")

# v9 渲染器在 -Og 下加上正式资源和浮点显示会超出 456 KiB；仅第三方库按体积优化，保留调试符号。
set_property(SOURCE ${LVGL_SOURCES} APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:Debug>:-Os>)
add_library(hc32_lvgl STATIC
    ${LVGL_SOURCES}
)
target_include_directories(hc32_lvgl PUBLIC
    ${EXTERNAL_DIR}
    ${LVGL_DIR}
    ${LVGL_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Display/Lvgl
)
target_compile_definitions(hc32_lvgl PUBLIC
    LV_DISABLE_API_MAPPING
    LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../Display/Lvgl/lv_conf.h"
)
target_link_libraries(hc32_lvgl PUBLIC
    hc32_project_options
    hc32_platform_headers
)

add_library(hc32_display_libraries INTERFACE)
target_link_libraries(hc32_display_libraries INTERFACE
    hc32_libraries_headers
    hc32_lvgl
)
else()
# 无屏产品：MultiButton 按键状态机（第三方库，原样引入自 X1_Panel_dev）
add_library(hc32_multi_button OBJECT
    MultiButton/multi_button.c
)
target_include_directories(hc32_multi_button PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiButton
)
target_link_libraries(hc32_multi_button PUBLIC
    hc32_project_options
)
endif()
