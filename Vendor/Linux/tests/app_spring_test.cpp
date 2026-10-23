/** @file app_spring_test.cpp @brief 弹簧驱动、SystemDash 焦点及对象/observer 生命周期回归。 */
#include <array>
#include <cassert>
#include "UI/Common/PageStyle.h"
#include "UI/Common/PageUtils.h"
#include "UI/SystemDash/SystemDashView.h"
#include "UI/Dialplate/DialplateView.h"
#include "UI/WorkSettings/WorkSettingsView.h"
#include "UI/RecordConfig/RecordConfigView.h"
#include "lv_spring.h"

namespace {
uint16_t pixels[294 * 20];

void
advance(unsigned ms) {
    for (unsigned elapsed = 0; elapsed < ms; elapsed += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
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
complete_card(const page::SystemDashView& view, lv_obj_t* target) {
    assert(lv_obj_get_style_bg_opa(target, LV_PART_MAIN) == LV_OPA_COVER);
    assert(lv_obj_get_style_border_width(target, LV_PART_MAIN) == 1);
    assert(lv_color_to_u32(lv_obj_get_style_border_color(target, LV_PART_MAIN))
           == lv_color_to_u32(page::style::kBorder));
    const auto control = view.control(target);
    const bool action =
        control == page::SystemDashView::Control::Shutdown || control == page::SystemDashView::Control::Reboot;
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(target, LV_PART_MAIN))
           == lv_color_to_u32(action ? page::style::kDanger : page::style::kFocus));
    auto* highlight = view.focus_highlight();
    assert(highlight && !lv_obj_is_hidden(highlight));
    assert(!lv_obj_is_clickable(highlight) && lv_obj_is_ignore_layout(highlight) && lv_obj_is_floating(highlight));
    assert(lv_obj_get_style_bg_opa(highlight, LV_PART_MAIN) == LV_OPA_TRANSP);
    assert(lv_obj_get_index(highlight)
           == static_cast<int32_t>(lv_obj_get_child_count(lv_obj_get_parent(highlight))) - 1);
}

void
aligned(const page::SystemDashView& view, lv_obj_t* target) {
    complete_card(view, target);
    auto* highlight = view.focus_highlight();
    assert(highlight && !lv_obj_is_hidden(highlight));
    lv_obj_update_layout(target);
    lv_area_t actual, expected;
    lv_obj_get_coords(highlight, &actual);
    lv_obj_get_coords(target, &expected);
    assert(actual.x1 == expected.x1 && actual.y1 == expected.y1 && actual.x2 == expected.x2
           && actual.y2 == expected.y2);
    assert(!view.focus_motion().running());
    for (auto axis : {app::LvSpring::X, app::LvSpring::Y, app::LvSpring::Width, app::LvSpring::Height}) {
        const auto& value = view.focus_motion().value(axis);
        assert(value.settled() && value.position() == value.target() && value.velocity() == 0);
    }
}

void
path() {
    lv_anim_t animation;
    lv_anim_init(&animation);
    animation.duration = 250;
    for (const auto ends : {std::array<int32_t, 2>{-20, 100}, std::array<int32_t, 2>{100, -20}}) {
        animation.start_value = ends[0];
        animation.end_value = ends[1];
        animation.act_time = -10;
        assert(app::lv_anim_path_spring(&animation) == ends[0]);
        animation.act_time = 0;
        assert(app::lv_anim_path_spring(&animation) == ends[0]);
        animation.act_time = 60;
        const auto middle = app::lv_anim_path_spring(&animation);
        assert(middle > -20 && middle < 100);
        for (int elapsed : {250, 500}) {
            animation.act_time = elapsed;
            assert(app::lv_anim_path_spring(&animation) == ends[1]);
        }
    }
    animation.duration = 0;
    assert(app::lv_anim_path_spring(&animation) == animation.end_value);
}

void
driver_lifetime() {
    const auto baseline = timers();
    auto* object = page::style::box(lv_screen_active(), 0, 0, 30, 20);
    {
        app::LvSpring driver;
        assert(driver.bind(object));
        assert(!driver.running() && timers() == baseline);
        assert(driver.retarget(100, 50, 80, 40));
        assert(driver.running() && timers() == baseline + 1);
        advance(60);
        const auto x = driver.value(app::LvSpring::X).position();
        const auto velocity = driver.value(app::LvSpring::X).velocity();
        assert(x > 0 && x < 100 && velocity > 0);
        assert(driver.retarget(-100, -50, 20, 10));
        assert(driver.value(app::LvSpring::X).position() == x && driver.value(app::LvSpring::X).velocity() == velocity);
        assert(timers() == baseline + 1);
        /** 对象先销毁必须同步停止定时器。 */
        lv_obj_delete(object);
        assert(!driver.object() && !driver.running() && timers() == baseline);
        assert(!driver.retarget(1, 2, 3, 4));
        object = page::style::box(lv_screen_active(), 0, 0, 30, 20);
        assert(driver.bind(object));
        assert(driver.retarget(10, 20, 60, 40));
    } /** 驱动先销毁必须移除对象借用的删除回调。 */
    assert(timers() == baseline);
    advance(1000);
    lv_obj_delete(object);
    advance(1000);
    assert(timers() == baseline);
}

void
focus() {
    const auto baseline = timers();
    auto* root = page::style::box(lv_screen_active(), 0, 0, 294, 126);
    auto* subject = lv_subject_create(LV_SUBJECT_TYPE_POINTER);
    assert(subject);
    page::SystemDashState state{};
    state.wifi_available = true;
    lv_subject_set_pointer(subject, &state);
    {
        page::SystemDashView view;
        view.create(root);
        assert(view.bind(subject));
        page::utils::focus_controls(view.controls());
        view.resume_focus();
        aligned(view, lv_group_get_focused(lv_group_get_default()));
        for (unsigned index = 0; index < 4; ++index) {
            lv_group_focus_next(lv_group_get_default());
            complete_card(view, lv_group_get_focused(lv_group_get_default()));
            assert(view.focus_motion().running());
            advance(800);
            aligned(view, lv_group_get_focused(lv_group_get_default()));
        }
        for (unsigned index = 0; index < 12; ++index) {
            std::array<float, 4> positions{}, velocities{};
            for (unsigned axis = 0; axis < 4; ++axis) {
                const auto& value = view.focus_motion().value(static_cast<app::LvSpring::Axis>(axis));
                positions[axis] = value.position();
                velocities[axis] = value.velocity();
            }
            lv_group_focus_next(lv_group_get_default());
            complete_card(view, lv_group_get_focused(lv_group_get_default()));
            for (unsigned axis = 0; axis < 4; ++axis) {
                const auto& value = view.focus_motion().value(static_cast<app::LvSpring::Axis>(axis));
                assert(value.position() == positions[axis] && value.velocity() == velocities[axis]);
            }
            advance(60);
            assert(view.focus_motion().running());
        }
        advance(800);
        aligned(view, lv_group_get_focused(lv_group_get_default()));
        assert(timers() == baseline);
        lv_group_focus_next(lv_group_get_default());
        advance(60);
        /** 即使正在运动，语言/布局更新也直接落位。 */
        view.apply_language();
        aligned(view, lv_group_get_focused(lv_group_get_default()));
        auto* target = lv_group_get_focused(lv_group_get_default());
        lv_obj_set_size(target, 170, 34);
        lv_obj_update_layout(target);
        aligned(view, target);
        lv_obj_set_pressed(target, true);
        lv_obj_send_event(target, LV_EVENT_PRESSED, nullptr);
        assert(lv_obj_is_hidden(view.focus_highlight()));
        assert(lv_obj_get_style_bg_opa(target, LV_PART_MAIN) == LV_OPA_COVER);
        assert(lv_obj_get_style_border_opa(target, LV_PART_MAIN) == LV_OPA_COVER);
        lv_obj_set_pressed(target, false);
        lv_obj_send_event(target, LV_EVENT_RELEASED, nullptr);
        aligned(view, target);
        view.stop();
        page::utils::leave_focus_group();
        page::utils::focus_controls(view.controls(), target);
        view.resume_focus();
        /** 页面恢复没有从旧坐标飞入。 */
        aligned(view, target);
        view.stop();
        page::utils::leave_focus_group();
        view.show_power(true);
        page::utils::focus_controls(view.controls());
        view.resume_focus();
        aligned(view, lv_group_get_focused(lv_group_get_default()));
        lv_group_focus_next(lv_group_get_default());
        advance(800);
        aligned(view, lv_group_get_focused(lv_group_get_default()));
        lv_group_focus_next(lv_group_get_default());
        advance(60);
        assert(view.focus_motion().running());
        /** 动画中卸载，observer 及事件回调都必须解除。 */
        view.destroy();
        assert(!view.focus_motion().object() && !view.focus_motion().running());
        assert(timers() == baseline);
        state.wifi_on = true;
        lv_subject_notify(subject);
        lv_obj_send_event(target, LV_EVENT_FOCUSED, nullptr);
        assert(!view.focus_motion().object());
        page::utils::leave_focus_group();
    }
    lv_subject_delete(subject);
    lv_obj_delete(root);
    advance(2000);
    assert(timers() == baseline);
}
/** @brief 共享组件的解绑恢复、禁用/按下状态、换父容器与滚动坐标回归。 */
void
component() {
    const auto baseline = timers();
    auto* root = page::style::box(lv_screen_active(), 0, 0, 294, 126);
    auto* scroller = page::style::box(root, 0, 0, 200, 110);
    lv_obj_set_scrollable(scroller, true);
    auto* a = page::style::box(scroller, 10, 10, 80, 30, true);
    auto* b = page::style::box(scroller, 10, 150, 80, 30, true);
    auto* c = page::style::box(root, 210, 10, 80, 30, true);
    for (auto* object : {a, b, c}) {
        page::style::control(object);
    }
    lv_obj_update_layout(root);
    const auto root_events = lv_obj_get_event_count(root);
    const auto scroll_events = lv_obj_get_event_count(scroller);
    const auto a_events = lv_obj_get_event_count(a);
    {
        page::FocusHighlight focus;
        assert(!focus.bind(nullptr, std::array{a}));
        assert(focus.bind(root, std::array{a, b, c}));
        page::utils::focus_controls(std::array{a, b, c}, a);
        focus.resume();
        assert(focus.object() && !focus.motion().running());
        lv_obj_set_pressed(a, true);
        assert(lv_obj_is_hidden(focus.object()));
        assert(lv_obj_get_style_border_width(a, LV_PART_MAIN) == 2);
        assert(lv_color_to_u32(lv_obj_get_style_border_color(a, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kAccent));
        lv_obj_set_pressed(a, false);
        assert(!lv_obj_is_hidden(focus.object()));
        lv_obj_set_disabled(a, true);
        assert(lv_obj_is_hidden(focus.object()));
        lv_obj_set_disabled(a, false);
        assert(!lv_obj_is_hidden(focus.object()));
        lv_group_focus_obj(b);
        advance(60);
        lv_obj_scroll_to_y(scroller, 100, LV_ANIM_OFF);
        lv_obj_update_layout(root);
        lv_area_t actual, expected;
        lv_obj_get_coords(focus.object(), &actual);
        lv_obj_get_coords(b, &expected);
        assert(actual.x1 == expected.x1 && actual.y1 == expected.y1 && actual.x2 == expected.x2
               && actual.y2 == expected.y2);
        assert(!focus.motion().running());
        lv_group_focus_obj(c);
        assert(lv_obj_get_parent(focus.object()) == root && !focus.motion().running());
        lv_group_focus_obj(a);
        focus.resume();
        lv_group_focus_obj(b);
        advance(20);
        focus.stop();
        assert(!focus.motion().running());
        page::utils::leave_focus_group();
        lv_group_add_obj(lv_group_get_default(), c);
        focus.resume();
        assert(!focus.motion().running());
        focus.unbind();
        assert(!focus.object() && !focus.motion().running());
        assert(lv_obj_get_event_count(a) == a_events);
        assert(lv_obj_get_event_count(root) == root_events);
        assert(lv_obj_get_event_count(scroller) == scroll_events);
        assert(lv_obj_get_style_border_width(c, LV_PART_MAIN) == 2);
        assert(lv_color_to_u32(lv_obj_get_style_border_color(c, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kAccent));
        page::utils::leave_focus_group();
        lv_obj_send_event(a, LV_EVENT_FOCUSED, nullptr);
        assert(!focus.object());
        assert(focus.bind(root, std::array{a, b, c}));
        page::utils::focus_controls(std::array{a, b, c});
        focus.resume();
        lv_group_focus_obj(b);
        advance(20);
        /** @brief 控件及父容器先销毁，组件不保留回调、对象地址或定时器。 */
        lv_obj_delete(root);
        assert(!focus.object() && !focus.motion().running());
        focus.unbind();
    }
    advance(1000);
    assert(timers() == baseline);
}

/** @brief 每个接入页面的终态、60 ms 连切保速、离页停表与恢复回归。 */
template <typename View, std::size_t N>
void
rollout(View& view, const std::array<lv_obj_t*, N>& controls) {
    const auto baseline = timers();
    page::utils::focus_controls(controls);
    view.resume_focus();
    const auto& focus = view.focus_highlight();
    const auto check = [&] {
        lv_obj_update_layout(focus.object());
        lv_area_t actual, expected;
        lv_obj_get_coords(focus.object(), &actual);
        lv_obj_get_coords(lv_group_get_focused(lv_group_get_default()), &expected);
        assert(actual.x1 == expected.x1 && actual.y1 == expected.y1 && actual.x2 == expected.x2
               && actual.y2 == expected.y2);
        assert(!focus.motion().running());
    };
    advance(800);
    check();
    for (unsigned index = 0; index < controls.size() * 3; ++index) {
        const auto* parent = lv_obj_get_parent(lv_group_get_focused(lv_group_get_default()));
        std::array<float, 4> positions{}, velocities{};
        for (unsigned axis = 0; axis < 4; ++axis) {
            const auto& value = focus.motion().value(static_cast<app::LvSpring::Axis>(axis));
            positions[axis] = value.position();
            velocities[axis] = value.velocity();
        }
        lv_group_focus_next(lv_group_get_default());
        if (parent == lv_obj_get_parent(lv_group_get_focused(lv_group_get_default()))) {
            for (unsigned axis = 0; axis < 4; ++axis) {
                const auto& value = focus.motion().value(static_cast<app::LvSpring::Axis>(axis));
                assert(value.position() == positions[axis] && value.velocity() == velocities[axis]);
            }
        } else {
            check();
        }
        advance(60);
    }
    advance(800);
    check();
    assert(timers() == baseline);
    lv_group_focus_next(lv_group_get_default());
    advance(20);
    view.destroy();
    assert(!focus.object() && !focus.motion().running());
    assert(timers() == baseline);
    page::utils::leave_focus_group();
}

void
rollout_pages() {
    auto* root = page::style::box(lv_screen_active(), 0, 0, 294, 126);
    {
        page::DialplateView view{};
        view.create(root);
        rollout(view, view.controls(true));
    }
    lv_obj_clean(root);
    {
        page::WorkSettingsView view{};
        view.create(root);
        rollout(view, view.controls());
    }
    lv_obj_clean(root);
    {
        page::RecordConfigView view{};
        view.create(root);
        rollout(view, view.controls());
    }
    lv_obj_delete(root);
}
} // namespace

int
main() {
    lv_init();
    auto* display = lv_display_create(294, 126);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,
                            [](lv_display_t* current, const lv_area_t*, uint8_t*) { lv_display_flush_ready(current); });
    auto* group = lv_group_create();
    assert(group);
    lv_group_set_default(group);
    path();
    driver_lifetime();
    focus();
    component();
    rollout_pages();
    lv_group_delete(group);
    lv_display_delete(display);
    lv_deinit();
}
