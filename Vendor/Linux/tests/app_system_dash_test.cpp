#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "App.h"
#include "SystemService.h"
#include "DemoStatus.h"
#include "Pages/SaveConfig/SaveConfigView.h"
#include "Pages/SystemLoading/SystemLoadingView.h"
#include "Log.h"
#include "Utils/PageStyle.h"
#include "Utils/SettingsStyle.h"
#include "lvgl_private.h"

namespace {
/** @brief Reject SaveConfig navigation after power preparation to exercise rollback. */
PageManager::RES_TYPE
reject_save_navigation(PageManager::EVENT* event) {
    if (event->type == PageManager::EVENT_TYPE::PAGE_PUSH && event->param
        && std::strcmp(static_cast<const char*>(event->param), "Pages/SaveConfig") == 0) {
        auto* attempts = static_cast<unsigned*>(event->userData);
        ++*attempts;
        assert(SystemService::snapshot().power_pending);
        return PageManager::RES_TYPE::STOP_PROCESS;
    }
    return PageManager::RES_TYPE::OK;
}

/** @brief 通过公开 API 读取对象的屏幕绝对坐标供布局断言使用。 */
static lv_area_t
object_coords(const lv_obj_t* object) {
    lv_area_t area;
    lv_obj_get_coords(object, &area);
    return area;
}

constexpr int width = 294, height = 126;
constexpr i18n::TextId inventory_titles[] = {i18n::TextId::SystemWorkTitle,    i18n::TextId::SystemGpsTitle,
                                             i18n::TextId::SystemWifiTitle,    i18n::TextId::SystemBatteryTitle,
                                             i18n::TextId::SystemStorageTitle, i18n::TextId::SystemTitle};
uint16_t frame[width * height], pixels[width * 20];
App* active = nullptr;
std::vector<std::string> boot_logs;
void boot_log_sink(AppLogLevel, const char* module, const char* message, void*) {
    if (std::strcmp(module, "SystemLoading") == 0)
        boot_logs.emplace_back(message);
}
unsigned red_nodes(lv_obj_t* root) {
    if (lv_obj_is_hidden(root)) {
        return 0;
    }
    unsigned count =
        lv_obj_get_width(root) == 8 && lv_obj_get_height(root) == 8
        && lv_color_to_u32(lv_obj_get_style_bg_color(root, LV_PART_MAIN)) == lv_color_to_u32(lv_color_hex(0xd03c3b));
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        count += red_nodes(lv_obj_get_child(root, static_cast<int32_t>(i)));
    }
    return count;
}

void
flush(lv_display_t* driver, const lv_area_t* area, uint8_t* px_map) {
    uint16_t* colors = (uint16_t*)px_map;
    assert(area->x1 >= 0 && area->y1 >= 0 && area->x2 < width && area->y2 < height);
    for (int y = area->y1; y <= area->y2; ++y)
        for (int x = area->x1; x <= area->x2; ++x)
            frame[y * width + x] = *colors++;
    lv_display_flush_ready(driver);
}
void advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}
void input(App::InputAction action) {
    active->on_input(action);
    advance(420); // v9 滚动最长 400 ms，再留一个 16 ms 刷新周期使终态落定。
}
void sample_keys(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        advance(10);
        active->poll_keys(lv_tick_get());
    }
}
void product_click(App::Key key, bool double_click = false) {
    sample_keys(40); // Synchronize the idle page context, as the board's continuous polling does.
    for (unsigned click = 0; click < (double_click ? 2u : 1u); ++click) {
        active->on_key(key, true);
        sample_keys(40);
        active->on_key(key, false);
        sample_keys(80);
    }
    sample_keys(700); // Debounce, single-click decision and the resulting page transition.
}
/** Board evdev backlog: press and release queued during one slow frame, delivered together with their stamps. */
void product_backlog_click(App::Key key) {
    sample_keys(40);
    const std::uint64_t pressed_at = lv_tick_get() + 5;
    advance(90); // A long frame: no polling while both transitions are queued.
    active->on_key_at(key, true, pressed_at);
    active->on_key_at(key, false, pressed_at + 60);
    sample_keys(700);
}
/** Hold a key for hold_ms on the board path (debounced polling), then release. */
void product_hold(App::Key key, unsigned hold_ms) {
    sample_keys(40);
    active->on_key(key, true);
    sample_keys(hold_ms);
    active->on_key(key, false);
    sample_keys(700);
}
void product_start() {
    sample_keys(40);
    active->on_key(App::Key::Power, true);
    sample_keys(2100);
    active->on_key(App::Key::Power, false);
    sample_keys(400); // The release belongs to Startup, not the loading page.
}
lv_obj_t* label(lv_obj_t* root, const char* text) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strcmp(lv_label_get_text(root), text) == 0)
        return root;
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = label(lv_obj_get_child(root, static_cast<int32_t>(i)), text))
            return found;
    }
    return nullptr;
}
void check_layout(lv_obj_t* root) {
    if (lv_obj_is_hidden(root)) {
        return;
    }
    if (lv_obj_check_type(root, &lv_label_class)) {
        const auto* text = lv_label_get_text(root);
        const auto* font = lv_obj_get_style_text_font(root, LV_PART_MAIN);
        for (uint32_t i = 0; text[i];) {
            const auto codepoint = lv_text_encoded_next(text, &i);
            if (codepoint == '\n')
                continue;
            lv_font_glyph_dsc_t glyph{};
            const bool found = lv_font_get_glyph_dsc(font, &glyph, codepoint, 0);
            if (!found || glyph.is_placeholder) {
                std::fprintf(stderr, "missing glyph U+%04x in %s\n", (unsigned)codepoint, text);
            }
            assert(found && !glyph.is_placeholder);
        }
        auto* parent = lv_obj_get_parent(root);
        assert(object_coords(root).x1 >= object_coords(parent).x1
               && object_coords(root).x2 <= object_coords(parent).x2);
        assert(object_coords(root).y1 >= object_coords(parent).y1
               && object_coords(root).y2 <= object_coords(parent).y2);
        if (object_coords(root).y1 < 0 || object_coords(root).y2 >= height) {
            std::fprintf(stderr, "outside: %s y=%d..%d\n", text, object_coords(root).y1, object_coords(root).y2);
        }
        assert(object_coords(root).y1 >= 0 && object_coords(root).y2 < height);
    }
    if (lv_obj_check_type(root, &lv_image_class)) {
        lv_point_t pivot;
        lv_image_get_pivot(root, &pivot);
        lv_area_t rendered;
        lv_image_buf_get_transformed_area(&rendered, lv_obj_get_width(root), lv_obj_get_height(root),
                                          static_cast<int16_t>(lv_image_get_rotation(root)), lv_image_get_scale_x(root),
                                          lv_image_get_scale_y(root), &pivot);
        auto* parent = lv_obj_get_parent(root);
        assert(rendered.x1 + object_coords(root).x1 >= object_coords(parent).x1
               && rendered.x2 + object_coords(root).x1 <= object_coords(parent).x2);
        assert(rendered.y1 + object_coords(root).y1 >= object_coords(parent).y1
               && rendered.y2 + object_coords(root).y1 <= object_coords(parent).y2);
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        check_layout(lv_obj_get_child(root, static_cast<int32_t>(i)));
    }
}
bool on_screen(lv_obj_t* obj) {
    // LVGL layer-top has OVERFLOW_VISIBLE: lv_obj_is_visible alone also accepts off-screen objects.
    return obj && lv_obj_is_visible(obj) && object_coords(obj).y2 >= 0 && object_coords(obj).y1 < height;
}

