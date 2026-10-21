add_library(rm690a0 STATIC
    rm690a0.c
    "${CMAKE_CURRENT_SOURCE_DIR}/../platform/linux_display.c"
)
target_include_directories(rm690a0 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../platform"
)
target_compile_options(rm690a0 PRIVATE -Wall -Wextra -Werror)

add_library(lv_display STATIC lv_display.c)
target_link_libraries(lv_display PUBLIC rm690a0 lvgl8311)
target_compile_options(lv_display PRIVATE -Wall -Wextra -Werror)
