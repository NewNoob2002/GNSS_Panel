# Unified LVGL v9.6.0 from repository External/lvgl
get_filename_component(LVGL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../External/lvgl" REALPATH)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS "${LVGL_DIR}/src/*.c")

# 三端自有显示/输入端口，软件渲染不编译硬件与第三方后端。
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/draw/(dma2d|espressif|eve|nanovg|nema_gfx|nxp|opengles|renesas|sdl|sifli|vg_lite)/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/drivers/|/libs/")

add_library(lvgl960 STATIC ${LVGL_SOURCES})
target_include_directories(lvgl960 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
    "${LVGL_DIR}"
    "${LVGL_DIR}/include"
)
target_compile_definitions(lvgl960 PUBLIC
    LV_DISABLE_API_MAPPING
    LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h"
    $<$<BOOL:${RK3506_DEBUG_MONITOR}>:__CORE_DEBUG>
)
target_link_libraries(lvgl960 PUBLIC m)

if(RK3506_LVGL_DEMO)
    set(RK3506_LVGL_DEMO_REFRESH_MS 8 CACHE STRING "LVGL demo refresh period in ms")
    set(RK3506_LVGL_DEMO_DRAW_DIVISOR 4 CACHE STRING "LVGL demo draw buffer fraction denominator")
    target_compile_definitions(lvgl960 PUBLIC
        RK3506_LVGL_DEMO=1
        RK3506_LVGL_DEMO_REFRESH_MS=${RK3506_LVGL_DEMO_REFRESH_MS}
        RK3506_LVGL_DEMO_DRAW_DIVISOR=${RK3506_LVGL_DEMO_DRAW_DIVISOR}
        _POSIX_C_SOURCE=200809L
    )
endif()
