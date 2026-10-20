# HC32 自有构建文件（不随 scripts/sync_shared_app.sh 覆盖）。
# 本目录其余文件是 ATK_RK3506 application/App 的原样副本，来源提交见 UPSTREAM。
# Status/ 是 RK3506 的模拟实现（DemoStatus、模拟 system_backend），HC32 由 Shared/Display/AppPlatform.cpp 提供平台实现。
file(GLOB_RECURSE SHARED_APP_SOURCES CONFIGURE_DEPENDS
    *.c
    *.cpp
)
list(FILTER SHARED_APP_SOURCES EXCLUDE REGEX "/Status/")

add_library(hc32_simulator_app_headers INTERFACE)
target_include_directories(hc32_simulator_app_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(hc32_simulator_app_headers INTERFACE
    hc32_project_options
    hc32_libraries_headers
    hc32_display_libraries
)

add_library(hc32_simulator_app OBJECT
    ${SHARED_APP_SOURCES}
)
target_link_libraries(hc32_simulator_app PUBLIC hc32_simulator_app_headers)
