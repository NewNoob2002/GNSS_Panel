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
lv_area_t previous{};
bool have_previous = false;
unsigned flushed_pixels = 0;

/** @brief 按本页独有的非交互焦点样式定位高亮，不要求生产页面暴露测试接口。 */
lv_obj_t*
highlight(lv_obj_t* root) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (!lv_obj_is_clickable(root) && lv_obj_get_style_border_width(root, LV_PART_MAIN) == 2
        && lv_color_to_u32(lv_obj_get_style_border_color(root, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kAccent)) {
        return root;
    }
    for (unsigned index = 0; index < lv_obj_get_child_count(root); ++index) {
        if (auto* found = highlight(lv_obj_get_child(root, static_cast<int32_t>(index)))) {
            return found;
        }
    }
    return nullptr;
}

void
flush(lv_display_t* display, const lv_area_t* area, uint8_t* data) {
    auto* source = reinterpret_cast<uint16_t*>(data);
    for (int y = area->y1; y <= area->y2; ++y) {
        for (int x = area->x1; x <= area->x2; ++x) {
            frame[y * width + x] = *source++;
        }
    }
    flushed_pixels += static_cast<unsigned>(lv_area_get_width(area) * lv_area_get_height(area));
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

/** @brief 从真实 RGB565 帧校验边框存在、外接矩形和四条直边，不能只依赖对象隐藏标志。 */
unsigned
check_border(unsigned index, const lv_area_t& expected, int radius) {
    const uint16_t accent = lv_color_to_u16(page::style::kAccent);
    lv_area_t visible{width, height, -1, -1};
    unsigned count = 0;
    // 与同一 tick 的无边框画面作差，排除主卡图标和百分比的固定橙色。
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (frame[y * width + x] != accent || frame[y * width + x] == background[y * width + x]) {
                continue;
            }
            visible.x1 = std::min(visible.x1, x);
            visible.y1 = std::min(visible.y1, y);
            visible.x2 = std::max(visible.x2, x);
            visible.y2 = std::max(visible.y2, y);
            ++count;
        }
    }
    if (!count || visible.x1 != expected.x1 || visible.y1 != expected.y1 || visible.x2 != expected.x2
        || visible.y2 != expected.y2) {
        std::fprintf(stderr, "frame %u: border pixels=%u, visible=(%d,%d,%d,%d), geometry=(%d,%d,%d,%d)\n", index,
                     count, visible.x1, visible.y1, visible.x2, visible.y2, expected.x1, expected.y1, expected.x2,
                     expected.y2);
        assert(false && "focus border missing or occluded");
    }
    /** 按当前控件圆角留白，四条直边都必须完整可见。 */
    for (int x = expected.x1 + radius; x <= expected.x2 - radius; ++x) {
        assert(frame[expected.y1 * width + x] == accent);
        assert(frame[expected.y2 * width + x] == accent);
    }
    for (int y = expected.y1 + radius; y <= expected.y2 - radius; ++y) {
        assert(frame[y * width + expected.x1] == accent);
        assert(frame[y * width + expected.x2] == accent);
    }
    return count;
}

/** @brief 固定 20 ms 时间轴导出实际 RGB565 帧，供基线与弹簧版同序列比较。 */
void
capture(lv_display_t* display, const std::string& directory, unsigned index, FILE* metrics) {
    lv_refr_now(display);
    auto* object = highlight(lv_screen_active());
    assert(object && "focus overlay must exist in every frame");
    lv_area_t current;
    lv_obj_get_coords(object, &current);
    lv_obj_set_style_border_opa(object, LV_OPA_TRANSP, 0);
    lv_refr_now(display);
    std::copy(std::begin(frame), std::end(frame), std::begin(background));
    lv_obj_set_style_border_opa(object, LV_OPA_COVER, 0);
    lv_refr_now(display);
    const unsigned accent_pixels = check_border(index, current, lv_obj_get_style_radius(object, LV_PART_MAIN));
    if (metrics) {
        unsigned area = 0;
        if (have_previous
            && (current.x1 != previous.x1 || current.y1 != previous.y1 || current.x2 != previous.x2
                || current.y2 != previous.y2)) {
            const auto left = std::max(0, std::min(current.x1, previous.x1) & ~1);
            const auto top = std::max(0, std::min(current.y1, previous.y1) & ~1);
            const auto right = std::min(width - 1, std::max(current.x2, previous.x2) | 1);
            const auto bottom = std::min(height - 1, std::max(current.y2, previous.y2) | 1);
            area = static_cast<unsigned>((right - left + 1) * (bottom - top + 1));
        }
        std::fprintf(metrics, "%u,%u,%d,%d,%d,%d,%u,%u,%u\n", index, index * 20, current.x1, current.y1, current.x2,
                     current.y2, area, flushed_pixels, accent_pixels);
        previous = current;
        have_previous = true;
    }
    flushed_pixels = 0;
    if (directory.empty()) {
        return;
    }
    char name[32];
    std::snprintf(name, sizeof(name), "/frame-%04u.ppm", index);
    auto* file = std::fopen((directory + name).c_str(), "wb");
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
} // namespace

int
main(int argc, char** argv) {
    assert(argc <= 3);
    const std::string directory = argc >= 2 ? argv[1] : "";
    const bool rapid = argc == 3 && std::string(argv[2]) == "rapid";
    if (!directory.empty()) {
        std::filesystem::create_directories(directory);
    }
    FILE* metrics = directory.empty() ? nullptr : std::fopen((directory + "/geometry.csv").c_str(), "w");
    assert(directory.empty() || metrics);
    if (metrics) {
        std::fprintf(metrics, "frame,time_ms,x1,y1,x2,y2,aligned_dirty_pixels,host_flushed_pixels,accent_pixels\n");
    }
    lv_init();
    auto* display = lv_display_create(width, height);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    {
        demo::model().power = {};
        App app;
        assert(app.init());
        advance(400);
        test_input::status(DemoStatus::sample(16000));
        assert(app.show_page("Pages/SystemDash"));
        advance(600);
        assert(lv_group_get_obj_count(lv_group_get_default()) == 4);
        assert(lv_obj_get_x(app.focused()) == 8 && lv_obj_get_y(app.focused()) == 28);
        assert(lv_obj_get_width(app.focused()) == 88 && lv_obj_get_height(app.focused()) == 77);
        unsigned index = 0;
        const auto segment = [&](unsigned duration) {
            for (unsigned elapsed = 0; elapsed < duration; elapsed += 20) {
                capture(display, directory, index++, metrics);
                advance(20);
            }
        };
        segment(200);
        for (unsigned step = 0; step < (rapid ? 12u : 4u); ++step) {
            app.on_input(App::InputAction::NextFocus);
            segment(rapid ? 60 : 500);
        }
        segment(600);
        std::printf("Spring %s: %u/%u frames verified, missing=0, geometry_mismatch=0, occluded_edges=0\n",
                    rapid ? "rapid" : "normal", index, index);
    }
    advance(1000);
    assert(DataProc::Broker()->getNodeNumber() == 0);
    lv_display_delete(display);
    if (metrics) {
        assert(std::fclose(metrics) == 0);
    }
    lv_deinit();
}
