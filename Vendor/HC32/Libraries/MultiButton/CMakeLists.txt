add_library(hc32_multi_button OBJECT multi_button.c)
add_library(hc32::multibutton ALIAS hc32_multi_button)
target_include_directories(hc32_multi_button PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_multi_button PUBLIC hc32_project_options)