void
capture(lv_display_t* display, const std::string& directory, const std::string& name, bool status = true,
        unsigned settle_ms = 400) {
    advance(settle_ms);
    lv_obj_update_layout(lv_screen_active());
    // Inactive information cards are intentionally translated outside the viewport.
    const bool infos = std::strcmp(active->current_page(), "Pages/SystemInfos") == 0;
    auto* checked = lv_screen_active();
    if (infos) {
        auto* focused = active->focused();
        checked = lv_obj_get_parent(focused) == lv_screen_active() ? lv_obj_get_child(focused, 0)
                                                                   : lv_obj_get_parent(focused);
    }
    check_layout(checked);
    auto* position = label(lv_layer_top(), "DEMO");
    assert(position && on_screen(position) == status);
    lv_refr_now(display);
    if (directory.empty())
        return;
    auto* file = std::fopen((directory + "/" + name + ".ppm").c_str(), "wb");
    assert(file);
    assert(std::fprintf(file, "P6\n%d %d\n255\n", width, height) > 0);
    for (auto pixel : frame) {
        const unsigned char rgb[]{static_cast<unsigned char>(((pixel >> 11) & 31) * 255 / 31),
                                  static_cast<unsigned char>(((pixel >> 5) & 63) * 255 / 63),
                                  static_cast<unsigned char>((pixel & 31) * 255 / 31)};
        assert(std::fwrite(rgb, 1, 3, file) == 3);
    }
    assert(std::fclose(file) == 0);
}
void check_form_labels(lv_obj_t* obj);
void check_saveconfig_bounds(lv_obj_t* obj);

/** Verify the standby target, its direct-child labels and all content fit without cards. */
void check_startup() {
    auto* start = active->focused();
    assert(start && lv_obj_get_width(start) >= 100 && lv_obj_get_height(start) >= 90);
    assert(label(lv_screen_active(), "GNSS RTK"));
    assert(lv_group_get_obj_count(lv_group_get_default()) == 1);
    assert(lv_obj_is_clickable(start));
    assert(lv_obj_get_style_bg_opa(start, LV_PART_MAIN) == LV_OPA_TRANSP);
    assert(lv_obj_get_style_border_width(start, LV_PART_MAIN) == 0);
    auto* content = lv_obj_get_parent(start);
    check_form_labels(lv_obj_get_parent(content));
    check_saveconfig_bounds(content);
    auto* ring = lv_obj_get_child(start, 0);
    assert(lv_obj_check_type(ring, &lv_arc_class));
    assert(lv_arc_get_value(ring) == 0);
    assert(lv_obj_get_style_arc_opa(ring, LV_PART_MAIN) == LV_OPA_COVER);
    assert(lv_obj_get_style_arc_opa(ring, LV_PART_INDICATOR) == LV_OPA_COVER);
    for (auto id : {i18n::TextId::StartupStart, i18n::TextId::StartupHoldHint}) {
        auto* caption = label(lv_screen_active(), i18n::text(id));
        assert(caption);
        lv_point_t natural_size;
        lv_text_get_size(&natural_size, i18n::text(id), lv_obj_get_style_text_font(caption, LV_PART_MAIN), 0, 0,
                         LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        assert(natural_size.x <= lv_obj_get_width(caption));
        assert(lv_obj_get_parent(caption) == start); // The whole target is the hold area.
        assert(object_coords(caption).y1 > object_coords(ring).y2);
    }
    auto* action = label(start, i18n::text(i18n::TextId::StartupStart));
    auto* hint = label(start, i18n::text(i18n::TextId::StartupHoldHint));
    assert(object_coords(action).x2 < object_coords(hint).x1 && object_coords(action).y1 == object_coords(hint).y1);
}
// The standby battery slot has a 148-pixel interior and a four-pixel frame allowance.
constexpr int32_t kStartupBatteryWidth = 148;

/** Locate the battery slot in the transparent standby information block. */
lv_obj_t* battery_fill() {
    auto* caption = label(lv_screen_active(), "BAT");
    if (!caption)
        caption = label(lv_screen_active(), "CHG");
    assert(caption);
    auto* level = lv_obj_get_child(lv_obj_get_parent(caption), 3);
    assert(level && lv_obj_get_width(level) == kStartupBatteryWidth + 4 && lv_obj_get_height(level) == 10);
    auto* fill = lv_obj_get_child(level, 0);
    assert(fill);
    return fill;
}
// 状态栏电池槽内宽（px）；槽外框为 +4。
constexpr int32_t kStatusBatteryWidth = 20;
lv_obj_t* find_status_slot(lv_obj_t* root) {
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        auto* child = lv_obj_get_child(root, static_cast<int32_t>(i));
        if (lv_obj_get_width(child) == kStatusBatteryWidth + 4 && lv_obj_get_height(child) == 14
            && lv_obj_get_child_count(child) == 2) {
            return child;
        }
        if (auto* found = find_status_slot(child))
            return found;
    }
    return nullptr;
}
lv_obj_t* status_battery_fill() {
    auto* bar = lv_obj_get_child(lv_layer_top(), 0);
    assert(bar);
    auto* slot = find_status_slot(bar);
    assert(slot && "StatusBar battery slot missing");
    return lv_obj_get_child(slot, 0);
}

