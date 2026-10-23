add_library(hc32_${HC32_PREFIX}mp2762a OBJECT mp2762a.cpp)
add_library(hc32::${HC32_PREFIX}mp2762a ALIAS hc32_${HC32_PREFIX}mp2762a)
target_include_directories(hc32_${HC32_PREFIX}mp2762a PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}mp2762a PUBLIC hc32::${HC32_PREFIX}peripherals)
