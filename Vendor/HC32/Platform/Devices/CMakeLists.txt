add_subdirectory(Bq40z50)
add_subdirectory(Mp2762a)
add_library(hc32_${HC32_PREFIX}devices INTERFACE)
add_library(hc32::${HC32_PREFIX}devices ALIAS hc32_${HC32_PREFIX}devices)
target_link_libraries(hc32_${HC32_PREFIX}devices INTERFACE
    hc32::${HC32_PREFIX}bq40z50 hc32::${HC32_PREFIX}mp2762a)
target_sources(hc32_${HC32_PREFIX}devices INTERFACE
    $<TARGET_OBJECTS:hc32::${HC32_PREFIX}bq40z50>
    $<TARGET_OBJECTS:hc32::${HC32_PREFIX}mp2762a>)
