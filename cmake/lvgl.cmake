include_guard(GLOBAL)
get_filename_component(_PROJECT_LVGL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../External/lvgl" ABSOLUTE)

# Add upstream LVGL with a platform configuration and optional DEMOS.
# 用法：project_add_lvgl(<lv_conf.h 绝对路径> [DEMOS])；每次配置只选择一个平台。
function(project_add_lvgl conf_path)
    cmake_parse_arguments(LVGL "DEMOS" "" "" ${ARGN})
    # 保持原四处 LVGL 库的 STATIC 类型，通过缓存选项兼容上游的旧策略版本。
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "LVGL 使用静态库" FORCE)
    set(LV_BUILD_CONF_PATH "${conf_path}" CACHE PATH "平台 LVGL 配置" FORCE)
    foreach(option CONFIG_LV_BUILD_EXAMPLES CONFIG_LV_USE_THORVG LV_BUILD_INSTALL
            LV_USE_FIND_PACKAGE LV_USE_PKG_CONFIG LV_FETCH_DEPENDENCIES LV_BUILD_TESTS)
        set(${option} OFF CACHE BOOL "由仓库统一配置 LVGL" FORCE)
    endforeach()
    set(CONFIG_LV_BUILD_DEMOS ${LVGL_DEMOS} CACHE BOOL "Linux benchmark 演示" FORCE)
    add_subdirectory("${_PROJECT_LVGL_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/lvgl")
    # 应用和平台保持 v9 原生 API；LVGL 内部使用上游默认编译定义。
    target_compile_definitions(lvgl INTERFACE LV_DISABLE_API_MAPPING)
endfunction()
