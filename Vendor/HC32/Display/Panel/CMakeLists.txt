add_library(hc32_${HC32_PREFIX}panel OBJECT PanelRm690a0.cpp board_display.cpp)
add_library(hc32::${HC32_PREFIX}panel ALIAS hc32_${HC32_PREFIX}panel)
target_include_directories(hc32_${HC32_PREFIX}panel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}panel PUBLIC hc32::${HC32_PREFIX}peripherals
    hc32::${HC32_PREFIX}config_headers ${HC32_PORT})
