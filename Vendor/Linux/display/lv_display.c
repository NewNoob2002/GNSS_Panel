#include "lv_display.h"
#include <errno.h>

_Static_assert(LV_COLOR_DEPTH == 16, "RGB565 required");

/** @brief 将失效区域扩展到面板要求的双像素边界。 */
static void
round_area(lv_event_t* event) {
    lv_area_t* area = lv_event_get_param(event);
    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 |= 1;
    area->y2 |= 1;
}

/** @brief 将逻辑横屏 RGB565 旋转到面板坐标，并写为高字节在前的总线数据。 */
static void
rotate_90(const uint16_t* src, int w, int h, uint16_t* dst) {
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint16_t pixel = *src++;
            dst[(w - 1 - x) * h + y] = (uint16_t)((pixel << 8) | (pixel >> 8));
        }
    }
}

/** @brief 同步发送旋转区域，出错后仍归还 LVGL 缓冲供调用方退出。 */
static void
flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
    struct panel_display* port = lv_display_get_user_data(display);
    int w = lv_area_get_width(area), h = lv_area_get_height(area);
    rotate_90((const uint16_t*)pixels, w, h, port->tx_buf);
    if (!port->error
        && rm_write(port->screen, area->y1, RM_HEIGHT - 1 - area->x2, h, w, port->tx_buf, (size_t)w * (size_t)h * 2)
               < 0) {
        port->error = errno;
    }
    lv_display_flush_ready(display);
}

int
lv_display_register(struct panel_display* port, struct Rm690a0* screen) {
    port->screen = screen;
    port->error = 0;
    port->display = lv_display_create(RM_HEIGHT, RM_WIDTH);
    if (!port->display) {
        errno = ENOMEM;
        return -1;
    }
    lv_display_set_color_format(port->display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(port->display, port->buffers[0], port->buffers[1], sizeof(port->buffers[0]),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(port->display, port);
    lv_display_set_flush_cb(port->display, flush);
    lv_display_add_event_cb(port->display, round_area, LV_EVENT_INVALIDATE_AREA, NULL);
    return 0;
}

void lv_display_pattern(void) {
    lv_obj_t* root = lv_screen_active();
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
    lv_obj_set_pos(border, 0, 0);
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
    lv_obj_update_layout(root);
}
