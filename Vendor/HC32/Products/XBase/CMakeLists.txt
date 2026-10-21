# XBase 只使用共用显示业务，产品差异全部在 product_config.h。
add_library(hc32_product INTERFACE)
target_link_libraries(hc32_product INTERFACE hc32_display_stack)
