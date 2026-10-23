/** @file app_logo_reveal_test.cpp @brief 20 ms 渲染、静止像素与 Logo/Startup 生命周期证据。 */
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "UI/Common/LogoReveal.h"
#include "UI/Common/PageStyle.h"
#include "UI/Startup/StartupView.h"
#include "UI/SystemLoading/SystemLoadingView.h"

namespace {
constexpr int kWidth = 294, kHeight = 126;
std::array<uint16_t, kWidth * kHeight> frame;
uint16_t draw[kWidth * 20];
unsigned flush_count;
unsigned invalidation_count;
unsigned partial_pixels;
unsigned partial_invalidations;
bool partial_only;

/** @brief 在 LVGL 拆分绘制缓冲前验证失效区域，避免全屏刷新被分块 flush 掩盖。 */
void
invalidated(lv_event_t* event) {
    ++invalidation_count;
    if (partial_only) {
        const auto* area = static_cast<const lv_area_t*>(lv_event_get_param(event));
        assert(lv_area_get_width(area) < kWidth || lv_area_get_height(area) < kHeight);
        ++partial_invalidations;
    }
}

void
flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels) {
    ++flush_count;
    const int width = lv_area_get_width(area);
    if (partial_only) {
        partial_pixels += width * lv_area_get_height(area);
    }
    for (int y = area->y1; y <= area->y2; ++y) {
        std::memcpy(frame.data() + y * kWidth + area->x1, reinterpret_cast<uint16_t*>(pixels) + (y - area->y1) * width,
                    width * sizeof(uint16_t));
    }
    lv_display_flush_ready(display);
}

unsigned
timers() {
    unsigned count = 0;
    for (auto* timer = lv_timer_get_next(nullptr); timer; timer = lv_timer_get_next(timer)) {
        ++count;
    }
    return count;
}

void
advance(unsigned ms) {
    for (unsigned elapsed = 0; elapsed < ms; elapsed += 20) {
        lv_tick_inc(20);
        lv_timer_handler();
    }
    lv_refr_now(nullptr);
}

void
ppm(const std::string& file, uint8_t brightness = 255) {
    FILE* output = std::fopen(file.c_str(), "wb");
    assert(output);
    assert(std::fprintf(output, "P6\n%d %d\n255\n", kWidth, kHeight) > 0);
    for (const uint16_t pixel : frame) {
        const uint8_t rgb[] = {static_cast<uint8_t>(((pixel >> 11) & 31) * brightness / 31),
                               static_cast<uint8_t>(((pixel >> 5) & 63) * brightness / 63),
                               static_cast<uint8_t>((pixel & 31) * brightness / 31)};
        assert(std::fwrite(rgb, 1, sizeof(rgb), output) == sizeof(rgb));
    }
    assert(std::fclose(output) == 0);
}

std::vector<uint8_t>
reference(const char* filename) {
    std::ifstream input(std::string(LOGO_REFERENCE_DIR) + "/" + filename);
    assert(input);
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    const auto begin = text.find("_map[] = {");
    assert(begin != std::string::npos);
    const auto end = text.find('}', begin);
    std::vector<uint8_t> bytes;
    for (auto cursor = text.find("0x", begin); cursor < end; cursor = text.find("0x", cursor + 2)) {
        bytes.push_back(static_cast<uint8_t>(std::strtoul(text.c_str() + cursor, nullptr, 16)));
    }
    return bytes;
}

