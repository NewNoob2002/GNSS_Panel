// Standalone LVGL baseline check.
#include <stdio.h>
#include <stdlib.h>
#include "lvgl.h"

_Static_assert(LVGL_VERSION_MAJOR == 8 && LVGL_VERSION_MINOR == 3 && LVGL_VERSION_PATCH == 11, "LVGL version changed");
_Static_assert(LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 1, "Preserve the MCU RGB565 wire format");

static lv_color_t pixels[126 * 8];
static unsigned flush_count;
static int invalid_area;

static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
    if (!colors || area->x1 < 0 || area->y1 < 0 || area->x2 >= 126 || area->y2 >= 294 || area->x1 > area->x2
        || area->y1 > area->y2)
        invalid_area = 1;
    ++flush_count;
    lv_disp_flush_ready(driver);
}

int main(void) {
    lv_init();
    lv_disp_draw_buf_t draw;
    lv_disp_draw_buf_init(&draw, pixels, NULL, sizeof(pixels) / sizeof(pixels[0]));
    lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = 126;
    driver.ver_res = 294;
    driver.sw_rotate = 1;
    driver.rotated = LV_DISP_ROT_90;
    driver.draw_buf = &draw;
    driver.flush_cb = flush;
    lv_disp_t* display = lv_disp_drv_register(&driver);
    if (!display || lv_disp_get_hor_res(display) != 294 || lv_disp_get_ver_res(display) != 126)
        return EXIT_FAILURE;
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LVGL 8.3.11 / RK3506 P2");
    lv_obj_center(label);
    lv_refr_now(display);
    lv_mem_monitor_t memory;
    lv_mem_monitor(&memory);
    printf("LVGL %d.%d.%d: 294x126 RGB565 swapped, flushes=%u, free=%lu\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR,
           LVGL_VERSION_PATCH, flush_count, (unsigned long)memory.free_size);
    lv_disp_remove(display);
    return flush_count && !invalid_area && lv_mem_test() == LV_RES_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
