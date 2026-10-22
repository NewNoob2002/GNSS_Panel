add_library(hc32_bq40z50 OBJECT bq40z50.cpp)
add_library(hc32::bq40z50 ALIAS hc32_bq40z50)
target_include_directories(hc32_bq40z50 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_bq40z50 PUBLIC hc32_project_options hc32_platform_headers)
