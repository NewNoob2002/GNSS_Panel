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
    hc32_arduino_headers
    hc32_parser
)

if(PRODUCT_HAS_DISPLAY)
    # X1Pro / XBase 共用：带屏入口、共享 LVGL 应用的平台层、显示协议表、显示 HAL、LVGL 内存分配
    add_library(hc32_shared_display OBJECT
        Display/AppPlatform.cpp
        Display/display_main.cpp
        Display/display_protocol.cpp
        Display/HAL_Display.cpp
        Display/lvgl_new.cpp
    )
    target_include_directories(hc32_shared_display PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Display
    )
    target_link_libraries(hc32_shared_display PUBLIC
        hc32_shared_comm
        hc32_simulator_app_headers
        hc32_display_libraries
    )

    # 产品目标通过它取得显示层全部目标文件（OBJECT 库的目标文件不会经 INTERFACE 库传递）
    add_library(hc32_display_stack INTERFACE)
    target_sources(hc32_display_stack INTERFACE
        $<TARGET_OBJECTS:hc32_shared_display>
        $<TARGET_OBJECTS:hc32_simulator_app>
    )
    target_link_libraries(hc32_display_stack INTERFACE
        hc32_shared_display
        hc32_display_libraries
    )
endif()
