/** @file app_focus_rollout_preview.cpp @brief 多页焦点基线、终态与 60 ms 连切逐帧回归。 */
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>
#include "App.h"
#include "DemoStatus.h"
#include "DomainTestInput.hpp"
#include "UI/Common/PageStyle.h"

namespace {
constexpr int width = 294, height = 126;
uint16_t frame[width * height], background[width * height], pixels[width * 20];

void
flush(lv_display_t* display, const lv_area_t* area, uint8_t* data) {
    auto* source = reinterpret_cast<uint16_t*>(data);
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            frame[y * width + x] = *source++;
        }
    }
    lv_display_flush_ready(display);
}

void
advance(unsigned ms) {
    for (unsigned elapsed = 0; elapsed < ms; elapsed += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}

void
save(lv_display_t* display, const std::string& path) {
    lv_refr_now(display);
    auto* file = std::fopen(path.c_str(), "wb");
    assert(file);
    std::fprintf(file, "P6\n%d %d\n255\n", width, height);
    for (auto pixel : frame) {
        const unsigned char rgb[]{static_cast<unsigned char>(((pixel >> 11) & 31) * 255 / 31),
                                  static_cast<unsigned char>(((pixel >> 5) & 63) * 255 / 63),
                                  static_cast<unsigned char>((pixel & 31) * 255 / 31)};
        assert(std::fwrite(rgb, 1, 3, file) == 3);
    }
    assert(std::fclose(file) == 0);
}

/** @brief 只识别透明、不可点击的上层边框，排除卡片与内容装饰。 */
lv_obj_t*
highlight(lv_obj_t* root) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (!lv_obj_is_clickable(root) && lv_obj_get_style_border_width(root, LV_PART_MAIN) == 2
        && lv_obj_get_style_bg_opa(root, LV_PART_MAIN) == LV_OPA_TRANSP
        && lv_color_to_u32(lv_obj_get_style_border_color(root, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kAccent)) {
        return root;
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = highlight(lv_obj_get_child(root, static_cast<int32_t>(i)))) {
            return found;
        }
    }
    return nullptr;
}

void
aligned(lv_obj_t* target) {
    auto* overlay = highlight(lv_screen_active());
    assert(overlay);
    lv_area_t actual, expected;
    lv_obj_get_coords(overlay, &actual);
    lv_obj_get_coords(target, &expected);
    assert(actual.x1 == expected.x1 && actual.x2 == expected.x2 && actual.y1 == expected.y1
           && actual.y2 == expected.y2);
}

/** @brief 与同一 tick 的无覆盖框画面作差，全屏寻找覆盖框像素，避免固定橙色装饰干扰。 */
unsigned
border(const lv_area_t& area, int radius) {
    const auto accent = lv_color_to_u16(page::style::kAccent);
    assert(area.x1 >= 0 && area.y1 >= 0 && area.x2 < width && area.y2 < height);
    lv_area_t bounds{width, height, -1, -1};
    unsigned count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (frame[y * width + x] != accent || frame[y * width + x] == background[y * width + x]) {
                continue;
            }
            bounds.x1 = std::min(bounds.x1, x);
            bounds.y1 = std::min(bounds.y1, y);
            bounds.x2 = std::max(bounds.x2, x);
            bounds.y2 = std::max(bounds.y2, y);
            ++count;
        }
    }
    assert(count && bounds.x1 == area.x1 && bounds.y1 == area.y1 && bounds.x2 == area.x2 && bounds.y2 == area.y2);
    for (int x = area.x1 + radius; x <= area.x2 - radius; ++x) {
        assert(frame[area.y1 * width + x] == accent);
        assert(frame[area.y2 * width + x] == accent);
    }
    for (int y = area.y1 + radius; y <= area.y2 - radius; ++y) {
        assert(frame[y * width + area.x1] == accent);
        assert(frame[y * width + area.x2] == accent);
    }
    return count;
}
} // namespace

