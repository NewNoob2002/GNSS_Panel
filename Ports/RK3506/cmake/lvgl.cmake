# Unified LVGL 8.3.11 from repository Libraries/lvgl
get_filename_component(LVGL_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../Libraries/lvgl" REALPATH)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS "${LVGL_DIR}/src/*.c")

add_library(lvgl8311 STATIC ${LVGL_SOURCES})
target_include_directories(lvgl8311 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
    "${LVGL_DIR}"
)
target_compile_definitions(lvgl8311 PUBLIC
    LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/config/lv_conf.h
    $<$<BOOL:${RK3506_DEBUG_MONITOR}>:__CORE_DEBUG>
)
target_link_libraries(lvgl8311 PUBLIC m)

if(RK3506_LVGL_DEMO)
    set(RK3506_LVGL_DEMO_REFRESH_MS 8 CACHE STRING "LVGL demo refresh period in ms")
    set(RK3506_LVGL_DEMO_DRAW_DIVISOR 4 CACHE STRING "LVGL demo draw buffer fraction denominator")
    target_compile_definitions(lvgl8311 PUBLIC
        RK3506_LVGL_DEMO=1
        RK3506_LVGL_DEMO_REFRESH_MS=${RK3506_LVGL_DEMO_REFRESH_MS}
        RK3506_LVGL_DEMO_DRAW_DIVISOR=${RK3506_LVGL_DEMO_DRAW_DIVISOR}
        _POSIX_C_SOURCE=200809L
    )
endif()
