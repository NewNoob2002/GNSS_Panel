// 独立 LVGL 版本、缓冲与刷新检查。
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"

_Static_assert(LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR == 6 && LVGL_VERSION_PATCH == 0, "LVGL version changed");
_Static_assert(LV_COLOR_DEPTH == 16, "Preserve the MCU RGB565 wire format");

static uint16_t pixels[126 * 8];
static unsigned flush_count;
static int invalid_area;

static void
flush(lv_display_t* driver, const lv_area_t* area, uint8_t* px_map) {
    uint16_t* colors = (uint16_t*)px_map;
    if (!colors || area->x1 < 0 || area->y1 < 0 || area->x2 >= 294 || area->y2 >= 126 || area->x1 > area->x2
        || area->y1 > area->y2) {
        invalid_area = 1;
    }
    ++flush_count;
    lv_display_flush_ready(driver);
}

int main(void) {
    lv_init();
    lv_display_t* display = lv_display_create(294, 126);
    if (!display) {
        return EXIT_FAILURE;
    }
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    if (!display || lv_display_get_horizontal_resolution(display) != 294
        || lv_display_get_vertical_resolution(display) != 126) {
        return EXIT_FAILURE;
    }
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "LVGL 9.6.0 / RK3506 P2");
    lv_obj_center(label);
    lv_refr_now(display);
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    printf("LVGL %d.%d.%d: 294x126 RGB565 render, flushes=%u, free=%lu\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
           LVGL_VERSION_PATCH, flush_count, (unsigned long)memory.free_size);
    lv_display_delete(display);
    return flush_count && !invalid_area && lv_mem_test() == LV_RESULT_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
