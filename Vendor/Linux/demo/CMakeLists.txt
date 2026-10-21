file(GLOB LVGL_BENCHMARK_ASSETS CONFIGURE_DEPENDS
    "${LVGL_DIR}/demos/benchmark/assets/*.c*")

add_executable(lvgl_demo
    lvgl_demo.c
    demo_perf.c
    "${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c"
    ${LVGL_BENCHMARK_ASSETS}
)
target_include_directories(lvgl_demo PRIVATE "${LVGL_DIR}/demos")
target_link_libraries(lvgl_demo PRIVATE lv_display)
target_compile_options(lvgl_demo PRIVATE -Wall -Wextra -Werror)

add_executable(lvgl_demo_smoke
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/lvgl_demo_smoke.c"
    demo_perf.c
    "${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c"
    ${LVGL_BENCHMARK_ASSETS}
)
target_include_directories(lvgl_demo_smoke PRIVATE "${LVGL_DIR}/demos" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(lvgl_demo_smoke PRIVATE lvgl8311)
target_compile_options(lvgl_demo_smoke PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME lvgl_demo_smoke COMMAND lvgl_demo_smoke)