std::array<uint16_t, kWidth * kHeight>
old_frame(lv_obj_t* root, const logo_parts_t& logo, const char* file) {
    auto bytes = reference(file);
    assert(bytes.size() == logo.width * logo.height);
    lv_image_dsc_t image{};
    image.header.magic = LV_IMAGE_HEADER_MAGIC;
    image.header.cf = LV_COLOR_FORMAT_A8;
    image.header.w = image.header.stride = logo.width;
    image.header.h = logo.height;
    image.data_size = bytes.size();
    image.data = bytes.data();
    auto* container = page::style::box(root, 0, 0, kWidth, kHeight);
    lv_obj_set_style_bg_color(container, lv_color_hex(logo.layout.background), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    auto* object = lv_image_create(container);
    lv_image_set_src(object, &image);
    lv_obj_set_pos(object, logo.layout.x, logo.layout.y);
    lv_obj_set_style_image_recolor(object, lv_color_hex(logo.layout.foreground), 0);
    lv_obj_set_style_image_recolor_opa(object, LV_OPA_COVER, 0);
    lv_refr_now(nullptr);
    auto result = frame;
    lv_obj_delete(container);
    return result;
}

bool
visible(const logo_parts_t& logo, unsigned part, uint16_t background) {
    const auto& piece = logo.parts[part];
    const auto* image = static_cast<const lv_image_dsc_t*>(piece.image);
    for (unsigned y = 0; y < image->header.h; ++y) {
        for (unsigned x = 0; x < image->header.w; ++x) {
            if (image->data[y * image->header.stride + x] > 200
                && frame[(logo.layout.y + piece.y + y) * kWidth + logo.layout.x + piece.x + x] != background) {
                return true;
            }
        }
    }
    return false;
}

void
loading(lv_obj_t* root, const std::string& directory) {
    const auto baseline_timers = timers();
    const auto baseline_anims = lv_anim_count_running();
    const auto expected = old_frame(root, img_src_startupLogo_parts, "img_src_SystemLoading_XYZ.c");
    int first_s = -1, first_r = -1;
    {
        page::SystemLoadingView view;
        std::array<i18n::TextId, page::SystemLoadingView::kStepCount> titles{};
        titles.fill(i18n::TextId::LoadingConfiguration);
        view.create(root, titles);
        for (unsigned ms = 0; ms <= page::SystemLoadingView::kLogoMs; ms += 20) {
            const auto before_flushes = flush_count;
            const auto before_invalidations = invalidation_count;
            partial_only = ms >= 180 && ms < 900;
            partial_pixels = 0;
            if (ms) {
                advance(20);
            } else {
                lv_refr_now(nullptr);
            }
            if (partial_only) {
                assert(partial_pixels < kWidth * kHeight);
            }
            partial_only = false;
            if (ms >= 1220) {
                assert(flush_count == before_flushes);
                assert(invalidation_count == before_invalidations);
            }
            if (!ms) {
                for (auto pixel : frame) {
                    assert(pixel == 0); /** @brief 第 0 帧为黑色，不含任何前景。 */
                }
            }
            if (ms >= 160) {
                const auto bg = lv_color_to_u16(lv_color_hex(0x8ccf9c));
                if (first_s < 0 && visible(img_src_startupLogo_parts, 0, bg)) {
                    first_s = ms;
                }
                if (first_r < 0 && visible(img_src_startupLogo_parts, 7, bg)) {
                    first_r = ms;
                }
            }
            if (!directory.empty()) {
                ppm(directory + "/logo-" + std::to_string(10000 + ms) + ".ppm");
            }
        }
        assert(partial_invalidations > 0);
        assert(first_s >= 160 && first_s < first_r && first_r >= 500);
        assert(frame == expected); /** @brief 与拆分前整张 A8 图的 LVGL 渲染逐像素一致。 */
        assert(lv_anim_count_running() == baseline_anims && timers() == baseline_timers);
        for (const unsigned cancel_ms : {20u, 300u, 960u}) {
            view.show_logo();
            advance(cancel_ms);
            view.destroy();
            assert(lv_anim_count_running() == baseline_anims && timers() == baseline_timers);
        }
    }
    lv_obj_clean(root);
    advance(20);
    assert(lv_anim_count_running() == baseline_anims && timers() == baseline_timers);
    std::printf("Logo: S visible %d ms, r visible %d ms; 1300 ms exact pixels, lifecycle clean; "
                "%u partial invalidations, 1220-1300 ms zero invalidations/flushes\n",
                first_s, first_r, partial_invalidations);
}

void
variants(lv_obj_t* root) {
    for (const auto* logo : {&img_src_MiddleLogo_parts, &img_src_RGKLogo_parts}) {
        std::array<uint16_t, kWidth * kHeight> expected;
        if (logo == &img_src_MiddleLogo_parts) {
            expected = old_frame(root, *logo, "img_src_SystemLoading_Neutral.c");
        } else {
            const auto* image = static_cast<const lv_image_dsc_t*>(logo->parts[0].image);
            assert(image->header.cf == LV_COLOR_FORMAT_RGB565 && !logo->recolor && !logo->highlight);
            std::memcpy(expected.data(), image->data, sizeof(expected));
        }
        const auto count = timers();
        app::LogoReveal reveal;
        reveal.create(root, *logo, {});
        reveal.play();
        for (unsigned ms = 0; ms < 1300; ms += 20) {
            advance(20);
        }
        assert(frame == expected);
        reveal.destroy();
        assert(!lv_anim_count_running() && timers() == count);
        /** @brief 父对象先删除也必须解除借用 this 的动画。 */
        reveal.create(root, *logo, {});
        reveal.play();
        lv_obj_clean(root);
        assert(!lv_anim_count_running() && timers() == count);
    }
}

void
startup(lv_obj_t* root, const std::string& directory) {
    page::StartupView view;
    view.create(root);
    page::StartupState state;
    state.battery_valid = true;
    state.battery_percent = 76;
    std::snprintf(state.firmware.data(), state.firmware.size(), "V2.4.4");
    std::snprintf(state.hardware.data(), state.hardware.size(), "V1.5");
    view.update(state);
    view.reset_hold();
    lv_refr_now(nullptr);
    for (unsigned ms = 0; ms <= 600; ms += 20) {
        const unsigned brightness = ms >= 300 ? 255 : 255 * ms * (600 - ms) / (300 * 300);
        if (!directory.empty()) {
            ppm(directory + "/startup-" + std::to_string(10000 + ms) + ".ppm", brightness);
        }
    }
    view.destroy();
    lv_obj_clean(root);
}
} // namespace

int
main(int argc, char** argv) {
    assert(argc <= 2);
    const std::string directory = argc == 2 ? argv[1] : "";
    if (!directory.empty()) {
        std::filesystem::create_directories(directory);
    }
    lv_init();
    auto* display = lv_display_create(kWidth, kHeight);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw, nullptr, sizeof(draw), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_display_add_event_cb(display, invalidated, LV_EVENT_INVALIDATE_AREA, nullptr);
    auto* root = page::style::box(lv_screen_active(), 0, 0, kWidth, kHeight);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    loading(root, directory);
    variants(root);
    startup(root, directory);
    lv_obj_delete(root);
    lv_display_delete(display);
    lv_deinit();
}
