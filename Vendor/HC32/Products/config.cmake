# 产品公开目录仅包含本产品开关；版本头保留原生成位置。
add_library(hc32_${HC32_PREFIX}product_config INTERFACE)
add_library(hc32::${HC32_PREFIX}product_config ALIAS hc32_${HC32_PREFIX}product_config)
target_include_directories(hc32_${HC32_PREFIX}product_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated/Platform/Config)
