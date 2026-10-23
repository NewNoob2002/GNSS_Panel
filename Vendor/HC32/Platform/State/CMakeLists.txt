add_library(hc32_${HC32_PREFIX}state OBJECT SystemState.cpp)
add_library(hc32::${HC32_PREFIX}state ALIAS hc32_${HC32_PREFIX}state)
target_include_directories(hc32_${HC32_PREFIX}state PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}state PUBLIC proj::options
    hc32::${HC32_PREFIX}config_headers)
