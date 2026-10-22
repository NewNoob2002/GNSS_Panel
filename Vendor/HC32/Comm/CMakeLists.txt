add_library(hc32_${HC32_PREFIX}protocol OBJECT panel_protocol.cpp)
add_library(hc32::${HC32_PREFIX}protocol ALIAS hc32_${HC32_PREFIX}protocol)
target_include_directories(hc32_${HC32_PREFIX}protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}protocol PUBLIC
    hc32::${HC32_PREFIX}state hc32::parser ${HC32_PORT})

add_library(hc32_${HC32_PREFIX}comm OBJECT panel_protocol.cpp slave_i2c.cpp)
add_library(hc32::${HC32_PREFIX}comm ALIAS hc32_${HC32_PREFIX}comm)
target_include_directories(hc32_${HC32_PREFIX}comm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}comm PUBLIC
    hc32::${HC32_PREFIX}state hc32::parser ${HC32_PORT})
if(NOT HC32_PORTABLE)
    target_link_libraries(hc32_${HC32_PREFIX}comm PUBLIC hc32::hal)
endif()
