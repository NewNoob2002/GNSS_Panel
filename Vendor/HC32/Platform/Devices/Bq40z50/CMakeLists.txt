add_library(hc32_${HC32_PREFIX}bq40z50 OBJECT bq40z50.cpp)
add_library(hc32::${HC32_PREFIX}bq40z50 ALIAS hc32_${HC32_PREFIX}bq40z50)
target_include_directories(hc32_${HC32_PREFIX}bq40z50 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}bq40z50 PUBLIC hc32::${HC32_PREFIX}peripherals)
