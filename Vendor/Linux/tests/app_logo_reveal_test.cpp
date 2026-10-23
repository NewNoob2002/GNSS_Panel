/** @file app_logo_reveal_test.cpp @brief OEM 20 ms 逐帧、柔边、静止失效及双语设备名称回归。 */
#include <algorithm>
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
#include "UI/Resource/DeviceNameLayout.h"
#include "UI/Startup/StartupView.h"
#include "UI/SystemLoading/SystemLoadingView.h"
#include "oem_config.h"

namespace {
constexpr int kWidth = 294, kHeight = 126;
std::array<uint16_t, kWidth * kHeight> frame;
uint16_t draw[kWidth * 20];
unsigned flush_count;
unsigned invalidation_count;
unsigned partial_pixels;
unsigned partial_invalidations;
bool partial_only;

/** @brief 在 LVGL 拆分缓冲前统计，确保局部失效未被分块 flush 掩盖。 */
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

/** @brief 迁移客户从原单图独立取样；文字客户用无动画的原生 LVGL 零件渲染作终态。 */
std::array<uint16_t, kWidth * kHeight>
expected_frame(lv_obj_t* root, const logo_parts_t& logo) {
    std::ifstream input(LOGO_REFERENCE_FILE);
    std::vector<uint8_t> bytes;
    if (input) {
        const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        const auto begin = text.find("_map[] = {");
        assert(begin != std::string::npos);
        const auto end = text.find('}', begin);
        for (auto cursor = text.find("0x", begin); cursor < end; cursor = text.find("0x", cursor + 2)) {
            bytes.push_back(static_cast<uint8_t>(std::strtoul(text.c_str() + cursor, nullptr, 16)));
        }
    }
    auto* container = page::style::box(root, 0, 0, kWidth, kHeight);
    lv_obj_set_style_bg_color(container, lv_color_hex(logo.layout.background), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_image_dsc_t original{};
    if (!bytes.empty()) {
        assert(bytes.size() == logo.width * logo.height * (logo.recolor ? 1u : 2u));
        original.header.magic = LV_IMAGE_HEADER_MAGIC;
        original.header.cf = logo.recolor ? LV_COLOR_FORMAT_A8 : LV_COLOR_FORMAT_RGB565;
        original.header.w = logo.width;
        original.header.stride = logo.width * (logo.recolor ? 1 : 2);
        original.header.h = logo.height;
        original.data_size = bytes.size();
        original.data = bytes.data();
    }
    for (unsigned i = 0; i < (bytes.empty() ? logo.part_count : 1u); ++i) {
        auto* object = lv_image_create(container);
        lv_image_set_src(object, bytes.empty() ? logo.parts[i].image : &original);
        lv_obj_set_pos(object, logo.layout.x + (bytes.empty() ? logo.parts[i].x : 0),
                       logo.layout.y + (bytes.empty() ? logo.parts[i].y : 0));
        if (logo.recolor) {
            lv_obj_set_style_image_recolor(object, lv_color_hex(logo.layout.foreground), 0);
            lv_obj_set_style_image_recolor_opa(object, LV_OPA_COVER, 0);
        }
    }
    lv_refr_now(nullptr);
    auto result = frame;
    lv_obj_delete(container);
    return result;
}

unsigned
last_change(const logo_parts_t& logo) {
    const auto& effect = logo.effect;
    unsigned result = effect.background_ms + effect.group_duration_ms;
    if (effect.preset == LOGO_EFFECT_CASCADE) {
        result = 0;
        for (unsigned i = 0; i < logo.part_count; ++i) {
            const auto& part = logo.parts[i];
            const auto* group = logo.groups && part.group < logo.group_count ? &logo.groups[part.group] : nullptr;
            const unsigned delay =
                group && group->delay_ms >= 0 ? group->delay_ms : part.group * effect.group_interval_ms;
            const unsigned duration = group && group->duration_ms ? group->duration_ms : effect.group_duration_ms;
            result = std::max(result, effect.background_ms + delay + duration);
        }
    }
    if (logo.highlight && effect.highlight) {
        result = std::max(result, effect.highlight_start_ms + effect.highlight_duration_ms);
    }
    return result;
}

void
loading(lv_obj_t* root, const std::string& directory) {
    const auto baseline_timers = timers();
    const auto baseline_anims = lv_anim_count_running();
    const auto expected = expected_frame(root, oem_logo_parts);
    {
        page::SystemLoadingView view;
        std::array<i18n::TextId, page::SystemLoadingView::kStepCount> titles{};
        titles.fill(i18n::TextId::LoadingConfiguration);
        view.create(root, titles);
        for (unsigned ms = 0; ms <= page::SystemLoadingView::kLogoMs; ms += 20) {
            const auto before_flushes = flush_count, before_invalidations = invalidation_count;
            partial_only = oem_logo_parts.recolor && ms >= 180 && ms < 900;
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
            if (ms > last_change(oem_logo_parts) + 20) {
                assert(flush_count == before_flushes && invalidation_count == before_invalidations);
            }
            if (!ms) {
                assert(std::all_of(frame.begin(), frame.end(), [](auto pixel) { return pixel == 0; }));
            }
            if (!directory.empty()) {
                ppm(directory + "/logo-" + std::to_string(10000 + ms) + ".ppm");
            }
        }
        advance(20); // total_ms 可不整除 20，越过终点后再检查。
        assert(frame == expected);
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
    std::printf("OEM Logo: endpoint exact, lifecycle clean; %u partial invalidations\n", partial_invalidations);
}

/** @brief 各预设使用同一 OEM 零件，无客户分支；停留段必须零失效且零 flush。 */
void
presets(lv_obj_t* root, const std::string& directory) {
    const auto expected = expected_frame(root, oem_logo_parts);
    const char* names[] = {"cascade", "soft", "sweep", "wipe"};
    for (unsigned preset = 0; preset < 4; ++preset) {
        auto logo = oem_logo_parts;
        logo.effect = {150,
                       50,
                       preset == LOGO_EFFECT_SOFT    ? 600u
                       : preset == LOGO_EFFECT_SWEEP ? 180u
                       : preset == LOGO_EFFECT_WIPE  ? 700u
                                                     : 250u,
                       preset <= LOGO_EFFECT_SOFT ? 6 : 0,
                       preset == LOGO_EFFECT_CASCADE || preset == LOGO_EFFECT_SWEEP,
                       900,
                       300,
                       1300,
                       static_cast<logo_effect_preset_t>(preset)};
        const std::string path = directory.empty() ? "" : directory + "/" + names[preset];
        if (!path.empty()) {
            std::filesystem::create_directories(path);
        }
        const auto baseline_timers = timers();
        app::LogoReveal reveal;
        reveal.create(root, logo);
        reveal.play();
        for (unsigned ms = 0; ms <= 1400; ms += 20) {
            const auto before_flushes = flush_count, before_invalidations = invalidation_count;
            if (ms) {
                advance(20);
            } else {
                lv_refr_now(nullptr);
            }
            if (ms >= 1220) {
                assert(flush_count == before_flushes && invalidation_count == before_invalidations);
            }
            if (!path.empty() && ms <= 1300) {
                ppm(path + "/logo-" + std::to_string(10000 + ms) + ".ppm");
            }
        }
        assert(frame == expected);
        reveal.destroy();
        assert(!lv_anim_count_running() && timers() == baseline_timers);
        for (const unsigned cancel_ms : {20u, 300u, 960u}) {
            reveal.create(root, logo);
            reveal.play();
            advance(cancel_ms);
            reveal.stop();
            assert(!lv_anim_count_running() && timers() == baseline_timers);
            reveal.destroy();
        }
        reveal.create(root, logo);
        reveal.play();
        lv_obj_clean(root);
        assert(!lv_anim_count_running() && timers() == baseline_timers);
        std::printf("%s: exact endpoint, 1220-1400 ms zero invalidations/flushes, all cancellation phases clean\n",
                    names[preset]);
    }
}

/** @brief 实心合成图独立验证 wipe 真实像素柔边；整个屏幕扫描确保无越界绘制。 */
void
wipe_edge(lv_obj_t* root) {
    std::array<uint8_t, 64 * 32> alpha;
    alpha.fill(255);
    lv_image_dsc_t image{};
    image.header.magic = LV_IMAGE_HEADER_MAGIC;
    image.header.cf = LV_COLOR_FORMAT_A8;
    image.header.w = image.header.stride = 64;
    image.header.h = 32;
    image.data = alpha.data();
    image.data_size = alpha.size();
    const logo_part_t piece{&image, 0, 0, 0};
    const logo_parts_t logo{&piece, nullptr, 1,    1,     {0, 0xffffff, 115, 47},
                            64,     32,      true, false, {150, 50, 700, 0, false, 900, 300, 1300, LOGO_EFFECT_WIPE}};
    app::LogoReveal reveal;
    reveal.create(root, logo);
    reveal.play();
    advance(500);
    assert(lv_obj_get_child_count(reveal.container()) == 1);
    auto* object = lv_obj_get_child(reveal.container(), 0);
    assert(lv_obj_check_type(object, &lv_image_class));
    lv_area_t parent_area, image_area;
    lv_obj_get_coords(reveal.container(), &parent_area);
    lv_obj_get_coords(object, &image_area);
    assert(image_area.x1 >= parent_area.x1 && image_area.x2 <= parent_area.x2);
    assert(image_area.y1 >= parent_area.y1 && image_area.y2 <= parent_area.y2);
    const int front = (500 - 150) * (64 + 16) / 700;
    unsigned mixed = 0;
    uint16_t previous = 0xffff;
    for (int x = 0; x < kWidth; ++x) {
        const auto pixel = frame[60 * kWidth + x];
        if (x < 115 || x >= 179) {
            assert(pixel == 0);
        } else if (x < 115 + front - 16) {
            assert(pixel == 0xffff);
        } else if (x >= 115 + front) {
            assert(pixel == 0);
        } else {
            assert(pixel <= previous);
            previous = pixel;
            mixed += pixel != 0 && pixel != 0xffff;
        }
    }
    assert(mixed >= 12 && mixed <= 16);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            if (x < 115 || x >= 179 || y < 47 || y >= 79) {
                assert(frame[y * kWidth + x] == 0);
            }
        }
    }
    assert(lv_obj_get_style_opa(reveal.container(), LV_PART_MAIN) == LV_OPA_COVER);
    reveal.destroy();
    advance(20);
    std::printf("wipe: %u soft-edge columns, no pixels outside Logo bounds, no container opacity layer\n", mixed);
}

lv_obj_t*
label(lv_obj_t* parent, const char* text) {
    if (lv_obj_check_type(parent, &lv_label_class) && std::strcmp(lv_label_get_text(parent), text) == 0) {
        return parent;
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(parent); ++i) {
        if (auto* found = label(lv_obj_get_child(parent, i), text)) {
            return found;
        }
    }
    return nullptr;
}

/** @brief 用实际 LVGL 排版对照生成器预测值与唯一布局常量，禁止名称静默截断。 */
void
name_width(lv_obj_t* object, int32_t limit, int32_t predicted) {
    lv_obj_update_layout(object);
    lv_point_t natural;
    assert(lv_obj_get_width(object) == limit);
    assert(lv_obj_get_style_text_letter_space(object, LV_PART_MAIN) == 0);
    lv_text_get_size(&natural, lv_label_get_text(object), lv_obj_get_style_text_font(object, LV_PART_MAIN), 0, 0,
                     LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    assert(natural.x == predicted && natural.x <= lv_obj_get_width(object));
    std::printf("Device name: %s, actual/predicted %ld/%ld <= %ld px\n", lv_label_get_text(object),
                static_cast<long>(natural.x), static_cast<long>(predicted), static_cast<long>(limit));
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
    for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
        i18n::set_language(language);
        view.apply_language();
        auto* title = label(root, i18n::device_name());
        assert(title
               && lv_color_to_u32(lv_obj_get_style_text_color(title, LV_PART_MAIN))
                      == lv_color_to_u32(lv_color_hex(OEM_TEXT_COLOR)));
        assert(lv_obj_get_style_text_font(title, LV_PART_MAIN) == resource_pool::get_font("oswaldBold_18"));
        name_width(title, OEM_STARTUP_TITLE_WIDTH,
                   language == i18n::Language::English ? OEM_DEVICE_EN_WIDTH : OEM_DEVICE_RU_WIDTH);
        lv_refr_now(nullptr);
        if (!directory.empty()) {
            ppm(directory + (language == i18n::Language::English ? "/startup-en.ppm" : "/startup-ru.ppm"));
        }
    }
    view.destroy();
    lv_obj_clean(root);
    {
        page::SystemLoadingView loading_view;
        std::array<i18n::TextId, page::SystemLoadingView::kStepCount> titles{};
        titles.fill(i18n::TextId::LoadingConfiguration);
        loading_view.create(root, titles);
        loading_view.show_initialization();
        advance(240);
        for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
            i18n::set_language(language);
            loading_view.apply_language();
            auto* footer = label(root, i18n::text(i18n::TextId::SystemLoadingTitle));
            assert(footer
                   && lv_color_to_u32(lv_obj_get_style_text_color(footer, LV_PART_MAIN))
                          == lv_color_to_u32(lv_color_hex(OEM_FOOTER_COLOR)));
            assert(lv_obj_get_style_text_font(footer, LV_PART_MAIN) == resource_pool::get_font("oswaldBold_12"));
            name_width(footer, OEM_LOADING_FOOTER_WIDTH,
                       language == i18n::Language::English ? OEM_DEVICE_FOOTER_EN_WIDTH : OEM_DEVICE_FOOTER_RU_WIDTH);
            lv_refr_now(nullptr);
            if (!directory.empty()) {
                ppm(directory + (language == i18n::Language::English ? "/loading-en.ppm" : "/loading-ru.ppm"));
            }
        }
        loading_view.destroy();
    }
    lv_obj_clean(root);
    i18n::set_language(i18n::Language::English);
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
    presets(root, directory);
    wipe_edge(root);
    startup(root, directory);
    lv_obj_delete(root);
    lv_display_delete(display);
    lv_deinit();
}
