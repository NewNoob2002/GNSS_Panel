# X1 无屏产品：自有入口、按键、电量补偿与协议表；不链接任何显示库。
add_library(hc32_product_x1 OBJECT
    main.cpp
    x1_keys.cpp
    x1_protocol.cpp
    x1_soc_compensation.cpp
)
target_link_libraries(hc32_product_x1 PUBLIC
    hc32_shared_comm
    hc32::multibutton
    hc32_platform_headers
    hc32_libraries_headers
)

add_library(hc32_product INTERFACE)
target_sources(hc32_product INTERFACE
    $<TARGET_OBJECTS:hc32_product_x1>
    $<TARGET_OBJECTS:hc32::multibutton>
)
