#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "App.h"
#include "UI/Common/PageStyle.h"

namespace {
constexpr unsigned kFrameMs = 10;
uint16_t pixels[294 * 20];

struct Frame {
    uint32_t tick;
    lv_area_t area;
    int step, fill, border;
};

struct Capture {
    lv_obj_t* panel = nullptr;
    bool active = false;
    bool returned = false;
    unsigned render_ms = 0;
    std::vector<Frame> frames;
} capture;

lv_obj_t*
root_of(lv_obj_t* object) {
    while (lv_obj_get_parent(object) != lv_screen_active()) {
        object = lv_obj_get_parent(object);
    }
    return object;
}

lv_obj_t*
highlight(lv_obj_t* root) {
    if (!lv_obj_is_clickable(root) && lv_obj_get_style_border_width(root, LV_PART_MAIN) == 2
        && lv_color_to_u32(lv_obj_get_style_border_color(root, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kAccent)) {
        return root;
    }
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = highlight(lv_obj_get_child(root, i))) {
            return found;
        }
    }
    return nullptr;
}

void
flush(lv_display_t* display, const lv_area_t*, uint8_t*) {
    // 与已有慢建页回归相同，用虚拟 tick 模拟同步渲染/发送耗时，不等待真实时间。
    if (capture.active && capture.returned && lv_display_flush_is_last(display)) {
        lv_tick_inc(capture.render_ms);
        capture.render_ms = 0;
    }
    lv_display_flush_ready(display);
}

void
frame_ready(lv_event_t*) {
    if (!capture.active || !capture.returned) {
        return;
    }
    Frame frame{};
    frame.tick = lv_tick_get();
    lv_obj_get_coords(capture.panel, &frame.area);
    frame.fill = lv_obj_get_style_bg_opa(capture.panel, LV_PART_MAIN);
    frame.border = lv_obj_get_style_border_opa(capture.panel, LV_PART_MAIN);
    const auto* animation = lv_anim_get(capture.panel, nullptr);
    // 盖满后仍可能有静默展开尾段；此处记录收回进度，完整不透明面板对应 step 0。
    frame.step = frame.fill == LV_OPA_COVER ? 0 : (animation ? animation->current_value : 1024);
    capture.frames.push_back(frame);
}

void
advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += kFrameMs) {
        lv_tick_inc(kFrameMs);
        DataProc_RunLoop();
        lv_timer_handler();
        // 固定帧边界先采动画再渲染，避免两个独立定时器的相位差混入可见时长。
        lv_anim_refr_now();
        lv_refr_now(lv_display_get_default());
    }
}

void
check_return(unsigned unload_ms, unsigned render_ms, bool interrupt, bool measure_only) {
    App app;
    assert(app.init());
    advance(400);
    assert(app.show_page("Pages/Dialplate"));
    advance(1200);
    auto* initial = app.focused();
    auto* home = root_of(initial);
    auto* panel = highlight(home);
    assert(panel);
    app.on_input(App::InputAction::Confirm);
    advance(450);
    assert(std::strcmp(app.current_page(), "Pages/Dialplate") != 0);
    auto* child = root_of(app.focused());
    // 子对象删除发生在 PageManager::stateUnload() 的 lv_obj_clean 中，晚于首页 WillAppear。
    auto* slow = lv_obj_create(child);
    lv_obj_add_event_cb(
        slow,
        [](lv_event_t* event) {
            lv_tick_inc(*static_cast<unsigned*>(lv_event_get_user_data(event)));
            capture.returned = true;
        },
        LV_EVENT_DELETE, &unload_ms);
    capture = {panel, true, false, render_ms, {}};
    assert(app.back());
    advance(20);
    assert(std::strcmp(app.current_page(), "Pages/Dialplate") == 0);
    assert(app.focused() == initial);
    if (!measure_only) {
        // 恢复焦点后的样式/父容器布局通知不得走用户换焦点的 finish_close 分支。
        auto* animation = lv_anim_get(panel, nullptr);
        assert(animation);
        lv_obj_send_event(initial, LV_EVENT_STYLE_CHANGED, nullptr);
        lv_obj_send_event(lv_obj_get_parent(initial), LV_EVENT_LAYOUT_CHANGED, nullptr);
        assert(lv_anim_get(panel, nullptr) == animation);
    }
    if (interrupt) {
        advance(80);
        assert(lv_anim_get(panel, nullptr));
        assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) > LV_OPA_TRANSP);
        assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) < LV_OPA_COVER);
        app.on_input(App::InputAction::NextFocus);
        assert(app.focused() != initial);
        assert(!lv_anim_get(panel, nullptr));
        assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_TRANSP);
    }
    advance(500);
    capture.active = false;
    assert(!capture.frames.empty());
    const auto& first = capture.frames.front();
    const Frame* last = nullptr;
    for (const auto& frame : capture.frames) {
        std::printf("frame unload=%u render=%u interrupt=%d t=%u step=%d xywh=%d,%d,%d,%d fill=%d border=%d\n",
                    unload_ms, render_ms, interrupt, frame.tick - first.tick, frame.step, frame.area.x1, frame.area.y1,
                    lv_area_get_width(&frame.area), lv_area_get_height(&frame.area), frame.fill, frame.border);
        if (!last && frame.fill == LV_OPA_TRANSP) {
            last = &frame;
        }
    }
    assert(last);
    std::printf("return unload=%u render=%u interrupt=%d first_step=%d visible_ms=%u\n", unload_ms, render_ms,
                interrupt, first.step, last->tick - first.tick);
    if (!measure_only && !interrupt) {
        assert(first.step <= 8);
        assert(first.fill >= 253 && first.border <= 2);
        assert(first.area.x1 <= 0 && first.area.x2 >= 293);
        assert(first.area.y1 <= 0 && first.area.y2 >= 125);
        const auto duration = last->tick - first.tick;
        assert(duration >= page::style::kPageTransitionMs - kFrameMs
               && duration <= page::style::kPageTransitionMs + kFrameMs);
        // 缓入曲线的前 10 ms 可能量化为 0；20 ms 内须出现移动，不得加入满屏等待定时器。
        bool moved = false;
        for (const auto& frame : capture.frames) {
            if (frame.tick - first.tick <= 2 * kFrameMs && frame.step > first.step) {
                moved = true;
            }
        }
        assert(moved);
        for (std::size_t i = 1; i < capture.frames.size(); ++i) {
            assert(capture.frames[i].step >= capture.frames[i - 1].step);
            assert(capture.frames[i].fill <= capture.frames[i - 1].fill);
        }
    }
    if (!measure_only && interrupt) {
        assert(lv_obj_get_style_bg_opa(panel, LV_PART_MAIN) == LV_OPA_TRANSP);
    }
    lv_area_t actual, target;
    lv_obj_get_coords(panel, &actual);
    lv_obj_get_coords(app.focused(), &target);
    assert(actual.x1 == target.x1 && actual.y1 == target.y1 && actual.x2 == target.x2 && actual.y2 == target.y2);
}
} // namespace

int
main(int argc, char**) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    lv_init();
    auto* display = lv_display_create(294, 126);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    lv_timer_set_period(lv_display_get_refr_timer(display), UINT32_MAX);
    assert(lv_display_add_event_cb(display, frame_ready, LV_EVENT_RENDER_READY, nullptr));
    for (const auto unload : {0U, 200U}) {
        for (const auto render : {0U, 200U}) {
            check_return(unload, render, false, argc > 1);
        }
    }
    if (argc == 1) {
        check_return(200, 200, true, false);
    }
    lv_display_delete(display);
    lv_deinit();
}