void
random_battery_events(lv_display_t* display, const std::string& output) {
    auto* fill = battery_fill();
    auto* status_fill = status_battery_fill();
    auto state = DemoStatus::sample(0);
    const auto check = [&] {
        active->update_status(state); // Exercise Application -> Status provider -> Model -> View.
        advance(40);
        const auto percent = state.battery_percent > 100 ? 100 : state.battery_percent;
        char text[24];
        std::snprintf(text, sizeof(text), "%u", percent);
        auto* block = lv_obj_get_parent(lv_obj_get_parent(fill));
        const bool charging = state.battery_valid && state.charging;
        auto* reading = lv_obj_get_child(block, 2);
        auto* digits = lv_obj_get_child(reading, 0);
        auto* unit = lv_obj_get_child(reading, 1);
        assert(lv_obj_is_hidden(digits) == !state.battery_valid);
        assert(label(reading, state.battery_valid ? text : "N/A"));
        assert(std::strcmp(lv_label_get_text(unit), state.battery_valid ? "%" : "N/A") == 0);
        assert(label(block, charging ? "CHG" : "BAT"));
        std::snprintf(text, sizeof(text), "%.2f V", state.battery_voltage);
        assert(label(block, state.battery_valid ? text : "N/A"));
        check_form_labels(block);
        check_layout(block);
        const auto color = !state.battery_valid        ? 0xa4adb5u
                           : charging || percent >= 50 ? 0x28c76fu
                           : percent >= 20             ? 0xff931eu
                           : percent >= 10             ? 0xea5455u
                                                       : 0xd03c3bu;
        auto* caption = label(block, charging ? "CHG" : "BAT");
        assert(lv_color_to_u32(lv_obj_get_style_text_color(caption, LV_PART_MAIN))
               == (lv_color_to_u32(charging ? lv_color_hex(color) : page::style::kMuted)));
        for (auto* current : {fill, status_fill}) {
            const auto full_width = current == fill ? kStartupBatteryWidth : kStatusBatteryWidth;
            auto* slot = lv_obj_get_parent(current);
            assert(lv_color_to_u32(lv_obj_get_style_bg_color(current, LV_PART_MAIN))
                   == lv_color_to_u32(lv_color_hex(color)));
            assert(lv_color_to_u32(lv_obj_get_style_border_color(slot, LV_PART_MAIN))
                   == lv_color_to_u32(lv_color_hex(color)));
            assert(lv_color_to_u32(lv_obj_get_style_bg_color(lv_obj_get_child(slot, 1), LV_PART_MAIN))
                   == lv_color_to_u32(lv_color_hex(color)));
            if (current == fill) {
                // 正极伸出槽框的部分也必须实际渲染，不能仅有样式而被 v9 裁掉。
                lv_refr_now(display);
                const auto terminal = object_coords(lv_obj_get_child(slot, 1));
                assert(frame[((terminal.y1 + terminal.y2) / 2) * width + terminal.x2]
                       == lv_color_to_u16(lv_color_hex(color)));
            }
            const auto target = state.battery_valid ? full_width * percent / 100 : 0;
            auto* animation = lv_anim_get(current, nullptr);
            if (charging) {
                assert(animation && animation->start_value == 0 && animation->end_value == full_width);
                assert(animation->reverse_duration == 0 && !animation->reverse_play_in_progress);
                assert(lv_obj_get_width(current) >= 0 && lv_obj_get_width(current) <= full_width);
            } else {
                assert(!animation && lv_obj_get_width(current) == static_cast<int32_t>(target));
            }
            assert(object_coords(current).x1 > object_coords(slot).x1
                   && object_coords(current).x2 < object_coords(slot).x2);
        }
        auto* numbers = lv_obj_get_child(lv_obj_get_parent(lv_obj_get_parent(status_fill)), -1);
        assert(lv_obj_is_hidden(numbers) == !state.battery_valid);
        if (state.battery_valid) {
            unsigned divisor = 100;
            for (unsigned digit = 0; digit < 3; ++digit, divisor /= 10) {
                auto* container = lv_obj_get_child(numbers, static_cast<int32_t>(digit));
                const bool hidden = digit == 0 ? percent < 100 : digit == 1 && percent < 10;
                assert(lv_obj_is_hidden(container) == hidden);
                if (hidden)
                    continue;
                auto* number = lv_obj_get_child(container, 0);
                const auto y = -static_cast<int32_t>((percent / divisor % 10)
                                                     * lv_obj_get_style_text_font(number, LV_PART_MAIN)->line_height);
                auto* animation = lv_anim_get(number, nullptr);
                assert((animation ? animation->end_value : lv_obj_get_y(number)) == y);
            }
        }
    };
    // Force threshold boundaries/full/empty while charging, independently of the seed.
    for (bool charging : {false, true}) {
        state.charging = charging;
        for (unsigned percent : {0u, 9u, 10u, 19u, 20u, 49u, 50u, 100u, 101u}) {
            state.battery_percent = percent;
            check();
        }
    }
    // At full actual charge, both slots still animate from zero and the number stays 100.
    advance(10000); // Let Startup's idle timer expose StatusBar for the joint captures.
    state.battery_percent = 100;
    state.charging = false;
    check();
    state.charging = true;
    active->update_status(state);
    lv_obj_update_layout(lv_screen_active());
    lv_obj_update_layout(lv_layer_top());
    assert(lv_obj_get_width(fill) == 0 && lv_obj_get_width(status_fill) == 0);
    capture(display, output, "Startup-StatusBar-charge-empty", on_screen(label(lv_layer_top(), "DEMO")), 0);
    advance(600);
    assert(lv_obj_get_width(fill) >= kStartupBatteryWidth * 45 / 100
           && lv_obj_get_width(fill) <= kStartupBatteryWidth * 55 / 100);
    assert(lv_obj_get_width(status_fill) >= 9 && lv_obj_get_width(status_fill) <= 11);
    capture(display, output, "Startup-StatusBar-charge-half", on_screen(label(lv_layer_top(), "DEMO")), 0);
    advance(650);
    assert(lv_obj_get_width(fill) == kStartupBatteryWidth && lv_obj_get_width(status_fill) == kStatusBatteryWidth);
    capture(display, output, "Startup-StatusBar-charge-full", on_screen(label(lv_layer_top(), "DEMO")), 0);
    advance(350);
    assert(lv_obj_get_width(fill) < kStartupBatteryWidth / 6 && lv_obj_get_width(status_fill) < 3);
    auto* reading = lv_obj_get_child(lv_obj_get_parent(lv_obj_get_parent(fill)), 2);
    assert(label(reading, "100") && label(reading, "%"));
    capture(display, output, "Startup-StatusBar-charge-reset", on_screen(label(lv_layer_top(), "DEMO")), 0);
    unsigned levels = 0, modes = 0;
    for (unsigned event = 0; event < 128; ++event) {
        const std::uint64_t elapsed = event * std::uint64_t{3000};
        state = DemoStatus::sample_random(elapsed, 42);
        const auto repeat = DemoStatus::sample_random(elapsed + 2999, 42);
        assert(state.battery_percent <= 100 && state.battery_valid && state.satellites <= 20);
        assert(state.battery_voltage >= 6.80f && state.battery_voltage <= 8.40f);
        assert(state.battery_percent == repeat.battery_percent && state.charging == repeat.charging
               && state.battery_voltage == repeat.battery_voltage && state.wifi == repeat.wifi
               && state.recording == repeat.recording && state.satellites == repeat.satellites);
        levels |= 1u << (state.battery_percent >= 50   ? 3
                         : state.battery_percent >= 20 ? 2
                         : state.battery_percent >= 10 ? 1
                                                       : 0);
        modes |= state.charging ? 2u : 1u;
        check();
        bool moved = false, status_moved = false, saw_reset = false;
        auto previous_width = lv_obj_get_width(fill);
        const auto first_status_width = lv_obj_get_width(status_fill);
        // Keep publishing throughout two animation cycles, just like the SDL host loop.
        for (unsigned frame = 0; frame < 15; ++frame) {
            advance(160);
            check();
            const auto current_width = lv_obj_get_width(fill);
            moved |= current_width != previous_width;
            saw_reset |= current_width < previous_width;
            status_moved |= lv_obj_get_width(status_fill) != first_status_width;
            previous_width = current_width;
        }
        if (state.charging)
            assert(moved && status_moved && saw_reset); // Grow from empty to full and restart, even at 100%.
        if (event < 4)
            capture(display, output, "Startup-random-42-" + std::to_string(event),
                    on_screen(label(lv_layer_top(), "DEMO")), 0);
    }
    assert(levels == 15 && modes == 3);
    // Retained/cached or recreated Startup must restart charging after a return.
    state.battery_percent = 40;
    state.charging = true;
    for (unsigned visit = 0; visit < 2; ++visit) {
        check();
        assert(active->show_page("Pages/SystemInfos"));
        assert(!lv_anim_get(fill, nullptr));
        active->update_status(state); // An update during the outgoing transition must not restart its animation.
        assert(!lv_anim_get(fill, nullptr));
        advance(400);
        assert(active->back());
        advance(400);
        fill = battery_fill();
        check();
    }
    state.battery_valid = false;
    check(); // Invalid data cancels both charging animations and clears both slots.
    capture(display, output, "Startup-battery-invalid", on_screen(label(lv_layer_top(), "DEMO")), 0);
    state.battery_valid = true;
    check();
    state.charging = false;
    check();
    std::puts("Startup/StatusBar random events: seed=42, 128 events, matching colors and 0-100% charging verified");
}