int
main(int argc, char** argv) {
    assert(argc == 5);
    const std::string directory = argv[1], page = argv[2], mode = argv[3];
    const bool stills = mode == "stills", rapid = mode == "rapid";
    assert(stills || rapid || mode == "normal");
    std::filesystem::create_directories(directory);
    lv_init();
    auto* display = lv_display_create(width, height);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    {
        demo::model().power = {};
        const auto language = std::string(argv[4]) == "ru" ? i18n::Language::Russian : i18n::Language::English;
        i18n::set_language(language);
        assert(i18n::get_language() == language);
        App app;
        assert(app.init());
        advance(400);
        test_input::status(DemoStatus::sample(16000));
        demo::model().recorder.recording = demo::model().recorder.reported_recording = false;
        advance(600);
        if (page != "Startup") {
            assert(app.show_page(("Pages/" + page).c_str()));
        }
        advance(1000);
        auto* group = lv_group_get_default();
        const auto controls = lv_group_get_obj_count(group);
        if (stills) {
            auto* geometry = std::fopen((directory + "/focus.csv").c_str(), "w");
            assert(geometry);
            std::fprintf(geometry, "name,x1,y1,x2,y2\n");
            for (unsigned i = 0; i < std::max(controls, 1u); ++i) {
                auto* focused = lv_group_get_focused(group);
                lv_area_t area{};
                if (focused) {
                    lv_obj_get_coords(focused, &area);
                }
                const auto name = "focus-" + std::to_string(i);
                std::fprintf(geometry, "%s,%d,%d,%d,%d\n", name.c_str(), area.x1, area.y1, area.x2, area.y2);
                save(display, directory + "/" + name + ".ppm");
                if (focused) {
                    lv_obj_set_pressed(focused, true);
                    lv_obj_send_event(focused, LV_EVENT_PRESSED, nullptr);
                    advance(60);
                    save(display, directory + "/pressed-" + std::to_string(i) + ".ppm");
                    lv_obj_set_pressed(focused, false);
                    lv_obj_send_event(focused, LV_EVENT_PRESS_LOST, nullptr);
                    advance(800);
                }
                app.on_input(App::InputAction::NextFocus);
                advance(800);
            }
            assert(std::fclose(geometry) == 0);
        } else {
            assert(controls > 1);
            auto* metrics = std::fopen((directory + "/geometry.csv").c_str(), "w");
            assert(metrics);
            std::fprintf(metrics, "frame,time_ms,x1,y1,x2,y2,radius,isolated,accent_pixels\n");
            unsigned index = 0;
            const auto segment = [&](unsigned duration) {
                for (unsigned elapsed = 0; elapsed < duration; elapsed += 20) {
                    char name[32];
                    auto* overlay = highlight(lv_screen_active());
                    assert(overlay);
                    lv_obj_set_style_border_opa(overlay, LV_OPA_TRANSP, 0);
                    std::snprintf(name, sizeof(name), "/background-%04u.ppm", index);
                    save(display, directory + name);
                    std::copy(std::begin(frame), std::end(frame), std::begin(background));
                    lv_obj_set_style_border_opa(overlay, LV_OPA_COVER, 0);
                    std::snprintf(name, sizeof(name), "/frame-%04u.ppm", index);
                    save(display, directory + name);
                    lv_area_t area;
                    lv_obj_get_coords(overlay, &area);
                    const int radius = lv_obj_get_style_radius(overlay, LV_PART_MAIN);
                    const unsigned count = border(area, radius);
                    std::fprintf(metrics, "%u,%u,%d,%d,%d,%d,%d,1,%u\n", index, index * 20, area.x1, area.y1, area.x2,
                                 area.y2, radius, count);
                    ++index;
                    advance(20);
                }
            };
            aligned(lv_group_get_focused(group));
            segment(200);
            for (unsigned i = 0; i < (rapid ? controls * 3 : controls); ++i) {
                app.on_input(App::InputAction::NextFocus);
                segment(rapid ? 60 : 800);
                if (!rapid) {
                    aligned(lv_group_get_focused(group));
                }
            }
            segment(800);
            aligned(lv_group_get_focused(group));
            assert(std::fclose(metrics) == 0);
            std::printf("%s %s: %u/%u frames verified, missing=0, geometry_mismatch=0, occluded_edges=0\n",
                        page.c_str(), mode.c_str(), index, index);
        }
    }
    advance(1000);
    assert(DataProc::Broker()->getNodeNumber() == 0);
    lv_display_delete(display);
    lv_deinit();
}
