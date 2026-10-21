#include "lv_display.h"
#include <errno.h>

_Static_assert(LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 1, "RGB565 wire order required");

static void round_area(lv_disp_drv_t* driver, lv_area_t* area) {
    (void)driver;
    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 |= 1;
    area->y2 |= 1;
}

static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels) {
    struct lv_display* port = driver->user_data;
    int w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
    if (!port->error && rm_write(port->screen, area->x1, area->y1, w, h, pixels, (size_t)w * (size_t)h * 2) < 0)
        port->error = errno;
    /* On failure release LVGL's buffer too, then the owner exits/reinitializes. */
    lv_disp_flush_ready(driver);
}

int lv_display_register(struct lv_display* port, struct Rm690a0* screen) {
    port->screen = screen;
    port->error = 0;
    lv_disp_draw_buf_init(&port->draw, port->buffers[0], port->buffers[1],
                          RM_WIDTH * RM_HEIGHT / RK3506_LVGL_DEMO_DRAW_DIVISOR);
    lv_disp_drv_init(&port->driver);
    port->driver.hor_res = RM_WIDTH;
    port->driver.ver_res = RM_HEIGHT;
    port->driver.sw_rotate = 1;
    port->driver.rotated = LV_DISP_ROT_90;
    port->driver.draw_buf = &port->draw;
    port->driver.flush_cb = flush;
    port->driver.rounder_cb = round_area;
    port->driver.user_data = port;
    port->display = lv_disp_drv_register(&port->driver);
    if (!port->display) {
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

void lv_display_pattern(void) {
    lv_obj_t* root = lv_scr_act();
    lv_obj_remove_style_all(root);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    static const uint32_t colors[] = {0xff0000, 0x00ff00, 0x0000ff, 0xffffff, 0x000000};
    for (unsigned i = 0; i < 5; ++i) {
        lv_obj_t* bar = lv_obj_create(root);
        lv_obj_remove_style_all(bar);
        lv_obj_set_pos(bar, 2 + (int)i * 58, 2);
        lv_obj_set_size(bar, 58, 122);
        lv_obj_set_style_bg_color(bar, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    }
    lv_obj_t* border = lv_obj_create(root);
    lv_obj_remove_style_all(border);
    lv_obj_set_size(border, 294, 126);
    lv_obj_set_style_border_width(border, 2, 0);
    lv_obj_set_style_border_color(border, lv_color_hex(0xffff00), 0);
    static const lv_align_t align[] = {LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT, LV_ALIGN_BOTTOM_LEFT,
                                       LV_ALIGN_BOTTOM_RIGHT};
    static const char* names[] = {"1 TL", "2 TR", "3 BL", "4 BR"};
    for (unsigned i = 0; i < 4; ++i) {
        lv_obj_t* label = lv_label_create(root);
        lv_label_set_text(label, names[i]);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffff00), 0);
        lv_obj_set_style_bg_color(label, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
        lv_obj_align(label, align[i], i % 2 ? -4 : 4, i / 2 ? -4 : 4);
    }
}
