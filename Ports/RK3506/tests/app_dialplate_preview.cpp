#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "Pages/Dialplate/DialplateView.h"
#include "Resource/ResourcePool.h"
#include "Utils/PageStyle.h"

constexpr int width_px = 294;
constexpr int height_px = 126;
static lv_color_t frame_pixels[width_px * height_px];
static lv_color_t draw_pixels[width_px * 20];
static unsigned flush_count;

static void advance(unsigned ms) {
    for (unsigned elapsed = 0; elapsed < ms; elapsed += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

static void check_count(NumberFlow* number, unsigned value) {
    auto* cont = number->get_cont();
    assert(lv_obj_get_child_cnt(cont) == 3);
    const unsigned digits = value >= 100 ? 3 : value >= 10 ? 2 : 1;
    const auto* font = lv_obj_get_style_text_font(lv_obj_get_child(lv_obj_get_child(cont, 2), 0), 0);
    assert(lv_obj_get_width(cont) == static_cast<int>(digits * lv_font_get_glyph_width(font, '0', '\0')));
    unsigned divisor = 100;
    for (int digit = 0; digit < 3; ++digit, divisor /= 10) {
        auto* cell = lv_obj_get_child(cont, digit);
        auto* strip = lv_obj_get_child(cell, 0);
        const auto* font = lv_obj_get_style_text_font(strip, 0);
        assert(lv_obj_has_flag(cell, LV_OBJ_FLAG_HIDDEN) == (digit < 2 && value < divisor));
        assert(lv_obj_get_y(strip) == -static_cast<int>(value / divisor % 10) * font->line_height
               || lv_obj_has_flag(cell, LV_OBJ_FLAG_HIDDEN));
    }
}

static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels) {
    assert(area->x1 >= 0 && area->y1 >= 0 && area->x2 < width_px && area->y2 < height_px);
    const int row_width_px = area->x2 - area->x1 + 1;
    for (int y = area->y1; y <= area->y2; ++y)
        std::memcpy(&frame_pixels[y * width_px + area->x1], &pixels[(y - area->y1) * row_width_px],
                    (std::size_t)row_width_px * sizeof(lv_color_t));
    ++flush_count;
    lv_disp_flush_ready(driver);
}

static bool save_ppm(const char* path) {
    FILE* file = std::fopen(path, "wb");
    if (!file)
        return false;
    bool ok = std::fprintf(file, "P6\n%d %d\n255\n", width_px, height_px) > 0;
    for (const lv_color_t color : frame_pixels) {
        const uint8_t rgb[] = {(uint8_t)(LV_COLOR_GET_R(color) * 255 / 31), (uint8_t)(LV_COLOR_GET_G(color) * 255 / 63),
                               (uint8_t)(LV_COLOR_GET_B(color) * 255 / 31)};
        if (ok && std::fwrite(rgb, 1, sizeof(rgb), file) != sizeof(rgb))
            ok = false;
    }
    return std::fclose(file) == 0 && ok;
}

int main(int argc, char** argv) {
    if (argc > 2)
        return 2;
    lv_init();
    lv_disp_draw_buf_t draw;
    lv_disp_draw_buf_init(&draw, draw_pixels, nullptr, width_px * 20);
    lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = width_px;
    driver.ver_res = height_px;
    driver.draw_buf = &draw;
    driver.flush_cb = flush;
    lv_disp_t* display = lv_disp_drv_register(&driver);
    assert(display);

    resource_pool::init();
    assert(resource_pool::get_image("settings") && resource_pool::get_image("ui_satellite_large"));
    assert(resource_pool::get_font("rajdhaniBold_40") && resource_pool::get_font("dialplate"));
    page::DialplateView view{};
    view.create(lv_scr_act());
    assert(view.ui.top_info.cont && view.ui.btn_cont.btn_map && view.ui.btn_cont.btn_rec);
    view.appear_anim_start();
    advance(600);
    const std::array names{"ui_work", "ui_record", "ui_info", "ui_settings"};
    const std::array buttons{view.ui.btn_cont.btn_map, view.ui.btn_cont.btn_rec, view.ui.btn_cont.btn_menu,
                             view.ui.btn_cont.btn_shutdown};
    assert(lv_obj_get_child_cnt(lv_scr_act()) == 2); // 首页没有焦点文字说明。
    for (unsigned i = 0; i < buttons.size(); ++i) {
        const auto* image = static_cast<const lv_img_dsc_t*>(resource_pool::get_image(names[i]));
        assert(image && image->header.w == 16 && image->header.h == 16);
        assert(image->header.cf == LV_IMG_CF_ALPHA_8BIT && image->data_size == 256);
        unsigned peak = 0;
        for (unsigned pixel = 0; pixel < image->data_size; ++pixel)
            if (image->data[pixel] > peak)
                peak = image->data[pixel];
        assert(peak == 255);
        assert(lv_obj_get_style_bg_img_src(buttons[i], 0) == image);
        assert(lv_obj_get_style_bg_img_recolor(buttons[i], 0).full == lv_color_white().full);
        assert(lv_obj_get_style_bg_img_recolor_opa(buttons[i], 0) == LV_OPA_COVER);
        assert(lv_obj_get_width(buttons[i]) == 56 && lv_obj_get_height(buttons[i]) == 32);
    }
    const std::array controls{view.ui.btn_cont.btn_map, view.ui.btn_cont.btn_rec, view.ui.btn_cont.btn_menu,
                              view.ui.btn_cont.btn_shutdown, view.ui.top_info.icon_satellite};
    auto* group = lv_group_create();
    assert(group);
    for (auto* control : controls)
        lv_group_add_obj(group, control);
    advance(250);
    // 滚轮可在一帧内连续切换；旧焦点的橙框必须立即消失。
    for (unsigned step = 0; step < 100; ++step) {
        if (step % 3 == 0)
            lv_group_focus_prev(group);
        else
            lv_group_focus_next(group);
        advance(step % 4 * 10);
        auto* focused = lv_group_get_focused(group);
        for (auto* control : controls) {
            const bool selected = control == focused;
            assert(lv_obj_has_state(control, LV_STATE_FOCUSED) == selected);
            assert((lv_obj_get_style_border_color(control, 0).full == page::style::kAccent.full) == selected);
            assert(lv_obj_get_style_border_width(control, 0) == (selected ? 2 : 1));
        }
    }
    lv_group_del(group);
    const auto& top = view.ui.top_info;
    auto* used = top.satellite_used->get_cont();
    auto* tracked = top.satellite_tracked->get_cont();
    assert(lv_obj_has_flag(used, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(tracked, LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(top.satellite_missing, LV_OBJ_FLAG_HIDDEN));
    DataProc::DeviceSnapshot device{};
    device.satellites_valid = true;
    const struct {
        unsigned used, tracked;
    } counts[] = {{0, 0}, {9, 14}, {18, 26}, {10, 100}, {99, 255}, {100, 9}, {255, 255}, {999, 999}, {9, 1}};
    for (const auto& count : counts) {
        device.satellites_used = count.used;
        device.satellites_tracked = count.tracked;
        view.update(device);
        advance(400);
        check_count(top.satellite_used, count.used);
        check_count(top.satellite_tracked, count.tracked);
        assert(used->coords.x1 > top.icon_satellite->coords.x2);
        assert(used->coords.x2 < top.satellite_separator->coords.x1);
        assert(top.satellite_separator->coords.x2 < tracked->coords.x1);
        assert(tracked->coords.x2 < top.radio_card->coords.x1);
        assert(used->coords.y2 == tracked->coords.y2);
        const auto* separator_font = lv_obj_get_style_text_font(top.satellite_separator, 0);
        assert(top.satellite_separator->coords.y2 - separator_font->base_line == used->coords.y2);
        assert(used->coords.y2 < top.cont->coords.y2);
        assert(used->coords.y1 - top.cont->coords.y1 >= 10);
        assert(top.satellite_separator->coords.x1 - used->coords.x2 == 3);
        assert(tracked->coords.x1 - top.satellite_separator->coords.x2 == 3);
        auto* row = lv_obj_get_parent(used);
        const auto left_space = used->coords.x1 - row->coords.x1;
        const auto right_space = row->coords.x2 - tracked->coords.x2;
        assert(left_space >= 0 && right_space >= 0 && left_space - right_space >= -1 && left_space - right_space <= 1);
    }
    for (unsigned i = 0; i < lv_obj_get_child_cnt(top.cont); ++i) {
        auto* child = lv_obj_get_child(top.cont, static_cast<int32_t>(i));
        if (lv_obj_check_type(child, &lv_label_class)) {
            assert(std::strcmp(lv_label_get_text(child), "USED") != 0);
            assert(std::strcmp(lv_label_get_text(child), "TRK") != 0);
        }
    }
    device.satellites_valid = false;
    view.update(device);
    assert(lv_obj_has_flag(used, LV_OBJ_FLAG_HIDDEN) && lv_obj_has_flag(tracked, LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(top.satellite_missing, LV_OBJ_FLAG_HIDDEN));
    device.satellites_valid = true;
    device.satellites_used = 18;
    device.satellites_tracked = 26;
    device.work_valid = true;
    device.work_mode = DataProc::WorkMode::Base;
    device.radio_on = true;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "BASE") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "TX") == 0);
    assert(lv_obj_get_style_text_color(top.mode_label, 0).full == page::style::kInfo.full);
    device.radio_on = false;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "OFF") == 0);
    device.work_valid = false;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "--") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "--") == 0);
    device.work_valid = true;
    device.work_mode = DataProc::WorkMode::Rover;
    device.radio_on = true;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "ROVER") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "RX") == 0);
    assert(lv_obj_get_style_text_color(top.mode_label, 0).full == page::style::kSuccess.full);
    advance(400);
    assert(!lv_obj_has_flag(used, LV_OBJ_FLAG_HIDDEN) && !lv_obj_has_flag(tracked, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(top.satellite_missing, LV_OBJ_FLAG_HIDDEN));
    lv_refr_now(display);
    assert(flush_count && lv_obj_get_child_cnt(lv_scr_act()) >= 2);
    unsigned nonblack_pixels = 0;
    for (const lv_color_t color : frame_pixels)
        nonblack_pixels += LV_COLOR_GET_R(color) || LV_COLOR_GET_G(color) || LV_COLOR_GET_B(color);
    assert(nonblack_pixels > 100);

    device.record_valid = true;
    for (bool recording : {true, false}) {
        device.recording = recording;
        view.update(device);
        assert(lv_obj_get_style_bg_img_src(view.ui.btn_cont.btn_rec, 0) == resource_pool::get_image("ui_record"));
        assert(lv_obj_get_style_border_color(view.ui.btn_cont.btn_rec, 0).full
               == (recording ? page::style::kDanger : page::style::kBorder).full);
    }
    const bool saved = argc == 1 || save_ppm(argv[1]);
    view.destroy();
    lv_obj_clean(lv_scr_act());
    if (!saved) {
        std::perror("save preview");
        return 1;
    }
    return 0;
}
