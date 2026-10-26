#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "App.h"
#include "ButtonGesture.h"
#include "DemoStatus.h"
#include "DomainTestInput.hpp"
#include "I18n.h"
#include "PageManager.h"
#include "UI/Resource/LogoParts.h"
#include "UI/Resource/ResourcePool.h"
#include "UI/SystemLoading/SystemLoadingView.h"
#include "hal_device_checks.h"

/** @brief 通过公开 API 读取对象的屏幕绝对坐标供布局断言使用。 */
static lv_area_t
object_coords(const lv_obj_t* object) {
    lv_area_t area;
    lv_obj_get_coords(object, &area);
    return area;
}

constexpr int width_px = 294;
constexpr int height_px = 126;
static uint16_t frame[width_px * height_px];
static uint16_t dialplate_frame[width_px * height_px];
static uint16_t draw_pixels[width_px * 20];
static unsigned flushes;
static size_t peak_nodes;
static size_t peak_pool;

/** @brief Dialplate 的焦点覆盖框：页面根下唯一的浮动子对象（shared_layer）。 */
static lv_obj_t*
find_highlight(lv_obj_t* page_root) {
    for (uint32_t i = 0; i < lv_obj_get_child_count(page_root); ++i) {
        auto* child = lv_obj_get_child(page_root, static_cast<int32_t>(i));
        if (lv_obj_is_floating(child)) {
            return child;
        }
    }
    return nullptr;
}

static lv_obj_t* find_label(lv_obj_t* root, const char* text) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strstr(lv_label_get_text(root), text))
        return root;
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_label(lv_obj_get_child(root, static_cast<int32_t>(i)), text))
            return found;
    }
    return nullptr;
}

static lv_obj_t* find_image(lv_obj_t* root, const void* source) {
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_image_class) && lv_image_get_src(root) == source) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_image(lv_obj_get_child(root, static_cast<int32_t>(i)), source))
            return found;
    }
    return nullptr;
}

namespace {
// 建页期间推进 200 ms，模拟 MCU 上耗时较长的页面创建。
class SlowPage final : public PageBase {
  public:
    /** @brief Simulate a slow load without spending real time. */
    void
    onViewLoad() override {
        lv_tick_inc(200);
    }
};
class RootPage final : public PageBase {};
} // namespace

static void advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        const size_t nodes = DataProc::Broker()->getNodeNumber();
        if (nodes > peak_nodes) {
            peak_nodes = nodes;
        }
        lv_timer_handler();
        lv_mem_monitor_t memory;
        lv_mem_monitor(&memory);
        if (memory.max_used > peak_pool) {
            peak_pool = memory.max_used;
        }
    }
}

static void
flush(lv_display_t* driver, const lv_area_t* area, uint8_t* px_map) {
    uint16_t* pixels = (uint16_t*)px_map;
    assert(area->x1 >= 0 && area->y1 >= 0 && area->x2 < width_px && area->y2 < height_px);
    const int width = area->x2 - area->x1 + 1;
    for (int y = area->y1; y <= area->y2; ++y)
        std::memcpy(&frame[y * width_px + area->x1],
                    &pixels[static_cast<std::size_t>(y - area->y1) * static_cast<std::size_t>(width)],
                    width * sizeof(uint16_t));
    ++flushes;
    lv_display_flush_ready(driver);
}