void
boot_sequence(lv_display_t* display, const std::string& output, const std::string& suffix) {
    const auto first_log = boot_logs.size();
    assert(std::strcmp(active->current_page(), "Pages/SystemLoading") == 0);
    capture(display, output, "SystemLoading-Logo" + suffix, false, 0);
    assert(!label(lv_screen_active(), i18n::text(i18n::TextId::SystemLoadingTitle)));
    unsigned frame_index = 0;
    uint32_t last_frame_ms = lv_tick_get() - 100;
    const auto step_frames = [&](unsigned ms) {
        for (unsigned elapsed = 0; elapsed < ms; elapsed += 10) {
            advance(10);
            if (!output.empty() && lv_tick_elaps(last_frame_ms) >= 100
                && std::strcmp(active->current_page(), "Pages/SystemLoading") == 0) {
                char name[48];
                std::snprintf(name, sizeof(name), "Boot-frame%s-%03u", suffix.c_str(), frame_index++);
                capture(display, output, name, false, 0);
                last_frame_ms = lv_tick_get();
            }
        }
    };
    const auto wait_for = [&](i18n::TextId id) {
        for (unsigned elapsed = 0; elapsed < 3500; elapsed += 10) {
            assert(std::strcmp(active->current_page(), "Pages/SystemLoading") == 0);
            if (label(lv_screen_active(), i18n::text(id))) {
                return;
            }
            step_frames(10);
        }
        assert(false && "boot phase deadline exceeded");
    };
    const i18n::TextId stages[] = {i18n::TextId::LoadingConfiguration, i18n::TextId::PreparingServices,
                                   i18n::TextId::StartingGnssService, i18n::TextId::PreparingNetwork,
                                   i18n::TextId::FinalizingStartup};
    for (unsigned i = 0; i < 5; ++i) {
        wait_for(stages[i]);
        // Steps take their real (simulated) time; the network check only fails once it finishes.
        for (unsigned ms = 0; i == 3 && ms < 2000; ms += 10) {
            if (label(lv_screen_active(), i18n::text(i18n::TextId::InitializationFailedContinue))) {
                break;
            }
            step_frames(10);
        }
        // Stage 3 capture around 60%, like the reference; a finished (failed) stage is shown only briefly.
        step_frames(i == 2 ? 400 : i == 3 ? 0 : 180);
        capture(display, output, "SystemLoading-Step" + std::to_string(i + 1) + suffix, false, 0);
        auto* headline = label(lv_screen_active(), i18n::text(stages[i]));
        assert(headline && label(lv_screen_active(), "DEMO"));
        assert(red_nodes(lv_screen_active()) == (i >= 3 ? 1u : 0u));
        if (i == 3) {
            assert(lv_color_to_u32(lv_obj_get_style_text_color(headline, LV_PART_MAIN))
                   == lv_color_to_u32(lv_color_hex(0xd03c3b)));
            assert(label(lv_screen_active(), i18n::text(i18n::TextId::InitializationFailedContinue)));
        }
        assert(lv_obj_get_style_border_width(lv_obj_get_parent(headline), LV_PART_MAIN) == 0);
        assert(lv_obj_get_style_outline_width(lv_obj_get_parent(headline), LV_PART_MAIN) == 0);
        lv_point_t text_size;
        lv_text_get_size(&text_size, lv_label_get_text(headline), lv_obj_get_style_text_font(headline, LV_PART_MAIN), 0,
                         0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        assert(text_size.x <= lv_obj_get_width(headline)); // No silently clipped translated headline.
        char step[40];
        std::snprintf(step, sizeof(step), i18n::text(i18n::TextId::InitializationStep), static_cast<int>(i + 1));
        assert(label(lv_screen_active(), step));
    }
    wait_for(i18n::TextId::ReadyWithWarnings);
    step_frames(260); // Failed nodes stay red throughout the completion animation.
    capture(display, output, "SystemLoading-Ready" + suffix, false, 0);
    assert(label(lv_screen_active(), "100%") && label(lv_screen_active(), i18n::text(i18n::TextId::StartupCompleted)));
    assert(label(lv_screen_active(), i18n::text(i18n::TextId::ServicesLimited)));
    assert(red_nodes(lv_screen_active()) == 1);
    assert(!label(lv_screen_active(), i18n::text(i18n::TextId::SystemReady)));
    auto* ready_panel = lv_obj_get_parent(label(lv_screen_active(), i18n::text(i18n::TextId::ReadyWithWarnings)));
    assert(lv_obj_get_style_border_width(ready_panel, LV_PART_MAIN) == 0);
    assert(lv_obj_get_style_outline_width(ready_panel, LV_PART_MAIN) == 0);
    step_frames(1200);
    advance(400);
    assert(std::strcmp(active->current_page(), "Pages/Dialplate") == 0);
    std::size_t cursor = first_log;
    const char* names[] = {"configuration", "services", "gnss", "network", "finalization"};
    for (unsigned i = 0; i < 5; ++i) {
        for (const char* outcome : {"begin", i == 3 ? "FAILED" : "OK"}) {
            char expected[96];
            std::snprintf(expected, sizeof(expected), "step %u/5 %s: %s [DEMO]", i + 1, names[i], outcome);
            while (cursor < boot_logs.size() && boot_logs[cursor].find(expected) == std::string::npos)
                ++cursor;
            assert(cursor < boot_logs.size()); // Each failed result must be followed by later begin/results.
            ++cursor;
        }
    }
}
void focused(i18n::TextId id) {
    assert(active->focused() && label(active->focused(), i18n::text(id)));
}
void check_form_labels(lv_obj_t* obj) {
    if (lv_obj_is_hidden(obj)) {
        return;
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        const char* text = lv_label_get_text(obj);
        const auto* font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_point_t natural;
        lv_text_get_size(&natural, text, font, 0, lv_obj_get_style_text_line_space(obj, LV_PART_MAIN), LV_COORD_MAX,
                         LV_TEXT_FLAG_NONE);
        if (natural.x > lv_obj_get_width(obj) || natural.y > lv_obj_get_height(obj))
            std::fprintf(stderr, "clipped: %s needs %dx%d, has %dx%d\n", text, natural.x, natural.y,
                         lv_obj_get_width(obj), lv_obj_get_height(obj));
        assert(natural.x <= lv_obj_get_width(obj) && natural.y <= lv_obj_get_height(obj));
        assert(object_coords(obj).x1 >= 0 && object_coords(obj).x2 < width && object_coords(obj).y1 >= 0
               && object_coords(obj).y2 < height);
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); ++i) {
        check_form_labels(lv_obj_get_child(obj, static_cast<int32_t>(i)));
    }
}
void choose(i18n::TextId id) {
    for (unsigned i = 0; i < 8 && !label(active->focused(), i18n::text(id)); ++i)
        input(App::InputAction::NextFocus);
    focused(id);
}
/** Check opaque content stays below the status bar, allowing only the thin battery slot's border. */
void check_saveconfig_bounds(lv_obj_t* obj) {
    assert(object_coords(obj).y1 >= 26 && object_coords(obj).y2 < height);
    assert(object_coords(obj).x1 >= 0 && object_coords(obj).x2 < width);
    assert(lv_obj_get_style_opa(obj, LV_PART_MAIN) == LV_OPA_COVER);
    const bool battery_slot = lv_obj_get_width(obj) == kStartupBatteryWidth + 4 && lv_obj_get_height(obj) == 10;
    assert(lv_obj_get_style_border_width(obj, LV_PART_MAIN) == (battery_slot ? 1 : 0));
    assert(lv_obj_get_style_bg_grad_dir(obj, LV_PART_MAIN) == LV_GRAD_DIR_NONE);
    assert(lv_obj_get_style_shadow_width(obj, LV_PART_MAIN) == 0);
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); ++i) {
        check_saveconfig_bounds(lv_obj_get_child(obj, static_cast<int32_t>(i)));
    }
}

