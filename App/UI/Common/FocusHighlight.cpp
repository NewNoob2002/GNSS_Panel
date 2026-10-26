/** @file FocusHighlight.cpp @brief 与控件同父容器的上层边框，连续切换保留弹簧速度。 */
#include "FocusHighlight.h"
#include <algorithm>
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
    motion_.set_settled_callback(on_settled, this);
    motion_.set_frame_callback(on_frame);
    for (std::size_t i = 0; i < count; ++i) {
        auto* object = controls[i];
        if (!object)
            continue;
        if (!lv_obj_add_event_cb(object, on_control, LV_EVENT_ALL, this)) {
            unbind();
            return false;
        }
        controls_[i] = object;
        /** @brief 普通边框占 1 px，补 1 px 内距保留原焦点 2 px 占位；按下时覆盖框向内挤压，控件不再自绘橙框。 */
        /** @brief 渐变填充的取样范围依赖边框宽度，保留 2 px 才不会改变内部像素。 */
        const bool gradient = lv_obj_get_style_bg_grad_dir(object, LV_PART_MAIN) != LV_GRAD_DIR_NONE;
        lv_obj_set_style_border_width(object, gradient ? 2 : 1, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(object, style::kBorder, LV_STATE_FOCUSED);
        lv_obj_set_style_pad_all(object, gradient ? 0 : 1, LV_STATE_FOCUSED);
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
    mode_ = Mode::Focus;
    done_ = nullptr;
    focused_ = nullptr;
    host_ = nullptr;
    watch_parent(nullptr);
    for (auto*& object : controls_) {
        if (!object)
            continue;
        lv_obj_remove_event_cb_with_user_data(object, on_control, this);
        lv_obj_set_style_border_width(object, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(object, style::kAccent, LV_STATE_FOCUSED);
        for (auto prop : {LV_STYLE_PAD_LEFT, LV_STYLE_PAD_RIGHT, LV_STYLE_PAD_TOP, LV_STYLE_PAD_BOTTOM})
            lv_obj_remove_local_style_prop(object, prop, LV_STATE_FOCUSED);
        object = nullptr;
    }
    auto* highlight = motion_.object();
    motion_.unbind();
    if (highlight)
        lv_obj_delete(highlight);
}

void FocusHighlight::visibility() {
    if (!motion_.object() || !focused_ || mode_ != Mode::Focus)
        return; // 展开/收回中的面板始终可见。
    lv_obj_set_style_opa(motion_.object(), lv_obj_get_style_opa(focused_, LV_PART_MAIN), 0);
    lv_obj_set_hidden(motion_.object(), lv_obj_is_disabled(focused_) || lv_obj_is_hidden(focused_));
}

void FocusHighlight::freeze(bool frozen) {
    auto* group = lv_group_get_default();
    if (frozen_ == frozen || !group)
        return;
    lv_group_focus_freeze(group, frozen);
    frozen_ = frozen;
}

void FocusHighlight::restore_border() {
    if (motion_.object())
        lv_obj_set_style_bg_opa(motion_.object(), LV_OPA_TRANSP, 0);
}

bool FocusHighlight::open(lv_color_t fill, DoneCallback done, void* user_data) {
    auto* highlight = motion_.object();
    // 收回途中可再次展开：从当前几何保速反向，填充恢复不透明。
    if (!highlight || !focused_ || (mode_ != Mode::Focus && mode_ != Mode::Closing))
        return false;
    mode_ = Mode::Opening;
    done_ = done;
    done_user_data_ = user_data;
    freeze(true); // 展开到导航完成前不响应换焦点。
    lv_obj_set_style_bg_color(highlight, fill, 0);
    lv_obj_set_style_bg_opa(highlight, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(highlight, LV_OPA_COVER, 0);
    lv_obj_set_hidden(highlight, false);
    lv_obj_move_to_index(highlight, -1);
    /** @brief 越出宿主边缘至少一个圆角加边框，展开终态只剩 fill 色，可与目标页背景无缝衔接。 */
    auto* host = lv_obj_get_parent(highlight);
    const int32_t margin = lv_obj_get_style_radius(highlight, LV_PART_MAIN)
                           + lv_obj_get_style_border_width(highlight, LV_PART_MAIN) + 2;
    const int32_t width = lv_obj_get_content_width(host) + 2 * margin;
    const int32_t height = lv_obj_get_content_height(host) + 2 * margin;
    open_area_ = static_cast<float>(width) * static_cast<float>(height);
    motion_.retarget(-margin, -margin, width, height);
    return true;
}

void FocusHighlight::close() {
    if (mode_ == Mode::Focus || mode_ == Mode::Closing || !motion_.object())
        return;
    freeze(false);
    done_ = nullptr;
    mode_ = Mode::Closing;
    if (!focused_) {
        mode_ = Mode::Focus;
        motion_.stop();
        restore_border();
        lv_obj_set_hidden(motion_.object(), true);
        return;
    }
    focus_to(focused_, true); // 保速收回到焦点控件，静止后 on_settled 恢复透明。
}

/** @brief 展开完成：只通知一次，弹簧可继续在面板后方静止。 */
void FocusHighlight::opened_now() {
    mode_ = Mode::Open;
    const auto done = done_;
    done_ = nullptr;
    if (done)
        done(done_user_data_);
}

/**
 * @brief 展开：面板四边都越过宿主边缘即视为完成，不等弹簧收敛尾段（误差 0.1 px 级、肉眼不可见）。
 *        收回：填充不透明度随面积从展开尺寸降到控件尺寸线性减为 0，控件内容逐渐露出，抵达时已透明。
 */
void FocusHighlight::on_frame(void* user_data) {
    auto* self = static_cast<FocusHighlight*>(user_data);
    auto* highlight = self->motion_.object();
    if (!highlight)
        return;
    const auto& spring = self->motion_;
    if (self->mode_ == Mode::Closing) {
        const float area = spring.value(app::LvSpring::Width).position() * spring.value(app::LvSpring::Height).position();
        const float target = spring.value(app::LvSpring::Width).target() * spring.value(app::LvSpring::Height).target();
        const float span = self->open_area_ - target;
        const float ratio = span > 0 ? std::clamp((area - target) / span, 0.0f, 1.0f) : 0.0f;
        lv_obj_set_style_bg_opa(highlight, static_cast<lv_opa_t>(ratio * LV_OPA_COVER), 0);
        return;
    }
    if (self->mode_ != Mode::Opening)
        return;
    auto* host = lv_obj_get_parent(highlight);
    // 读弹簧刚写入的几何：对象坐标要到下一次布局刷新才更新。
    const auto& m = self->motion_;
    const float x = m.value(app::LvSpring::X).position(), y = m.value(app::LvSpring::Y).position();
    if (x <= 0 && y <= 0 && x + m.value(app::LvSpring::Width).position() >= lv_obj_get_content_width(host)
        && y + m.value(app::LvSpring::Height).position() >= lv_obj_get_content_height(host))
        self->opened_now();
}

void FocusHighlight::on_settled(void* user_data) {
    auto* self = static_cast<FocusHighlight*>(user_data);
    if (self->mode_ == Mode::Opening) {
        self->opened_now();
    } else if (self->mode_ == Mode::Closing) {
        self->mode_ = Mode::Focus;
        self->restore_border();
        self->visibility();
    }
}

void FocusHighlight::focus_to(lv_obj_t* object, bool animated, bool layout_only) {
    auto* highlight = motion_.object();
    if (!highlight || !object || aligning_)
        return;
    if (mode_ == Mode::Opening || mode_ == Mode::Open) {
        focused_ = object; // 展开面板保持盖住宿主，只记录收回目标。
        return;
    }
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
    auto x = area.x1 - origin.x1 - lv_obj_get_style_space_left(host, LV_PART_MAIN);
    auto y = area.y1 - origin.y1 - lv_obj_get_style_space_top(host, LV_PART_MAIN);
    auto width = lv_obj_get_width(object), height = lv_obj_get_height(object);
    /** @brief 按下时向内挤压（每边最多 4/3 px、不超过尺寸 1/8），任何重新对齐都保持挤压，松开即回弹。 */
    if (lv_obj_is_pressed(object)) {
        const int32_t dx = std::min<int32_t>(4, width / 8), dy = std::min<int32_t>(3, height / 8);
        x += dx;
        y += dy;
        width -= 2 * dx;
        height -= 2 * dy;
    }
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
        if (self->object() && self->mode_ == Mode::Focus) // 离页清空焦点组时保留展开面板。
            lv_obj_set_hidden(self->object(), true);
    } else if (self->focused_ == object) {
        if (code == LV_EVENT_SIZE_CHANGED)
            self->focus_to(object, false);
        else if (code == LV_EVENT_STYLE_CHANGED)
            self->focus_to(object, false, true);
        else if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
            self->visibility();
            self->focus_to(object, self->animate_);
        } else if (code == LV_EVENT_STATE_CHANGED)
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
void FocusHighlight::sync() {
    if (mode_ == Mode::Focus)
        focus_to(focused_, false);
}
void FocusHighlight::resume() {
    sync();
    animate_ = true;
}
void FocusHighlight::stop() {
    animate_ = false;
    motion_.stop();
    freeze(false); // 焦点组由所有页面共用，离页必须解冻。
    if (mode_ == Mode::Closing) { // 收回中离页：直接落位，下次进入不再遗留不透明面板。
        mode_ = Mode::Focus;
        restore_border();
        focus_to(focused_, false);
    }
}
} // namespace page
