#ifndef PANEL_DISPLAY_H
#define PANEL_DISPLAY_H
#include "lvgl.h"
#include "rm690a0.h"

#ifndef RK3506_LVGL_DEMO_DRAW_DIVISOR
#define RK3506_LVGL_DEMO_DRAW_DIVISOR 4
#endif

struct panel_display {
    struct Rm690a0* screen;
    lv_display_t* display;
    uint16_t buffers[2][(RM_WIDTH * RM_HEIGHT / RK3506_LVGL_DEMO_DRAW_DIVISOR + 1) & ~1U];
    uint16_t tx_buf[RM_WIDTH * RM_HEIGHT / RK3506_LVGL_DEMO_DRAW_DIVISOR];
    int error;
};

int lv_display_register(struct panel_display* port, struct Rm690a0* screen);
void lv_display_pattern(void);
#endif