/** Verify the fixed shutdown segment, its repeating rotation and the centered large power icon. */
void check_saveconfig_spinner(lv_obj_t* ring, lv_anim_exec_xcb_t countdown) {
    assert(!lv_anim_get(ring, countdown));
    auto* spinner = lv_anim_get(ring, nullptr);
    assert(spinner && spinner->duration == 1200 && spinner->repeat_cnt == LV_ANIM_REPEAT_INFINITE);
    assert(spinner->end_value - spinner->start_value == 360 && spinner->path_cb == lv_anim_path_linear);
    assert(lv_arc_get_angle_start(ring) == 0 && lv_arc_get_angle_end(ring) == 90);
    assert(lv_color_to_u32(lv_obj_get_style_arc_color(ring, LV_PART_INDICATOR))
           == lv_color_to_u32(page::style::kDanger));
    assert(lv_color_to_u32(lv_obj_get_style_arc_color(ring, LV_PART_MAIN)) == lv_color_to_u32(page::style::kPanel));
    assert(lv_obj_is_hidden(lv_obj_get_child(ring, 0)));
    assert(lv_obj_is_hidden(lv_obj_get_child(ring, 1)));
    auto* icon = lv_obj_get_child(ring, 2);
    assert(!lv_obj_is_hidden(icon));
    assert(lv_image_get_src(icon) == resource_pool::get_image("ui_power_large"));
    assert(lv_color_to_u32(lv_obj_get_style_image_recolor(icon, LV_PART_MAIN))
           == lv_color_to_u32(page::style::kDanger));
    lv_obj_update_layout(ring);
    assert(lv_obj_get_x(icon) * 2 + lv_obj_get_width(icon) == lv_obj_get_width(ring));
    assert(lv_obj_get_y(icon) * 2 + lv_obj_get_height(icon) == lv_obj_get_height(ring));
}

/** Exercise direct shutdown entry, user execution, terminal updates and animation cleanup on reappearance. */
void saveconfig_view_lifecycle() {
    const auto animations = lv_anim_count_running();
    auto* root = page::style::box(lv_screen_active(), 0, 0, width, height);
    page::SaveConfigView view;
    view.create(root);
    auto* ring = lv_obj_get_child(root, 0);
    auto* countdown = view.progress_animation();
    assert(lv_anim_get_playtime(countdown) == 8000 && countdown->path_cb == lv_anim_path_linear);
    DataProc::SystemState state{};
    state.power_pending = true;
    state.power_action = DataProc::PowerAction::Off;
    view.appear();
    view.update(state);
    assert(label(ring, "8") && lv_arc_get_value(ring) == 0);
    lv_anim_start(countdown);
    advance(100);
    state.power_pending = false;
    state.shutting_down = true;
    state.power_cause = DataProc::PowerCause::User;
    view.update(state);
    check_saveconfig_spinner(ring, countdown->exec_cb);
    assert(label(root, i18n::text(i18n::TextId::SaveConfigPowerOffPushKey)));
    state.shutting_down = false;
    view.update(state); // A later publication cannot resume a countdown during this visit.
    check_saveconfig_spinner(ring, countdown->exec_cb);
    assert(label(root, i18n::text(i18n::TextId::SaveConfigKeepPower)));
    view.stop();
    assert(!lv_anim_get(ring, nullptr));
    assert(!lv_anim_get(lv_obj_get_child(root, 1), nullptr));
    state.power_pending = true;
    view.appear();
    view.update(state);
    assert(label(ring, "8") && lv_arc_get_value(ring) == 0);
    assert(lv_arc_get_angle_start(ring) == 0 && lv_arc_get_angle_end(ring) == 0);
    assert(!lv_obj_is_hidden(lv_obj_get_child(ring, 0)));
    assert(lv_obj_is_hidden(lv_obj_get_child(ring, 2)));
    state.shutting_down = true;
    view.appear();
    view.update(state); // A visit that starts during shutdown goes straight to the spinner.
    check_saveconfig_spinner(ring, countdown->exec_cb);
    view.stop();
    assert(!lv_anim_get(ring, nullptr));
    assert(!lv_anim_get(lv_obj_get_child(root, 1), nullptr));
    lv_obj_delete(root);
    advance(9000);
    assert(lv_anim_count_running() == animations);
}

