add_library(hc32_mp2762a OBJECT mp2762a.cpp)
add_library(hc32::mp2762a ALIAS hc32_mp2762a)
target_include_directories(hc32_mp2762a PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_mp2762a PUBLIC hc32_project_options hc32_platform_headers)
