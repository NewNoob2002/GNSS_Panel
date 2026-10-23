/** @file FocusHighlight.cpp @brief 与控件同父容器的上层边框，连续切换保留弹簧速度。 */
#include "FocusHighlight.h"
#include "PageStyle.h"
#include "Log.h"

namespace page {
bool FocusHighlight::bind_controls(lv_obj_t* root, lv_obj_t* const* controls, std::size_t count,
                                   bool shared_layer) {
    unbind();
    if (!root)
        return false;
    host_ = shared_layer ? root : nullptr;
    auto* highlight = style::box(root, 0, 0, 1, 1);
    style::card(highlight);
    lv_obj_set_style_border_width(highlight, 2, 0);
    lv_obj_set_style_border_color(highlight, style::kAccent, 0);
    lv_obj_set_style_bg_opa(highlight, LV_OPA_TRANSP, 0);
    lv_obj_set_ignore_layout(highlight, true);
    lv_obj_set_floating(highlight, true);
    lv_obj_set_hidden(highlight, true);
    if (!motion_.bind(highlight)) {
        lv_obj_delete(highlight);
        return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        auto* object = controls[i];
        if (!object)
            continue;
        if (!lv_obj_add_event_cb(object, on_control, LV_EVENT_ALL, this)) {
            unbind();
            return false;
        }
        controls_[i] = object;
        /** @brief 普通边框占 1 px，补 1 px 内距保留原焦点 2 px 占位；按下恢复原绘制。 */
        /** @brief 渐变填充的取样范围依赖边框宽度，保留 2 px 才不会改变内部像素。 */
        const bool gradient = lv_obj_get_style_bg_grad_dir(object, LV_PART_MAIN) != LV_GRAD_DIR_NONE;
        lv_obj_set_style_border_width(object, gradient ? 2 : 1, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(object, style::kBorder, LV_STATE_FOCUSED);
        lv_obj_set_style_pad_all(object, gradient ? 0 : 1, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(object, 2, LV_STATE_FOCUSED | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(object, style::kAccent, LV_STATE_FOCUSED | LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(object, 0, LV_STATE_FOCUSED | LV_STATE_PRESSED);
    }
    for (auto* object : controls_)
        if (object && lv_obj_is_focused(object)) {
            focus_to(object, false);
            break;
        }
    return true;
}

void FocusHighlight::watch_parent(lv_obj_t* parent) {
    if (parent_ == parent)
        return;
    if (parent_)
        lv_obj_remove_event_cb_with_user_data(parent_, on_parent, this);
    parent_ = parent;
    if (parent_ && !lv_obj_add_event_cb(parent_, on_parent, LV_EVENT_ALL, this))
        app_fatal("FocusHighlight", "Parent event binding failed");
}

void FocusHighlight::unbind() {
    stop();
    focused_ = nullptr;
    host_ = nullptr;
    watch_parent(nullptr);
    for (auto*& object : controls_) {
        if (!object)
            continue;
        lv_obj_remove_event_cb_with_user_data(object, on_control, this);
        lv_obj_set_style_border_width(object, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(object, style::kAccent, LV_STATE_FOCUSED);
        for (auto state : {lv_style_selector_t{LV_STATE_FOCUSED},
                           lv_style_selector_t{LV_STATE_FOCUSED | LV_STATE_PRESSED}}) {
            for (auto prop : {LV_STYLE_PAD_LEFT, LV_STYLE_PAD_RIGHT, LV_STYLE_PAD_TOP, LV_STYLE_PAD_BOTTOM})
                lv_obj_remove_local_style_prop(object, prop, state);
        }
        lv_obj_remove_local_style_prop(object, LV_STYLE_BORDER_WIDTH, LV_STATE_FOCUSED | LV_STATE_PRESSED);
        lv_obj_remove_local_style_prop(object, LV_STYLE_BORDER_COLOR, LV_STATE_FOCUSED | LV_STATE_PRESSED);
        object = nullptr;
    }
    auto* highlight = motion_.object();
    motion_.unbind();
    if (highlight)
        lv_obj_delete(highlight);
}

void FocusHighlight::visibility() {
    if (!motion_.object() || !focused_)
        return;
    lv_obj_set_style_opa(motion_.object(), lv_obj_get_style_opa(focused_, LV_PART_MAIN), 0);
    lv_obj_set_hidden(motion_.object(), lv_obj_is_pressed(focused_) || lv_obj_is_disabled(focused_)
                                          || lv_obj_is_hidden(focused_));
}

void FocusHighlight::focus_to(lv_obj_t* object, bool animated, bool layout_only) {
    auto* highlight = motion_.object();
    if (!highlight || !object || aligning_)
        return;
    aligning_ = true;
    focused_ = object;
    lv_obj_update_layout(object);
    auto* parent = lv_obj_get_parent(object);
    auto* host = host_ ? host_ : parent;
    if (lv_obj_get_parent(highlight) != host) {
        lv_obj_set_parent(highlight, host);
        animated = false;
    }
    watch_parent(parent);
    lv_obj_move_to_index(highlight, -1);
    lv_obj_set_style_radius(highlight, lv_obj_get_style_radius(object, LV_PART_MAIN), 0);
    visibility();
    /** @brief floating 覆盖框不随宿主滚动，按屏幕坐标换算到宿主内容区才能与卡片重合（含跨父容器）。 */
    lv_area_t area, origin;
    lv_obj_get_coords(object, &area);
    lv_obj_get_coords(host, &origin);
    const auto x = area.x1 - origin.x1 - lv_obj_get_style_space_left(host, LV_PART_MAIN);
    const auto y = area.y1 - origin.y1 - lv_obj_get_style_space_top(host, LV_PART_MAIN);
    const auto width = lv_obj_get_width(object), height = lv_obj_get_height(object);
    const bool unchanged = motion_.value(app::LvSpring::X).target() == x
                           && motion_.value(app::LvSpring::Y).target() == y
                           && motion_.value(app::LvSpring::Width).target() == width
                           && motion_.value(app::LvSpring::Height).target() == height;
    if (!layout_only || !unchanged) {
        if (animated)
            motion_.retarget(x, y, width, height);
        else
            motion_.snap(x, y, width, height);
    }
    aligning_ = false;
}

void FocusHighlight::on_control(lv_event_t* event) {
    auto* self = static_cast<FocusHighlight*>(lv_event_get_user_data(event));
    auto* object = lv_event_get_current_target_obj(event);
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_DELETE) {
        for (auto*& control : self->controls_)
            if (control == object)
                control = nullptr;
        if (self->focused_ == object) {
            self->focused_ = nullptr;
            self->motion_.stop();
            if (self->object())
                lv_obj_set_hidden(self->object(), true);
        }
    } else if (code == LV_EVENT_FOCUSED)
        self->focus_to(object, self->animate_);
    else if (code == LV_EVENT_DEFOCUSED && self->focused_ == object) {
        self->focused_ = nullptr;
        if (self->object())
            lv_obj_set_hidden(self->object(), true);
    } else if (self->focused_ == object) {
        if (code == LV_EVENT_SIZE_CHANGED)
            self->focus_to(object, false);
        else if (code == LV_EVENT_STYLE_CHANGED)
            self->focus_to(object, false, true);
        else if (code == LV_EVENT_STATE_CHANGED || code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED
                 || code == LV_EVENT_PRESS_LOST)
            self->visibility();
    }
}

void FocusHighlight::on_parent(lv_event_t* event) {
    auto* self = static_cast<FocusHighlight*>(lv_event_get_user_data(event));
    const auto code = lv_event_get_code(event);
    if (code == LV_EVENT_DELETE)
        self->parent_ = nullptr;
    else if (code == LV_EVENT_SCROLL || code == LV_EVENT_LAYOUT_CHANGED || code == LV_EVENT_CHILD_CHANGED)
        self->focus_to(self->focused_, false, code != LV_EVENT_SCROLL);
}
void FocusHighlight::sync() { focus_to(focused_, false); }
void FocusHighlight::resume() {
    sync();
    animate_ = true;
}
void FocusHighlight::stop() {
    animate_ = false;
    motion_.stop();
}
} // namespace page