/** Exercise bilingual power actions, live platform takeover and the visible completion interval. */
void
saveconfig_scenarios(lv_display_t* display, const std::string& output) {
    using DataProc::PowerAction;
    using DataProc::PowerCause;
    using i18n::TextId;
    saveconfig_view_lifecycle();
    for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
        for (unsigned scenario = 0; scenario < 5; ++scenario) {
            i18n::set_language(language);
            {
                App app;
                active = &app;
                assert(app.init());
                advance(400);
                product_start();
                advance(7000);
                assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
                assert(app.show_page("Pages/SystemDash"));
                advance(600);
                choose(TextId::PowerTitle);
                input(App::InputAction::Confirm);
                // Platform scenarios take over an already visible Restarting page.
                const bool restart = scenario != 0;
                if (restart)
                    input(App::InputAction::NextFocus);
                product_hold(App::Key::Power, 2300);
                assert(std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
                auto* action = label(lv_screen_active(), i18n::text(restart ? TextId::SaveConfigRestarting
                                                                            : TextId::SaveConfigPoweringOff));
                assert(action);
                auto* root = lv_obj_get_parent(lv_obj_get_parent(action));
                auto* ring = lv_obj_get_child(root, 0);
                assert(lv_obj_check_type(ring, &lv_arc_class));
                auto* seconds = label(ring, "8");
                assert(seconds
                       && lv_obj_get_style_text_font(seconds, LV_PART_MAIN)
                              == resource_pool::get_font("rajdhaniBold_40"));
                auto* countdown = lv_anim_get(ring, nullptr);
                assert(countdown && countdown->duration == 8000 && countdown->path_cb == lv_anim_path_linear);
                const auto countdown_callback = countdown->exec_cb;
                unsigned deadline = 0;
                for (auto* timer = lv_timer_get_next(nullptr); timer; timer = lv_timer_get_next(timer)) {
                    if (timer->period == 8500) {
                        assert(deadline == 0);
                        deadline = timer->last_run + timer->period;
                    }
                }
                assert(deadline > lv_tick_get());
                assert(lv_arc_get_min_value(ring) == 0 && lv_arc_get_max_value(ring) == 1000);
                assert(lv_image_get_src(lv_obj_get_child(ring, 0)) == resource_pool::get_image("ui_power"));
                assert(lv_obj_is_hidden(lv_obj_get_child(ring, 2)));
                assert(lv_color_to_u32(lv_obj_get_style_arc_color(ring, LV_PART_INDICATOR))
                       == (lv_color_to_u32(restart ? page::style::kAccent : page::style::kDanger)));
                assert(label(root, i18n::text(TextId::SaveConfigWarning)));
                assert(label(root, i18n::text(TextId::SaveConfigCancelHint)));
                assert(!label(root, i18n::text(TextId::SaveConfigKeepPower)));
                const auto calls = SystemService::snapshot().power_calls;
                const char* names[] = {"Off", "Reboot", "Host", "LowBattery", "Forced"};
                if (scenario >= 2) {
                    const auto cause = scenario == 2   ? PowerCause::Host
                                       : scenario == 3 ? PowerCause::LowBattery
                                                       : PowerCause::Forced;
                    const auto id = scenario == 2   ? TextId::SaveConfigPowerOffBoard
                                    : scenario == 3 ? TextId::SaveConfigPowerOffLowBattery
                                                    : TextId::SaveConfigPowerOffForced;
                    assert(SystemService::begin_shutdown(cause));
                    // No tick or navigation: the System publication must update the existing widgets.
                    assert(label(root, i18n::text(id)));
                    assert(lv_color_to_u32(lv_obj_get_style_arc_color(ring, LV_PART_INDICATOR))
                           == lv_color_to_u32(page::style::kDanger));
                    assert(label(root, i18n::text(TextId::SaveConfigPoweringOff)));
                    assert(!label(root, i18n::text(TextId::SaveConfigRestarting)));
                    assert(!label(root, i18n::text(TextId::SaveConfigCancelHint)));
                    assert(label(root, i18n::text(TextId::SaveConfigKeepPower)));
                    assert(!SystemService::snapshot().power_pending);
                    assert(app.back() && std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
                    check_saveconfig_spinner(ring, countdown_callback);
                }
                advance(20);
                lv_obj_update_layout(root);
                check_form_labels(root);
                for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
                    check_saveconfig_bounds(lv_obj_get_child(root, static_cast<int32_t>(i)));
                }
                const auto name = std::string("SaveConfig-") + names[scenario]
                                  + (language == i18n::Language::English ? "-en" : "-ru");
                capture(display, output, name, true, 0);
                if (scenario < 2) {
                    unsigned steps = 0;
                    auto previous = lv_arc_get_value(ring);
                    int previous_seconds = 8;
                    bool captured_midpoint = false;
                    for (unsigned elapsed = 0; elapsed < 8000 && !label(ring, "0"); elapsed += 10) {
                        assert(SystemService::snapshot().power_calls == calls);
                        advance(10);
                        const auto value = lv_arc_get_value(ring);
                        assert(value >= previous);
                        steps += value != previous;
                        previous = value;
                        const int remaining = ((1000 - value) * 8 + 999) / 1000;
                        char text[16];
                        std::snprintf(text, sizeof(text), "%d", remaining);
                        assert(std::strcmp(lv_label_get_text(seconds), text) == 0);
                        assert(remaining == previous_seconds || remaining == previous_seconds - 1);
                        previous_seconds = remaining;
                        if (!captured_midpoint && remaining == 4) {
                            capture(display, output, name + "-mid", true, 0);
                            captured_midpoint = true;
                        }
                    }
                    assert(steps > 100 && captured_midpoint);
                    assert(label(ring, "0") && lv_obj_get_style_opa(seconds, LV_PART_MAIN) == LV_OPA_COVER);
                    assert(lv_arc_get_value(ring) == 1000 && lv_arc_get_angle_end(ring) == 360);
                    assert(!lv_anim_get(ring, countdown_callback));
                    assert(SystemService::snapshot().power_calls == calls);
                    const auto remaining = deadline - lv_tick_get();
                    assert(remaining >= 480 && remaining <= 520); // Visible full ring precedes the 500 ms hold.
                    lv_obj_update_layout(root);
                    check_form_labels(root);
                    advance(remaining - 10);
                    assert(SystemService::snapshot().power_calls == calls);
                    advance(10); // Execute at exactly the unchanged 8500 ms controller deadline.
                    assert(SystemService::snapshot().power_calls == calls + 1);
                    assert(SystemService::snapshot().power_action
                           == (restart ? PowerAction::Reboot : PowerAction::Off));
                    advance(400);
                    assert(std::strcmp(app.current_page(), restart ? "Pages/Dialplate" : "Pages/Startup") == 0);
                } else {
                    const auto rotation = lv_arc_get_rotation(ring);
                    const auto value = lv_arc_get_value(ring);
                    advance(200);
                    assert(lv_arc_get_rotation(ring) != rotation);
                    capture(display, output, name + "-spinner", true, 0);
                    advance(8500);
                    check_saveconfig_spinner(ring, countdown_callback);
                    assert(lv_arc_get_value(ring) == value);
                    assert(SystemService::snapshot().power_calls == calls);
                    assert(std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
                    assert(app.back() && std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
                    assert(SystemService::enter_standby());
                    advance(600);
                    assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
                }
                active = nullptr;
            }
            advance(9000); // Destruction must clear callbacks and timers borrowing the old page.
            assert(DataProc::Broker()->getNodeNumber() == 0);
            assert(lv_anim_count_running() == 0);
        }
    }
}
} // namespace
int main(int argc, char** argv) {
    assert(argc <= 2);
    const std::string output = argc == 2 ? argv[1] : "";
    if (!output.empty())
        std::filesystem::create_directories(output);
    app_log_set_level(APP_LOG_INFO);
    app_log_set_sink(boot_log_sink, nullptr);
    lv_init();
    lv_display_t* display = lv_display_create(width, height);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    assert(display);
    {
        App app;
        active = &app;
        assert(app.init());
        advance(400);
        assert(label(lv_screen_active(), "1.0.0") && label(lv_screen_active(), "85") && label(lv_screen_active(), "%")
               && label(lv_screen_active(), "7.60 V"));
        assert(!label(lv_screen_active(), "N/A"));
        assert(!on_screen(label(lv_layer_top(), "DEMO")));
        check_startup();
        capture(display, output, "Startup-hidden-en", false);
        advance(10000);
        capture(display, output, "Startup-idle-en");
        product_click(App::Key::Power);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0 && label(lv_screen_active(), "0%"));
        app.on_key(App::Key::Power, true);
        sample_keys(500);
        assert(!label(lv_screen_active(), "0%"));
        auto* start_label = label(app.focused(), i18n::text(i18n::TextId::StartupStart));
        assert(start_label
               && lv_color_to_u32(lv_obj_get_style_text_color(start_label, LV_PART_MAIN))
                      == lv_color_to_u32(page::style::kHighlight));
        capture(display, output, "Startup-hold-en");
        app.on_key(App::Key::Power, false);
        sample_keys(800);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0 && label(lv_screen_active(), "0%"));
        app.on_key(App::Key::Power, true);
        sample_keys(500);
        app.cancel_input();
        sample_keys(2500);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0 && label(lv_screen_active(), "0%"));
        product_start();
        boot_sequence(display, output, "-en");
        assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
        assert(!app.back());
        app.update_status(DemoStatus::sample(16000));
        input(App::InputAction::NextFocus);
        input(App::InputAction::Confirm);
        assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
        assert(!app.show_page("Pages/Shutdown"));
        focused(i18n::TextId::LanguageTitle);
        assert(!app.show_page("Pages/SystemSettings"));
        for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
            choose(i18n::TextId::LanguageTitle);
            if (i18n::get_language() != language) {
                input(App::InputAction::Confirm);
            }
            assert(i18n::get_language() == language);
            const std::string suffix = language == i18n::Language::English ? "-en" : "-ru";
            focused(i18n::TextId::LanguageTitle);
            assert(lv_group_get_obj_count(lv_group_get_default()) == 3); // Disabled Wi-Fi is skipped.
            capture(display, output, "SystemDash" + suffix, false);
            auto* wifi = label(lv_screen_active(), i18n::text(i18n::TextId::SystemWifiTitle));
            assert(wifi && lv_obj_is_disabled(lv_obj_get_parent(wifi)));
            assert(!label(lv_screen_active(), i18n::text(i18n::TextId::DateTimeTitle)));
            product_click(App::Key::Function);
            focused(i18n::TextId::PowerTitle);
            product_backlog_click(App::Key::Function);
            focused(i18n::TextId::Back);
            product_click(App::Key::Power);
            assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
            assert(!SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 0);
            advance(600);
            assert(on_screen(label(lv_layer_top(), "DEMO")));
            product_click(App::Key::Power); // Home remembers the System entry.
            focused(i18n::TextId::LanguageTitle);
            product_click(App::Key::Function);
            focused(i18n::TextId::PowerTitle);
            input(App::InputAction::Confirm);
            focused(i18n::TextId::ShutdownAction); // No Cancel button: a short press dismisses.
            assert(lv_color_to_u32(lv_obj_get_style_bg_color(app.focused(), LV_PART_MAIN))
                   == lv_color_to_u32(page::style::kDanger)); // Red when focused.
            assert(lv_group_get_obj_count(lv_group_get_default()) == 2);
            capture(display, output, "Power" + suffix, false);
            input(App::InputAction::NextFocus);
            focused(i18n::TextId::RebootAction);
            app.on_input(App::InputAction::Press); // Half a hold: the bar shows progress.
            advance(1000);
            capture(display, output, "Power-Hold" + suffix, false);
            app.on_input(App::InputAction::Release); // Released early: closes, nothing prepared.
            advance(400);
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0
                   && SystemService::snapshot().power_calls == 0 && !SystemService::snapshot().power_pending);
            focused(i18n::TextId::PowerTitle);
            assert(!on_screen(label(lv_layer_top(), "DEMO")));
            assert(app.show_page("Pages/SystemInfos"));
            advance(400);
            assert(label(lv_screen_active(), "31.230400\n121.473700\n12.50 m")); // Simulated Device data.
            assert(label(lv_screen_active(), "1.0.0\n02:15:30\n1\nDEMO-001"));
            assert(!label(lv_screen_active(), "N/A"));
            assert(!DataProc::Broker()->search("SystemLoadingModel"));
            assert(!label(lv_screen_active(), i18n::text(i18n::TextId::SystemInitialization)));
            assert(!label(lv_screen_active(), i18n::text(i18n::TextId::InitializationConfig)));
            assert(lv_group_get_obj_count(lv_group_get_default()) == 7);
            auto* entry_root = app.focused();
            assert(lv_obj_get_parent(entry_root) == lv_screen_active());
            assert(lv_obj_get_style_outline_width(entry_root, LV_PART_MAIN) == 0);
            product_click(App::Key::Function); // root -> Work
            focused(i18n::TextId::SystemWorkTitle);
            product_click(App::Key::Function);
            focused(i18n::TextId::SystemGpsTitle); // One Next must advance to the next visible inventory section.
            input(App::InputAction::PreviousFocus);
            focused(i18n::TextId::SystemWorkTitle);
            auto* scroller = lv_obj_get_parent(lv_obj_get_parent(app.focused()));
            assert(lv_obj_get_height(scroller) == height && lv_obj_get_scroll_y(scroller) == 0);
            int32_t previous_scroll_y = 0;
            for (unsigned card = 0; card < 6; ++card) {
                const auto scroll_y = lv_obj_get_scroll_y(scroller);
                assert(scroll_y >= previous_scroll_y); // Forward traversal must never scroll upward.
                previous_scroll_y = scroll_y;
                focused(inventory_titles[card]);
                capture(display, output, "SystemInfos-" + std::to_string(card) + suffix, false);
                product_click(App::Key::Function);
            }
            assert(app.focused() == entry_root);
            assert(lv_obj_get_scroll_y(scroller) == 0);
            input(App::InputAction::PreviousFocus);
            focused(i18n::TextId::SystemTitle);
            product_click(App::Key::Power); // 选择信息卡直接返回。
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
            assert(app.show_page("Pages/SystemInfos"));
            advance(400);
            assert(lv_obj_get_parent(app.focused()) == lv_screen_active()); // 每次进入从根页开始。
            assert(label(lv_screen_active(), "1.0.0\n02:15:30\n1\nDEMO-001"));
            assert(lv_group_get_obj_count(lv_group_get_default()) == 7);
            capture(display, output, "SystemInfos-Reopen" + suffix, false);
            product_click(App::Key::Power); // 根页也可点击返回。
            assert(!on_screen(label(lv_layer_top(), "DEMO")));
            assert(app.show_page("Pages/StarMap"));
            advance(600);
            assert(app.focused() && lv_obj_is_focused(app.focused()));
            assert(lv_obj_get_style_outline_width(app.focused(), LV_PART_MAIN) == 0);
            assert(lv_obj_get_style_border_width(app.focused(), LV_PART_MAIN) == 0);
            capture(display, output, "StarMap-no-outline" + suffix);
            assert(label(lv_screen_active(), "4") && label(lv_screen_active(), "3")
                   && label(lv_screen_active(), "DEMO"));
            assert(!label(lv_screen_active(), "-"));
            input(App::InputAction::Confirm);
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
        }
        choose(i18n::TextId::PowerTitle);
        auto* dash_root = app.focused();
        while (lv_obj_get_parent(dash_root) != lv_screen_active()) {
            dash_root = lv_obj_get_parent(dash_root);
        }
        auto* dash = static_cast<PageBase*>(lv_obj_get_user_data(dash_root));
        assert(dash && dash->getState() == PageBase::STATE::ACTIVITY);
        auto* manager = dash->getManager();
        unsigned rejected = 0;
        manager->setEventCallback(reject_save_navigation, &rejected);
        product_click(App::Key::Power);
        product_hold(App::Key::Power, 2300);
        assert(rejected == 1);
        assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
        assert(!manager->isSwitching());
        assert(!SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 0);
        focused(i18n::TextId::PowerTitle);
        manager->setEventCallback(nullptr, nullptr);
        product_click(App::Key::Power);
        product_hold(App::Key::Power, 2300);
        assert(std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
        assert(SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 0);
        product_click(App::Key::Function, true); // Cancel before the simulated service runs.
        assert(!SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 0);
        focused(i18n::TextId::PowerTitle); // Leaving for SaveConfig closed the dialog.
        // Both held actions run only after SaveConfig's timer; cancel also clears the transaction.
        for (auto action : {DataProc::PowerAction::Off, DataProc::PowerAction::Reboot}) {
            const auto open = [&] {
                choose(i18n::TextId::PowerTitle);
                product_click(App::Key::Power);
                focused(i18n::TextId::ShutdownAction);
                if (action == DataProc::PowerAction::Reboot) {
                    product_click(App::Key::Function); // Function still moves focus inside the dialog.
                    focused(i18n::TextId::RebootAction);
                }
            };
            const auto calls = SystemService::snapshot().power_calls;
            open();
            product_click(App::Key::Power); // A short press dismisses without preparing anything.
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0 && !SystemService::snapshot().power_pending
                   && SystemService::snapshot().power_calls == calls);
            focused(i18n::TextId::PowerTitle); // The dismissing press must not reopen the dialog.
            open();
            product_hold(App::Key::Power, 1200); // Released before 100%.
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0
                   && !SystemService::snapshot().power_pending);
            focused(i18n::TextId::PowerTitle);
            open();
            product_hold(App::Key::Power, 2300);
            assert(std::strcmp(app.current_page(), "Pages/SaveConfig") == 0
                   && SystemService::snapshot().power_calls == calls && SystemService::snapshot().power_pending
                   && SystemService::snapshot().power_action == action);
            product_click(App::Key::Power, true); // Duplicate commit cannot create another request.
            advance(12000);
            assert(SystemService::snapshot().power_calls == calls + 1
                   && SystemService::snapshot().power_action == action);
            assert(!SystemService::snapshot().power_pending);
            if (action == DataProc::PowerAction::Off) {
                assert(std::strcmp(app.current_page(), "Pages/Startup") == 0 && !app.back());
                assert(label(lv_screen_active(), "0%"));
                assert(!DataProc::Broker()->search("DialplateModel"));
                assert(!DataProc::Broker()->search("SystemDashModel"));
                assert(!DataProc::Broker()->search("SaveConfigModel"));
                capture(display, output, "PowerOff-Startup", false);
                product_click(App::Key::Power); // Short click cannot restart after shutdown.
                assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
                product_start();
                boot_sequence(display, output, "-ru");
            }
            assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0 && !app.back());
            auto* restored_root = app.focused();
            assert(restored_root);
            while (lv_obj_get_parent(restored_root) != lv_screen_active()) {
                restored_root = lv_obj_get_parent(restored_root);
            }
            assert(lv_obj_get_x(restored_root) == 0 && lv_obj_get_y(restored_root) == 0);
            assert(lv_obj_get_style_opa(restored_root, LV_PART_MAIN) == LV_OPA_COVER);
            assert(app.show_page("Pages/SystemDash"));
            advance(400);
        }
        assert(app.show_page("Pages/SystemInfos"));
        advance(400);
        for (unsigned card = 0; card < 6; ++card) {
            capture(display, output, "SystemInfos-" + std::to_string(card), false);
            input(App::InputAction::NextFocus);
        }
        input(App::InputAction::Back);
        advance(400);
        assert(!on_screen(label(lv_layer_top(), "DEMO")));
        input(App::InputAction::Back);
        advance(300);
        assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0 && !app.back());
        assert(on_screen(label(lv_layer_top(), "DEMO"))); // Returning to the working root restores the bar.
        active = nullptr;
    }
    assert(DataProc::Broker()->getNodeNumber() == 0);
    // A fresh application before any startup reports zero initialization errors and six inventory controls.
    for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
        i18n::set_language(language);
        App app;
        active = &app;
        assert(app.init());
        advance(400);
        check_startup();
        const std::string suffix = language == i18n::Language::English ? "-en" : "-ru";
        capture(display, output, "Startup-modern" + suffix, false);
        assert(app.show_page("Pages/SystemInfos"));
        advance(400);
        assert(label(lv_screen_active(), "1.0.0\n02:15:30\n0\nDEMO-001"));
        assert(lv_group_get_obj_count(lv_group_get_default()) == 7);
        assert(lv_obj_get_parent(app.focused()) == lv_screen_active());
        assert(!label(lv_screen_active(), i18n::text(i18n::TextId::SystemInitialization)));
        product_click(App::Key::Function); // root -> Work
        for (unsigned section = 0; section < 6; ++section) {
            focused(inventory_titles[section]);
            product_click(App::Key::Function);
        }
        assert(lv_obj_get_parent(app.focused()) == lv_screen_active());
        product_click(App::Key::Function, true);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
        active = nullptr;
    }
    assert(DataProc::Broker()->getNodeNumber() == 0);
    saveconfig_scenarios(display, output);
    app_log_set_sink(nullptr, nullptr);
    // The View also handles all-success and multiple-failure outcomes, then resets on fresh initialization.
    auto* canvas = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(canvas);
    lv_obj_set_size(canvas, width, height);
    page::SystemLoadingView view;
    view.create(canvas);
    for (bool fail : {false, true}) {
        view.show_logo();
        view.show_initialization();
        assert(!view.has_failures() && red_nodes(canvas) == 0);
        for (unsigned i = 0; i < page::SystemLoadingView::kStepCount; ++i) {
            view.begin_step(i);
            view.show_waiting(i * 1500); // Longer checks show their elapsed seconds.
            view.finish_step(i, !(fail && (i == 1 || i == 3)));
            advance(page::SystemLoadingView::kProgressMs + 20);
        }
        view.show_ready();
        advance(260);
        assert(view.has_failures() == fail && red_nodes(canvas) == (fail ? 2u : 0u));
        assert(label(canvas, i18n::text(fail ? i18n::TextId::ReadyWithWarnings : i18n::TextId::SystemReady)));
        view.destroy();
    }
    lv_obj_delete(canvas);
    // 新表单英俄文字、交互和活动页面退出后的定时器清理。
    for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
        i18n::set_language(language);
        for (const char* page : {"Pages/WorkSettings", "Pages/RecordConfig", "Pages/SystemDash"}) {
            {
                App app;
                active = &app;
                assert(app.init());
                app.update_status(DemoStatus::sample(0));
                advance(400);
                assert(app.show_page(page));
                auto* entering = app.focused();
                assert(lv_anim_get(entering, page::settings_style::move_in));
                advance(600);
                assert(!lv_anim_get(entering, page::settings_style::move_in));
                assert(lv_obj_get_style_translate_y(entering, LV_PART_MAIN) == 0);
                auto* root = app.focused();
                while (lv_obj_get_parent(root) != lv_screen_active()) {
                    root = lv_obj_get_parent(root);
                }
                check_form_labels(root);
                capture(display, output,
                        std::string(page + 6) + (language == i18n::Language::English ? "-new-en" : "-new-ru"),
                        std::strcmp(page, "Pages/SystemDash") != 0);
                if (std::strcmp(page, "Pages/WorkSettings") == 0) {
                    input(App::InputAction::Confirm); // Rover -> Base
                    input(App::InputAction::Confirm); // Base -> Network
                    focused(i18n::TextId::NetworkBase);
                    assert(lv_group_get_obj_count(lv_group_get_default()) == 3);
                    check_form_labels(root);
                    capture(display, output,
                            std::string("NetworkBase-new-") + (language == i18n::Language::English ? "en" : "ru"));
                } else if (std::strcmp(page, "Pages/RecordConfig") == 0) {
                    choose(i18n::TextId::StartupStart);
                    input(App::InputAction::Confirm); // 单击即开始
                    assert(DeviceService::snapshot().recording);
                    assert(lv_group_get_obj_count(lv_group_get_default()) == 2);
                    check_form_labels(root);
                    capture(display, output,
                            std::string("Recording-new-") + (language == i18n::Language::English ? "en" : "ru"));
                    input(App::InputAction::Confirm);
                    assert(!DeviceService::snapshot().recording);
                    assert(lv_group_get_obj_count(lv_group_get_default()) == 4);
                }
                // 留在表单上销毁 App，ASan 检查定时器不再借用 Controller。
                active = nullptr;
            }
            advance(16000);
            assert(DataProc::Broker()->getNodeNumber() == 0);
        }
    }
    // Test battery levels and charging animations on Startup page
    {
        App app;
        active = &app;
        assert(app.init());
        advance(400);
        assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);

        // 1. High battery (≥50%): Green #28C76F, BAT label, no charging
        page::StatusBarState state{};
        state.battery_valid = true;
        state.battery_percent = 85;
        state.charging = false;
        state.battery_voltage = 7.60f;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "85") && label(lv_screen_active(), "%") && label(lv_screen_active(), "7.60 V")
               && label(lv_screen_active(), "BAT"));
        capture(display, output, "Startup-battery-85-green", false);

        // 2. Medium battery (20%~49%): Amber #FF931E
        state.battery_percent = 35;
        state.battery_voltage = 7.45f;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "35") && label(lv_screen_active(), "%") && label(lv_screen_active(), "7.45 V")
               && label(lv_screen_active(), "BAT"));
        capture(display, output, "Startup-battery-35-orange", false);

        // 3. Low battery (10%~19%): Red #EA5455
        state.battery_percent = 15;
        state.battery_voltage = 7.20f;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "15") && label(lv_screen_active(), "%") && label(lv_screen_active(), "7.20 V")
               && label(lv_screen_active(), "BAT"));
        capture(display, output, "Startup-battery-15-red", false);

        // 4. Critical low battery (<10%): Dark red #D03C3B
        state.battery_percent = 5;
        state.battery_voltage = 6.90f;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "5") && label(lv_screen_active(), "%") && label(lv_screen_active(), "6.90 V")
               && label(lv_screen_active(), "BAT"));
        capture(display, output, "Startup-battery-5-critical", false);

        // 5. Charging state: CHG label, animated fill progressing towards full width
        state.battery_percent = 40;
        state.battery_voltage = 7.80f;
        state.charging = true;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "40") && label(lv_screen_active(), "%") && label(lv_screen_active(), "7.80 V")
               && label(lv_screen_active(), "CHG"));
        capture(display, output, "Startup-battery-charging-start", false);
        advance(600); // Wait mid-animation
        capture(display, output, "Startup-battery-charging-mid", false);
        advance(600); // Completion of one charge cycle
        capture(display, output, "Startup-battery-charging-full", false);

        // 6. Unplug charger: returns to BAT, static width
        state.charging = false;
        app.update_status(state);
        advance(50);
        assert(label(lv_screen_active(), "40") && label(lv_screen_active(), "%") && label(lv_screen_active(), "7.80 V")
               && label(lv_screen_active(), "BAT"));
        capture(display, output, "Startup-battery-unplugged", false);
        random_battery_events(display, output);

        active = nullptr;
    }
    assert(DataProc::Broker()->getNodeNumber() == 0);
    lv_display_delete(display);
}
