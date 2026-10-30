#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include "App.h"
#include "Def/DP_Settings.h"
#include "DemoStatus.h"
#include "UI/Startup/StartupModel.h"

namespace {
constexpr int width = 294, height = 126;
uint16_t frame[width * height], pixels[width * 20];

void
flush(lv_display_t* display, const lv_area_t* area, uint8_t* data) {
    const auto* colors = reinterpret_cast<const uint16_t*>(data);
    assert(area->x1 >= 0 && area->y1 >= 0 && area->x2 < width && area->y2 < height);
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            frame[y * width + x] = *colors++;
        }
    }
    lv_display_flush_ready(display);
}

void
advance(unsigned ms = 200) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}

lv_obj_t*
label(lv_obj_t* root, const char* text) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strcmp(lv_label_get_text(root), text) == 0) {
        return root;
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = label(lv_obj_get_child(root, static_cast<int32_t>(i)), text)) {
            return found;
        }
    }
    return nullptr;
}

void
check_reading(const char* caption, const char* reading) {
    lv_obj_update_layout(lv_screen_active());
    auto* prefix = label(lv_screen_active(), caption);
    auto* value = label(lv_screen_active(), reading);
    assert(prefix && value);
    lv_area_t a, b;
    lv_obj_get_coords(prefix, &a);
    lv_obj_get_coords(value, &b);
    assert(a.x2 < b.x1 && a.y1 == b.y1 && b.x2 < width);
    lv_point_t size;
    lv_text_get_size(&size, reading, lv_obj_get_style_text_font(value, LV_PART_MAIN),
                     lv_obj_get_style_text_letter_space(value, LV_PART_MAIN), 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    assert(size.x <= lv_obj_get_width(value) && size.y <= lv_obj_get_height(value));
}

void
capture(lv_display_t* display, const std::string& directory, const char* name) {
    lv_refr_now(display);
    if (directory.empty()) {
        return;
    }
    const auto path = directory + "/" + name + ".ppm";
    auto* file = std::fopen(path.c_str(), "wb");
    assert(file);
    const int header = std::fprintf(file, "P6\n%d %d\n255\n", width, height);
    assert(header > 0);
    for (uint16_t pixel : frame) {
        const unsigned char rgb[] = {static_cast<unsigned char>(((pixel >> 11) & 31) * 255 / 31),
                                     static_cast<unsigned char>(((pixel >> 5) & 63) * 255 / 63),
                                     static_cast<unsigned char>((pixel & 31) * 255 / 31)};
        const auto written = std::fwrite(rgb, 1, sizeof(rgb), file);
        assert(written == sizeof(rgb));
    }
    const int closed = std::fclose(file);
    assert(closed == 0);
}
} // namespace

int
main(int argc, char** argv) {
    assert(argc <= 2);
    const std::string output = argc == 2 ? argv[1] : "";
    if (!output.empty()) {
        std::filesystem::create_directories(output);
    }
    lv_init();
    auto* display = lv_display_create(width, height);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    // 先验证 Demo 从同一 HAL 模型提供合理的正负电流，再冻结为确定样本。
    DemoStatus::sample(15000);
    assert(demo::model().battery.current_valid && demo::model().battery.current_ma == 1600);
    DemoStatus::sample(0);
    assert(demo::model().battery.current_valid && demo::model().battery.current_ma < 0);
    auto& battery = demo::model().battery;
    battery = {true, 76, 76.0f, 7.8f, true, 28.0f, hal::ChargeState::Normal, true, 1600, true};
    demo::model().power = {};
    {
        App app;
        assert(app.init());
        advance(800);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
        auto* model = static_cast<page::StartupModel*>(DataProc::Broker()->search("StartupModel"));
        assert(model);
        unsigned changes = 0;
        auto* observer = lv_subject_add_observer(
            model->subject(),
            [](lv_observer_t* o, lv_subject_t*) { ++*static_cast<unsigned*>(lv_observer_get_user_data(o)); }, &changes);
        assert(observer && changes == 1);
        check_reading("CHG", "12.5W");

        // 平均电流变化不足以改变 0.1 W 文本时，领域仍更新而页面不重新发布。
        battery.current_ma = 1601;
        advance();
        assert(model->state().battery_current_ma == 1601 && changes == 1);
        battery.voltage = 7.8001f;
        advance();
        assert(changes == 1);
        battery.current_ma = 1610;
        advance();
        assert(changes == 2);
        check_reading("CHG", "12.6W");

        battery.voltage = 7.8f;
        battery.current_ma = 1600;
        advance();
        for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
            const int result = dp::Settings::notify(*DataProc::Broker()->mainNode(), {language});
            assert(result == DataNode::RES_OK);
            advance();
            check_reading("CHG", "12.5W");
            capture(display, output,
                    language == i18n::Language::English ? "Startup-charging-en" : "Startup-charging-ru");
        }
        battery.current_valid = false;
        advance();
        check_reading("CHG", "7.80 V");
        capture(display, output, "Startup-current-unavailable");
        const auto fallback_changes = changes;
        battery.current_ma = 1700;
        advance();
        assert(changes == fallback_changes);
        battery.current_valid = true;
        for (int16_t current : {int16_t{-32768}, int16_t{-350}, int16_t{0}, int16_t{49}}) {
            battery.current_ma = current;
            advance();
            check_reading("CHG", "7.80 V");
            assert(changes == fallback_changes);
        }
        battery.current_ma = 50;
        advance();
        check_reading("CHG", "0.4W");
        battery.current_ma = 1600;
        battery.charge = hal::ChargeState::NotCharging;
        advance();
        check_reading("BAT", "7.80 V");
        battery.charge = hal::ChargeState::Fast;
        battery.current_ma = 32767;
        battery.voltage = 65.535f;
        advance();
        check_reading("CHG", "2147.4W");
        for (float voltage : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
            battery.voltage = voltage;
            advance();
            check_reading("CHG", "--- V");
        }
        battery.valid = false;
        advance();
        check_reading("BAT", "N/A");
        lv_observer_delete(observer);
    }
    lv_display_delete(display);
    lv_deinit();
    std::puts("Startup：BQ 平均电流功率、阈值/失败回退、文本去重与英俄文布局通过。");
}
