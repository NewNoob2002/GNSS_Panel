add_library(hc32_segger_rtt OBJECT SEGGER_RTT.c SEGGER_RTT_printf.c)
add_library(hc32::segger_rtt ALIAS hc32_segger_rtt)
target_include_directories(hc32_segger_rtt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_segger_rtt PUBLIC proj::options)
