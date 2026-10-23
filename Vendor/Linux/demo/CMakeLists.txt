add_executable(lvgl_demo lvgl_demo.c demo_perf.c)
target_link_libraries(lvgl_demo PRIVATE linux::display linux::display_io lvgl::demos)
target_compile_options(lvgl_demo PRIVATE -Wall -Wextra -Werror)

add_executable(lvgl_demo_smoke
    lvgl_demo_smoke.c
    demo_perf.c
)
target_include_directories(lvgl_demo_smoke PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lvgl_demo_smoke PRIVATE lvgl::lvgl lvgl::demos)
target_compile_options(lvgl_demo_smoke PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME lvgl_demo_smoke COMMAND lvgl_demo_smoke)
