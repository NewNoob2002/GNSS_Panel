# Config 仅发布本目录与所选产品的配置契约；State 仅取 mcu_define.h。
add_library(hc32_${HC32_PREFIX}config_headers INTERFACE)
add_library(hc32::${HC32_PREFIX}config_headers ALIAS hc32_${HC32_PREFIX}config_headers)
target_include_directories(hc32_${HC32_PREFIX}config_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})
# 产品头由产品目录的目标公开。
set(HC32_CONFIG_ONLY TRUE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../Products/${HC32_PRODUCT}
    ${CMAKE_CURRENT_BINARY_DIR}/product_headers EXCLUDE_FROM_ALL)
target_link_libraries(hc32_${HC32_PREFIX}config_headers INTERFACE
    proj::options hc32::${HC32_PREFIX}product_config)
if(NOT HC32_PORTABLE OR PORTABLE_DRIVERS)
    add_library(hc32_${HC32_PREFIX}board OBJECT board_peripherals.cpp)
    add_library(hc32::${HC32_PREFIX}board ALIAS hc32_${HC32_PREFIX}board)
    target_include_directories(hc32_${HC32_PREFIX}board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(hc32_${HC32_PREFIX}board PUBLIC
        hc32::${HC32_PREFIX}config_headers hc32::${HC32_PREFIX}peripherals)
endif()