int main(int argc, char** argv) {
    if (argc > 3)
        return 2;
    ButtonGesture gesture;
    using Action = ButtonGesture::Action;
    assert(gesture.sample(true, 0) == Action::None);
    assert(gesture.sample(false, 5) == Action::None); // bounce
    assert(gesture.sample(true, 10) == Action::None);
    assert(gesture.sample(true, 35) == Action::None);
    assert(gesture.sample(false, 60) == Action::None);
    assert(gesture.sample(false, 85) == Action::None);
    assert(gesture.sample(false, 336) == Action::Single);
    assert(gesture.sample(true, 400) == Action::None);
    assert(gesture.sample(true, 425) == Action::None);
    assert(gesture.sample(false, 445) == Action::None);
    assert(gesture.sample(false, 470) == Action::None);
    assert(gesture.sample(true, 500) == Action::None);
    assert(gesture.sample(true, 525) == Action::None);
    assert(gesture.sample(false, 550) == Action::None);
    assert(gesture.sample(false, 575) == Action::Double);
    assert(gesture.sample(false, 900) == Action::None);
    ButtonGesture boundary;
    boundary.sample(true, 1000);
    boundary.sample(true, 1020);
    boundary.sample(false, 1040);
    boundary.sample(false, 1060);
    assert(boundary.sample(true, 1305) == Action::None); // Press within 250ms, debounce past deadline.
    assert(boundary.sample(true, 1325) == Action::None);
    boundary.sample(false, 1340);
    assert(boundary.sample(false, 1360) == Action::Double);
    assert(boundary.sample(false, 1700) == Action::None);
    boundary.cancel(true);
    assert(boundary.sample(false, 1800) == Action::None);
    assert(boundary.sample(false, 1820) == Action::None);
    assert(boundary.sample(false, 2200) == Action::None); // A cancelled held key is not a click.

    lv_init();
    lv_display_t* display = lv_display_create(width_px, height_px);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_pixels, NULL, sizeof(draw_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    assert(display);
    {
        // Building a page is not elapsed transition time: the first frame starts at the beginning of the slide.
        PageManager manager;
        assert(manager.registerPage(new RootPage, "Test/Root") == PageManager::RES_TYPE::OK);
        assert(manager.registerPage(new SlowPage, "Test/Slow") == PageManager::RES_TYPE::OK);
        manager.setGlobalLoadAnim(PAGE_ANIM::MOVE_LEFT, 260, lv_anim_path_linear);
        assert(manager.push("Test/Root") == PageManager::RES_TYPE::OK);
        // The UI always has some animation running (status icons, dashboard cards); with an empty list LVGL would
        // reset its clock on the next start and hide the problem.
        lv_anim_t background;
        lv_anim_init(&background);
        lv_anim_set_var(&background, &background);
        lv_anim_set_exec_cb(&background, [](void*, int32_t) {});
        lv_anim_set_values(&background, 0, 100);
        lv_anim_set_duration(&background, 1000);
        lv_anim_set_repeat_count(&background, LV_ANIM_REPEAT_INFINITE);
        assert(lv_anim_start(&background));
        advance(400);
        assert(manager.push("Test/Slow") == PageManager::RES_TYPE::OK);
        lv_tick_inc(10);
        lv_timer_handler();
        auto* slow_root = manager.getCurrentPage()->getRoot();
        assert(lv_obj_get_style_x(slow_root, LV_PART_MAIN) > width_px * 3 / 4); // Not 210 of 260 ms already gone.
        advance(400);
        assert(lv_obj_get_style_x(slow_root, LV_PART_MAIN) == 0);
        lv_anim_delete(&background, nullptr);
    }
    {
        demo::model().power = {};
        App app;
        assert(app.init());
        test_input::status(DemoStatus::sample(0));
        using Input = App::InputAction;
        if (argc == 3) {
            lv_tick_inc(400);
            lv_timer_handler(); // 初始页面切换完成后才可再次 Push
            test_input::status(DemoStatus::sample(16000));
            assert(app.show_page(argv[1]));
            lv_tick_inc(500);
            lv_timer_handler();
        } else {
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            assert(app.focused());
            advance(400); // Finish initial transition before testing actual input.
            auto* startup_status = find_label(lv_layer_top(), "DEMO");
            assert(startup_status && object_coords(startup_status).y2 < 0);
            app.on_input(Input::Confirm);
            advance(300);
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            app.on_input(Input::Press);
            advance(500);
            assert(!find_label(lv_screen_active(), "0%"));
            app.on_input(Input::Release); // A short press must not start.
            advance(400);
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            assert(find_label(lv_screen_active(), "0%"));
            app.on_input(Input::Press);
            advance(500);
            app.cancel_input(); // SDL focus loss cancels rather than confirming the held control.
            app.on_input(Input::Release);
            advance(2500);
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            assert(find_label(lv_screen_active(), "0%"));
            // LVGL pointer presses bypass App::pressed_; window loss must still cancel their hold.
            lv_obj_set_pressed(app.focused(), true);
            lv_obj_send_event(app.focused(), LV_EVENT_PRESSED, nullptr);
            advance(500);
            app.cancel_input();
            assert(!lv_obj_is_pressed(app.focused()));
            advance(2500);
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            assert(find_label(lv_screen_active(), "0%"));
            advance(2000);
            assert(object_coords(startup_status).y2 < 0); // Still below the ten-second Startup idle deadline.
            advance(1500);
            assert(object_coords(startup_status).y1 >= 0); // Delay expired; slide-in completed.
            app.on_input(Input::Press);
            advance(1000);
            app.on_input(Input::Press); // Repeated keydown cannot restart the timer.
            advance(1100);
            assert(std::strcmp(app.current_page(), "Pages/SystemLoading") == 0);
            const auto* loading_logo = resource_pool::get_image("oemLogoParts");
            assert(loading_logo);
            loading_logo = static_cast<const logo_parts_t*>(loading_logo)->parts[0].image;
            assert(find_image(lv_screen_active(), loading_logo));
            assert(!find_label(lv_screen_active(), i18n::text(i18n::TextId::SystemLoadingTitle)));
            lv_tick_inc(5000); // A stalled host frame must not lose the loading completion request.
            lv_timer_handler();
            assert(std::strcmp(app.current_page(), "Pages/SystemLoading") == 0);
            advance(20);
            assert(object_coords(startup_status).y2 < 0); /** @brief 长帧之后状态栏已完全移出屏幕。 */
            assert(!app.back());                   // Loading replaced Startup, rather than retaining it underneath.
            app.on_input(Input::Release);          // Cross-page release must not activate the next page.
            advance(400);
            assert(!find_image(lv_screen_active(), loading_logo));
            assert(find_label(lv_screen_active(), i18n::text(i18n::TextId::SystemLoadingTitle)));
            // Steps take their simulated check time (0/400/1200/700/200 ms, each shown at least 300 ms).
            assert(find_label(lv_screen_active(), i18n::text(i18n::TextId::LoadingConfiguration))
                   || find_label(lv_screen_active(), i18n::text(i18n::TextId::PreparingServices)));
            assert(find_label(lv_screen_active(), i18n::text(i18n::TextId::PreparingModules)));
            advance(1600);
            auto* progress = find_label(lv_screen_active(), "%");
            assert(progress && std::atoi(lv_label_get_text(progress)) >= 40);
            assert(std::strcmp(app.current_page(), "Pages/SystemLoading") == 0);
            advance(2400);
            assert(find_label(lv_screen_active(), i18n::text(i18n::TextId::ReadyWithWarnings)));
            advance(1400);
            assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
            assert(!DataProc::Broker()->search("StartupModel"));
            assert(!DataProc::Broker()->search("SystemLoadingModel"));
            assert(!app.back()); // Dialplate is the root, not a child of the boot pages.
            lv_obj_t* initial_focus = app.focused();
            assert(initial_focus);
            app.on_input(Input::NextFocus);
            assert(app.focused() != initial_focus);
            app.on_input(Input::PreviousFocus);
            assert(app.focused() == initial_focus);
            auto* home_root = initial_focus;
            while (lv_obj_get_parent(home_root) != lv_screen_active()) {
                home_root = lv_obj_get_parent(home_root);
            }
            auto* panel = find_highlight(home_root);
            assert(panel && lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_TRANSP);
            app.on_input(Input::Confirm);
            // The focused entry expands over the home page first (spring steps are capped at 64 ms, so deliver
            // frames), then the page switches without a slide once the panel covers the screen.
            advance(60);
            assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
            assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_COVER);
            assert(object_coords(panel).x2 - object_coords(panel).x1
                   > object_coords(initial_focus).x2 - object_coords(initial_focus).x1);
            app.on_input(Input::NextFocus); // Focus is frozen until the navigation happens.
            assert(app.focused() == initial_focus);
            advance(240);
            assert(std::strcmp(app.current_page(), "Pages/SystemInfos") == 0);
            advance(50);
            app.on_input(Input::Back);
            advance(10); // Leaving an expanded entry is immediate; the home page then collapses the panel.
            assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
            assert(app.focused() == initial_focus);
            lv_obj_update_layout(home_root);
            assert(object_coords(panel).x1 <= 0 && object_coords(panel).x2 >= width_px - 1);
            advance(1500);
            lv_obj_update_layout(home_root);
            const auto collapsed = object_coords(panel), entry = object_coords(initial_focus);
            assert(collapsed.x1 == entry.x1 && collapsed.y1 == entry.y1 && collapsed.x2 == entry.x2
                   && collapsed.y2 == entry.y2);
            assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_TRANSP);
            app.on_input(Input::NextFocus); // Unfrozen again.
            assert(app.focused() != initial_focus);
            app.on_input(Input::PreviousFocus);
            advance(800);
            // Re-entering while the panel is still collapsing expands again instead of falling back to a slide.
            app.on_input(Input::Confirm);
            advance(300);
            assert(std::strcmp(app.current_page(), "Pages/SystemInfos") == 0);
            app.on_input(Input::Back);
            advance(100);
            assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) < LV_OPA_COVER); // Fading while collapsing.
            app.on_input(Input::Confirm);
            advance(300);
            assert(std::strcmp(app.current_page(), "Pages/SystemInfos") == 0);
            auto* infos_root = app.focused();
            while (lv_obj_get_parent(infos_root) != lv_screen_active()) {
                infos_root = lv_obj_get_parent(infos_root);
            }
            assert(lv_obj_get_x(infos_root) == 0); // Switched without the page's own slide.
            app.on_input(Input::Back);
            advance(1500);
            assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_TRANSP);
        }
        lv_refr_now(display);
        assert(flushes);
        unsigned lit = 0;
        for (const auto& pixel : frame)
            lit += ((pixel >> 11) & 31) || ((pixel >> 5) & 63) || (pixel & 31);
        assert(lit > 100);
        if (argc != 3)
            std::memcpy(dialplate_frame, frame, sizeof(frame));
        if (argc >= 2) {
            FILE* image = std::fopen(argv[argc - 1], "wb");
            assert(image);
            assert(std::fprintf(image, "P6\n%d %d\n255\n", width_px, height_px) > 0);
            for (const uint16_t pixel : frame) {
                const std::uint8_t rgb[] = {(std::uint8_t)(((pixel >> 11) & 31) * 255 / 31),
                                            (std::uint8_t)(((pixel >> 5) & 63) * 255 / 63),
                                            (std::uint8_t)((pixel & 31) * 255 / 31)};
                assert(std::fwrite(rgb, 1, sizeof(rgb), image) == sizeof(rgb));
            }
            assert(std::fclose(image) == 0);
        }
        if (argc != 3) {
            const struct {
                const char* name;
                const char* label;
            } pages[] = {
                {"Pages/WorkSettings", "TRIMTALK"}, {"Pages/RecordConfig", "XYZ"},
                {"Pages/SystemInfos", "Firmware"},  {"Pages/SystemDash", "Language"},
                {"Pages/StarMap", "SATELLITES"},    {"Pages/SaveConfig", "Powering off"},
            };
            for (const auto& page : pages) {
                lv_tick_inc(300);
                lv_timer_handler();
                test_input::status(DemoStatus::sample(16000));
                demo::model().recorder.recording = demo::model().recorder.reported_recording = false;
                lv_tick_inc(500);
                DataProc_RunLoop();
                assert(app.show_page(page.name));
                assert(std::strcmp(app.current_page(), page.name) == 0);
                auto* page_root = find_label(lv_screen_active(), page.label);
                assert(page_root);
                while (lv_obj_get_parent(page_root) != lv_screen_active()) {
                    page_root = lv_obj_get_parent(page_root);
                }
                if (std::strcmp(page.name, "Pages/SaveConfig") == 0) {
                    auto* seconds = find_label(page_root, "8");
                    assert(seconds && lv_obj_get_style_opa(seconds, LV_PART_MAIN) == LV_OPA_COVER);
                }
                // Pages slide (no page-level opacity layer): every child of the home page pushes in from the right
                // over a parallax-shifted, scrim-dimmed home; only the save page covers from the bottom.
                const auto expected_anim =
                    std::strcmp(page.name, "Pages/SaveConfig") == 0 ? PAGE_ANIM::OVER_TOP : PAGE_ANIM::PUSH_LEFT;
                const auto* controller = static_cast<PageBase*>(lv_obj_get_user_data(page_root));
                assert(controller && controller->getLoadAnimType() == expected_anim);
                assert(controller->getLoadAnimType() != PAGE_ANIM::FADE_ON);
                advance(120);
                if (expected_anim == PAGE_ANIM::FADE_ON) {
                    const auto opa = lv_obj_get_style_opa(page_root, LV_PART_MAIN);
                    assert(opa > LV_OPA_TRANSP && opa < LV_OPA_COVER);
                } else if (expected_anim == PAGE_ANIM::PUSH_LEFT) {
                    assert(lv_obj_get_x(page_root) > 0 && lv_obj_get_x(page_root) < width_px);
                } else if (expected_anim == PAGE_ANIM::OVER_BOTTOM) {
                    assert(lv_obj_get_y(page_root) < 0 && lv_obj_get_y(page_root) > -height_px);
                } else {
                    assert(lv_obj_get_y(page_root) > 0 && lv_obj_get_y(page_root) < height_px);
                }
                advance(480); // Page and StatusBar animations both need frame delivery.
                assert(lv_obj_get_style_x(page_root, LV_PART_MAIN) == 0
                       && lv_obj_get_style_y(page_root, LV_PART_MAIN) == 0);
                assert(lv_obj_get_y(page_root) == (std::strcmp(page.name, "Pages/StarMap") == 0 ? 26 : 0));
                assert(lv_obj_get_style_opa(page_root, LV_PART_MAIN) == LV_OPA_COVER);
                const bool work = std::strcmp(page.name, "Pages/WorkSettings") == 0;
                const bool record = std::strcmp(page.name, "Pages/RecordConfig") == 0;
                const bool infos = std::strcmp(page.name, "Pages/SystemInfos") == 0;
                const bool system_dash = std::strcmp(page.name, "Pages/SystemDash") == 0;
                const bool star_map = std::strcmp(page.name, "Pages/StarMap") == 0;
                auto* position = find_label(lv_layer_top(), "DEMO");
                assert(position && (object_coords(position).y2 >= 0) == !infos);
                if (work || record || infos || system_dash || star_map) {
                    lv_obj_t* first = app.focused();
                    assert(first);
                    if (star_map) {
                        assert(lv_obj_is_focused(page_root));
                        assert(lv_obj_get_style_outline_width(page_root, LV_PART_MAIN) == 0);
                        assert(lv_obj_get_style_border_width(page_root, LV_PART_MAIN) == 0);
                    }
                    app.on_input(Input::NextFocus);
                    if (!star_map)
                        assert(app.focused() != first);
                    if (record) {
                        app.on_input(Input::PreviousFocus);
                        assert(find_label(app.focused(), i18n::text(i18n::TextId::StartupStart)));
                        // 开始与停止一致：单击即切换，双击（Commit）同样有效。
                        app.on_input(Input::Confirm);
                        assert(hal_test::snapshot<dp::Recorder>().recording);
                        assert(find_label(lv_screen_active(), i18n::text(i18n::TextId::RequestReceived)));
                        assert(lv_group_get_obj_count(lv_group_get_default()) == 2);
                        app.on_input(Input::Confirm);
                        assert(!hal_test::snapshot<dp::Recorder>().recording);
                        assert(lv_group_get_obj_count(lv_group_get_default()) == 4);
                        app.on_input(Input::Commit);
                        assert(hal_test::snapshot<dp::Recorder>().recording);
                        app.on_input(Input::Commit);
                        assert(!hal_test::snapshot<dp::Recorder>().recording);
                    }
                    if (work) {
                        assert(find_label(app.focused(), "TRIMTALK"));
                        app.on_input(Input::Press);
                        assert(lv_obj_is_pressed(app.focused()));
                        assert(find_label(app.focused(), "TRIMTALK"));
                        app.on_input(Input::Release);
                        assert(find_label(app.focused(), "TRIMMK3"));
                    }
                } else {
                    assert(!app.focused()); // 无可操作控件的静态页
                }
                if (infos) {
                    lv_area_t hor_bar, ver_bar;
                    auto* badge = find_label(page_root, "DEMO");
                    assert(badge);
                    auto* scroller = lv_obj_get_parent(lv_obj_get_parent(badge));
                    lv_obj_get_scrollbar_area(scroller, &hor_bar, &ver_bar);
                    assert(ver_bar.x1 < ver_bar.x2 && object_coords(badge).x2 < ver_bar.x1);
                    assert(!find_label(page_root, i18n::text(i18n::TextId::InfoBrowseHint)));
                }
                if (std::strcmp(page.name, "Pages/SaveConfig") == 0) {
                    auto* action = find_label(page_root, i18n::text(i18n::TextId::SaveConfigPoweringOff));
                    assert(action && object_coords(lv_obj_get_parent(action)).y1 >= 26);
                    assert(find_label(page_root, i18n::text(i18n::TextId::SaveConfigWarning)));
                }
                lv_refr_now(display);
                assert(std::memcmp(frame, dialplate_frame, sizeof(frame)) != 0);
                if (infos || star_map) {
                    app.on_input(Input::Confirm);
                } else {
                    assert(app.back());
                }
                if (expected_anim == PAGE_ANIM::MOVE_TOP) {
                    assert(lv_obj_get_style_y(page_root, LV_PART_MAIN) == 0); // No bottom-alignment jump at pop start.
                }
                lv_tick_inc(50);
                lv_timer_handler();
                assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
                lv_obj_update_layout(
                    page_root); // Animation setters dirty layout; getters otherwise see the last frame.
                if (expected_anim == PAGE_ANIM::FADE_ON) {
                    assert(lv_obj_get_style_opa(page_root, LV_PART_MAIN) < LV_OPA_COVER);
                } else if (expected_anim == PAGE_ANIM::PUSH_LEFT) {
                    assert(lv_obj_get_x(page_root) > 0); // Back reverses the direction, not another left wipe.
                } else if (expected_anim == PAGE_ANIM::OVER_BOTTOM) {
                    assert(lv_obj_get_style_y(page_root, LV_PART_MAIN) < 0);
                } else
                    assert(lv_obj_get_style_y(page_root, LV_PART_MAIN) > 0);
                assert(app.focused());
                assert(lv_group_get_focus_cb(lv_group_get_default()) == nullptr);
            }
            // Cancelling Logo, Initialization or Ready must remove timers/animations, not navigate later.
            constexpr uint32_t kLogoMs = page::SystemLoadingView::kLogoMs;
            for (const auto phase_ms : {400u, kLogoMs + 300, kLogoMs + 3150}) {
                advance(300);
                assert(app.show_page("Pages/SystemLoading"));
                advance(phase_ms);
                assert(app.show_page("Pages/SystemInfos")); // Loading stays cached beneath this page.
                advance(400);
                assert(app.back());
                advance(400);
                assert(std::strcmp(app.current_page(), "Pages/SystemLoading") == 0);
                advance(kLogoMs + 200); /** @brief OEM Logo 结束后，首步淡入已完成且尚未跳过最短展示窗口。 */
                auto* restarted = find_label(lv_screen_active(), i18n::text(i18n::TextId::LoadingConfiguration));
                assert(restarted && lv_obj_get_style_opa(restarted, LV_PART_MAIN) == LV_OPA_COVER);
                assert(!find_label(lv_screen_active(), i18n::text(i18n::TextId::ReadyWithWarnings)));
                assert(app.back());
                advance(4000);
                assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
                assert(!DataProc::Broker()->search("SystemLoadingModel"));
                assert(!app.back());
            }
            lv_tick_inc(300);
            lv_timer_handler();
            assert(app.show_page("Pages/SystemDash"));
            lv_tick_inc(500);
            lv_timer_handler();
            app.on_input(Input::NextFocus); // Wi-Fi 不可用，Language -> Power。
            app.on_input(Input::Confirm);
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
            app.on_input(Input::Commit); // Shutdown is focused; a double press no longer executes it.
            assert(std::strcmp(app.current_page(), "Pages/SystemDash") == 0);
            app.on_input(Input::Press); // Hold Power until the bar reaches 100%.
            advance(2100);
            assert(std::strcmp(app.current_page(), "Pages/SaveConfig") == 0);
            app.on_input(Input::Release);
            lv_tick_inc(50);
            lv_timer_handler();
            // Step real UI frames so save completion cannot race a still-busy page animation.
            for (int elapsed = 0; elapsed < 9000; elapsed += 10) {
                lv_tick_inc(10);
                lv_timer_handler();
            }
            assert(std::strcmp(app.current_page(), "Pages/Startup") == 0);
            assert(app.focused() && find_label(lv_screen_active(), "0%"));
            assert(!app.back()); // Off resets the root; no previous working/boot history remains.
        }
    }
    std::printf("Linux app: LVGL pool peak %zu / %u bytes, DataNode peak %zu\n", peak_pool,
                static_cast<unsigned>(LV_MEM_SIZE), peak_nodes);
    lv_display_delete(display);
}
