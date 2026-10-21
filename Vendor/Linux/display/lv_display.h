#ifndef LV_DISPLAY_H
#define LV_DISPLAY_H
#include "lvgl.h"
#include "rm690a0.h"

#ifndef RK3506_LVGL_DEMO_DRAW_DIVISOR
#define RK3506_LVGL_DEMO_DRAW_DIVISOR 4
#endif

struct lv_display {
    struct Rm690a0* screen;
    lv_disp_t* display;
    lv_disp_drv_t driver;
    lv_disp_draw_buf_t draw;
    lv_color_t buffers[2][RM_WIDTH * RM_HEIGHT / RK3506_LVGL_DEMO_DRAW_DIVISOR];
    int error;
};
int lv_display_register(struct lv_display* port, struct Rm690a0* screen);
void lv_display_pattern(void);
#endif
