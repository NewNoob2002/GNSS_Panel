#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "UI/Dialplate/DialplateView.h"
#include "UI/Resource/ResourcePool.h"
#include "UI/Common/PageStyle.h"

/** @brief 通过公开 API 读取对象的屏幕绝对坐标供布局断言使用。 */
static lv_area_t
object_coords(const lv_obj_t* object) {
    lv_area_t area;
    lv_obj_get_coords(object, &area);
    return area;
}

constexpr int width_px = 294;
constexpr int height_px = 126;
static uint16_t frame_pixels[width_px * height_px];
static uint16_t draw_pixels[width_px * 20];
static unsigned flush_count;

static void advance(unsigned ms) {
    for (unsigned elapsed = 0; elapsed < ms; elapsed += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

static void check_count(NumberFlow* number, unsigned value) {
    auto* cont = number->get_cont();
    assert(lv_obj_get_child_count(cont) == 3);
    const unsigned digits = value >= 100 ? 3 : value >= 10 ? 2 : 1;
    const auto* font = lv_obj_get_style_text_font(lv_obj_get_child(lv_obj_get_child(cont, 2), 0), LV_PART_MAIN);
    assert(lv_obj_get_width(cont) == static_cast<int>(digits * lv_font_get_glyph_width(font, '0', '\0')));
    unsigned divisor = 100;
    for (int digit = 0; digit < 3; ++digit, divisor /= 10) {
        auto* cell = lv_obj_get_child(cont, digit);
        auto* strip = lv_obj_get_child(cell, 0);
        const auto* font = lv_obj_get_style_text_font(strip, LV_PART_MAIN);
        assert(lv_obj_is_hidden(cell) == (digit < 2 && value < divisor));
        assert(lv_obj_get_y(strip) == -static_cast<int>(value / divisor % 10) * font->line_height
               || lv_obj_is_hidden(cell));
    }
}

static void
flush(lv_display_t* driver, const lv_area_t* area, uint8_t* px_map) {
    uint16_t* pixels = (uint16_t*)px_map;
    assert(area->x1 >= 0 && area->y1 >= 0 && area->x2 < width_px && area->y2 < height_px);
    const int row_width_px = area->x2 - area->x1 + 1;
    for (int y = area->y1; y <= area->y2; ++y)
        std::memcpy(&frame_pixels[y * width_px + area->x1], &pixels[(y - area->y1) * row_width_px],
                    (std::size_t)row_width_px * sizeof(uint16_t));
    ++flush_count;
    lv_display_flush_ready(driver);
}

static bool save_ppm(const char* path) {
    FILE* file = std::fopen(path, "wb");
    if (!file)
        return false;
    bool ok = std::fprintf(file, "P6\n%d %d\n255\n", width_px, height_px) > 0;
    for (const uint16_t color : frame_pixels) {
        const uint8_t rgb[] = {(uint8_t)(((color >> 11) & 31) * 255 / 31), (uint8_t)(((color >> 5) & 63) * 255 / 63),
                               (uint8_t)((color & 31) * 255 / 31)};
        if (ok && std::fwrite(rgb, 1, sizeof(rgb), file) != sizeof(rgb))
            ok = false;
    }
    return std::fclose(file) == 0 && ok;
}

int main(int argc, char** argv) {
    if (argc > 2)
        return 2;
    lv_init();
    lv_display_t* display = lv_display_create(width_px, height_px);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_pixels, NULL, sizeof(draw_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    assert(display);

    resource_pool::init();
    assert(resource_pool::get_image("ui_settings") && resource_pool::get_image("ui_satellite_large"));
    assert(!resource_pool::get_image("settings")); // 旧位图已由生成的 ui_* 图标替代并移除。
    assert(resource_pool::get_font("rajdhaniBold_40") && resource_pool::get_font("oswaldBold_12"));
    page::DialplateView view{};
    view.create(lv_screen_active());
    assert(view.widgets().top_info.cont && view.widgets().btn_cont.btn_map && view.widgets().btn_cont.btn_rec);
    view.appear_anim_start();
    advance(600);
    const std::array names{"ui_work", "ui_record", "ui_info", "ui_settings"};
    const std::array buttons{view.widgets().btn_cont.btn_map, view.widgets().btn_cont.btn_rec,
                             view.widgets().btn_cont.btn_menu, view.widgets().btn_cont.btn_shutdown};
    assert(lv_obj_get_child_count(lv_screen_active()) == 2); // 首页没有焦点文字说明。
    for (unsigned i = 0; i < buttons.size(); ++i) {
        const auto* image = static_cast<const lv_image_dsc_t*>(resource_pool::get_image(names[i]));
        assert(image && image->header.w == 16 && image->header.h == 16);
        assert(image->header.cf == LV_COLOR_FORMAT_A8 && image->data_size == 256);
        unsigned peak = 0;
        for (unsigned pixel = 0; pixel < image->data_size; ++pixel)
            if (image->data[pixel] > peak)
                peak = image->data[pixel];
        assert(peak == 255);
        assert(lv_obj_get_style_bg_image_src(buttons[i], LV_PART_MAIN) == image);
        assert(lv_color_to_u32(lv_obj_get_style_bg_image_recolor(buttons[i], LV_PART_MAIN))
               == lv_color_to_u32(lv_color_white()));
        assert(lv_obj_get_style_bg_image_recolor_opa(buttons[i], LV_PART_MAIN) == LV_OPA_COVER);
        assert(lv_obj_get_width(buttons[i]) == 56 && lv_obj_get_height(buttons[i]) == 32);
    }
    const auto controls = view.controls(true);
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
            assert(lv_obj_is_focused(control) == selected);
            assert((lv_color_to_u32(lv_obj_get_style_border_color(control, LV_PART_MAIN))
                    == lv_color_to_u32(page::style::kAccent))
                   == selected);
            assert(lv_obj_get_style_border_width(control, LV_PART_MAIN) == (selected ? 2 : 1));
        }
    }
    lv_group_delete(group);
    const auto& top = view.widgets().top_info;
    auto* used = top.satellite_used->get_cont();
    auto* tracked = top.satellite_tracked->get_cont();
    assert(lv_obj_is_hidden(used) && lv_obj_is_hidden(tracked));
    assert(!lv_obj_is_hidden(top.satellite_missing));
    page::DialplateState device{};
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
        assert(object_coords(used).x1 > object_coords(top.icon_satellite).x2);
        assert(object_coords(used).x2 < object_coords(top.satellite_separator).x1);
        assert(object_coords(top.satellite_separator).x2 < object_coords(tracked).x1);
        assert(object_coords(tracked).x2 < object_coords(top.radio_card).x1);
        assert(object_coords(used).y2 == object_coords(tracked).y2);
        const auto* separator_font = lv_obj_get_style_text_font(top.satellite_separator, LV_PART_MAIN);
        assert(object_coords(top.satellite_separator).y2 - separator_font->base_line == object_coords(used).y2);
        assert(object_coords(used).y2 < object_coords(top.cont).y2);
        assert(object_coords(used).y1 - object_coords(top.cont).y1 >= 10);
        assert(object_coords(top.satellite_separator).x1 - object_coords(used).x2 == 3);
        assert(object_coords(tracked).x1 - object_coords(top.satellite_separator).x2 == 3);
        auto* row = lv_obj_get_parent(used);
        const auto left_space = object_coords(used).x1 - object_coords(row).x1;
        const auto right_space = object_coords(row).x2 - object_coords(tracked).x2;
        assert(left_space >= 0 && right_space >= 0 && left_space - right_space >= -1 && left_space - right_space <= 1);
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(top.cont); ++i) {
        auto* child = lv_obj_get_child(top.cont, static_cast<int32_t>(i));
        if (lv_obj_check_type(child, &lv_label_class)) {
            assert(std::strcmp(lv_label_get_text(child), "USED") != 0);
            assert(std::strcmp(lv_label_get_text(child), "TRK") != 0);
        }
    }
    device.satellites_valid = false;
    view.update(device);
    assert(lv_obj_is_hidden(used) && lv_obj_is_hidden(tracked));
    assert(!lv_obj_is_hidden(top.satellite_missing));
    device.satellites_valid = true;
    device.satellites_used = 18;
    device.satellites_tracked = 26;
    device.work_valid = true;
    device.work_mode = hal::WorkMode::Base;
    device.radio_on = true;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "BASE") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "TX") == 0);
    assert(lv_color_to_u32(lv_obj_get_style_text_color(top.mode_label, LV_PART_MAIN))
           == lv_color_to_u32(page::style::kInfo));
    device.radio_on = false;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "OFF") == 0);
    device.work_valid = false;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "--") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "--") == 0);
    device.work_valid = true;
    device.work_mode = hal::WorkMode::Rover;
    device.radio_on = true;
    view.update(device);
    assert(std::strcmp(lv_label_get_text(top.mode_label), "ROVER") == 0);
    assert(std::strcmp(lv_label_get_text(top.radio_label), "RX") == 0);
    assert(lv_color_to_u32(lv_obj_get_style_text_color(top.mode_label, LV_PART_MAIN))
           == lv_color_to_u32(page::style::kSuccess));
    advance(400);
    assert(!lv_obj_is_hidden(used) && !lv_obj_is_hidden(tracked));
    assert(lv_obj_is_hidden(top.satellite_missing));
    lv_refr_now(display);
    assert(flush_count && lv_obj_get_child_count(lv_screen_active()) >= 2);
    unsigned nonblack_pixels = 0;
    for (const uint16_t color : frame_pixels) {
        nonblack_pixels += ((color >> 11) & 31) || ((color >> 5) & 63) || (color & 31);
    }
    assert(nonblack_pixels > 100);

    device.record_valid = true;
    for (bool recording : {true, false}) {
        device.recording = recording;
        view.update(device);
        assert(lv_obj_get_style_bg_image_src(view.widgets().btn_cont.btn_rec, LV_PART_MAIN)
               == resource_pool::get_image("ui_record"));
        assert(lv_color_to_u32(lv_obj_get_style_border_color(view.widgets().btn_cont.btn_rec, LV_PART_MAIN))
               == (lv_color_to_u32(recording ? page::style::kDanger : page::style::kBorder)));
    }
    const bool saved = argc == 1 || save_ppm(argv[1]);
    view.destroy();
    lv_obj_clean(lv_screen_active());
    if (!saved) {
        std::perror("save preview");
        return 1;
    }
    return 0;
}
