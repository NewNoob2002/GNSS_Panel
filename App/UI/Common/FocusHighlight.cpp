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
    if (motion_.object())
        lv_anim_delete(motion_.object(), nullptr);
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

/** @brief 回到普通覆盖框：透明填充、完整橙框。 */
void FocusHighlight::restore_border() {
    if (!motion_.object())
        return;
    lv_obj_set_style_bg_opa(motion_.object(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(motion_.object(), LV_OPA_COVER, 0);
}

/** @brief 控件在宿主内容区的覆盖框几何；按下时向内挤压（每边最多 4/3 px、不超过尺寸 1/8）。 */
void FocusHighlight::target_of(lv_obj_t* object, lv_obj_t* host, int32_t (&geometry)[4]) const {
    /** @brief floating 覆盖框不随宿主滚动，按屏幕坐标换算到宿主内容区才能与卡片重合（含跨父容器）。 */
    lv_area_t area, origin;
    lv_obj_get_coords(object, &area);
    lv_obj_get_coords(host, &origin);
    geometry[0] = area.x1 - origin.x1 - lv_obj_get_style_space_left(host, LV_PART_MAIN);
    geometry[1] = area.y1 - origin.y1 - lv_obj_get_style_space_top(host, LV_PART_MAIN);
    geometry[2] = lv_obj_get_width(object);
    geometry[3] = lv_obj_get_height(object);
    if (lv_obj_is_pressed(object)) {
        const int32_t dx = std::min<int32_t>(4, geometry[2] / 8), dy = std::min<int32_t>(3, geometry[3] / 8);
        geometry[0] += dx;
        geometry[1] += dy;
        geometry[2] -= 2 * dx;
        geometry[3] -= 2 * dy;
    }
}

/**
 * @brief 面板几何按缓入缓出曲线从当前位置插值到 to。收回用切屏时长（350 ms），减速落在入口上、内容逐渐露出；
 *        展开用 10/7 倍时长并在盖满宿主时即完成：首页各入口约在 330–365 ms 盖满，此时面板仍在中速移动，
 *        避免曲线最后约 15% 时间只挪几像素的近乎静止满屏；余下部分在首页隐藏后静默跑完。
 */
void FocusHighlight::panel_start(const int32_t (&to)[4]) {
    auto* highlight = motion_.object();
    motion_.stop();
    lv_anim_delete(highlight, nullptr);
    for (unsigned i = 0; i < 4; ++i) {
        from_[i] = static_cast<int32_t>(motion_.value(static_cast<app::LvSpring::Axis>(i)).position());
        to_[i] = to[i];
    }
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, highlight);
    lv_anim_set_user_data(&animation, this);
    lv_anim_set_values(&animation, 0, kPanelSteps);
    lv_anim_set_duration(&animation, mode_ == Mode::Opening ? style::kPageTransitionMs * 10 / 7
                                                            : style::kPageTransitionMs);
    lv_anim_set_path_cb(&animation, style::panel_transition_path);
    lv_anim_set_custom_exec_cb(&animation, on_panel);
    lv_anim_set_completed_cb(&animation, on_panel_done);
    // 收回起点已在首帧显示；不重复写入 step 0，避免使刷新器再发送一帧静止的满屏面板。
    if (mode_ == Mode::Closing)
        lv_anim_set_early_apply(&animation, false);
    if (!lv_anim_start(&animation)) {
        panel_apply(kPanelSteps);
        panel_done();
    }
}

/**
 * @brief 展开：填充不透明，橙框随进度淡出，终态不留贴边细线。
 *        收回：填充随进度淡出、橙框淡入，控件内容逐渐露出，抵达时只剩普通覆盖框。
 */
void FocusHighlight::panel_apply(int32_t step) {
    int32_t geometry[4];
    for (unsigned i = 0; i < 4; ++i)
        geometry[i] = from_[i] + (to_[i] - from_[i]) * step / kPanelSteps;
    motion_.snap(geometry[0], geometry[1], geometry[2], geometry[3]);
    const auto rising = static_cast<lv_opa_t>(LV_OPA_COVER * step / kPanelSteps);
    const auto falling = static_cast<lv_opa_t>(LV_OPA_COVER - rising);
    auto* highlight = motion_.object();
    // 盖满后的展开尾段仍保持不透明；首页隐藏期间只继续几何插值，不能误走收回的透明度。
    const bool opening = mode_ == Mode::Opening || mode_ == Mode::Open;
    lv_obj_set_style_bg_opa(highlight, opening ? static_cast<lv_opa_t>(LV_OPA_COVER) : falling, 0);
    lv_obj_set_style_border_opa(highlight, opening ? falling : rising, 0);
    auto* host = lv_obj_get_parent(highlight);
    if (opening && geometry[0] <= 0 && geometry[1] <= 0 && geometry[0] + geometry[2] >= lv_obj_get_content_width(host)
        && geometry[1] + geometry[3] >= lv_obj_get_content_height(host))
        panel_done(); // 盖满即完成；动画继续写几何，mode 已为 Open 后不再回调。
}

void FocusHighlight::panel_done() {
    if (mode_ == Mode::Opening) {
        mode_ = Mode::Open;
        const auto done = done_;
        done_ = nullptr;
        if (done)
            done(done_user_data_);
    } else if (mode_ == Mode::Closing) {
        mode_ = Mode::Focus;
        restore_border();
        visibility();
    }
}

void FocusHighlight::on_panel(lv_anim_t* animation, int32_t step) {
    static_cast<FocusHighlight*>(lv_anim_get_user_data(animation))->panel_apply(step);
}

void FocusHighlight::on_panel_done(lv_anim_t* animation) {
    static_cast<FocusHighlight*>(lv_anim_get_user_data(animation))->panel_done();
}

/** @brief 立即结束收回（离页或用户换焦点）：停在控件处，恢复普通覆盖框。 */
void FocusHighlight::finish_close() {
    if (mode_ != Mode::Closing)
        return;
    if (motion_.object())
        lv_anim_delete(motion_.object(), nullptr);
    mode_ = Mode::Focus;
    restore_border();
}

bool FocusHighlight::open(lv_color_t fill, DoneCallback done, void* user_data) {
    auto* highlight = motion_.object();
    // 收回途中可再次展开：从当前几何反向，填充恢复不透明。
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
    const int32_t to[4]{-margin, -margin, lv_obj_get_content_width(host) + 2 * margin,
                        lv_obj_get_content_height(host) + 2 * margin};
    panel_start(to);
    return true;
}

void FocusHighlight::close() {
    if (mode_ == Mode::Focus || mode_ == Mode::Closing || !motion_.object())
        return;
    freeze(false);
    done_ = nullptr;
    mode_ = Mode::Closing;
    if (!focused_) {
        finish_close();
        motion_.stop();
        lv_obj_set_hidden(motion_.object(), true);
        return;
    }
    lv_obj_update_layout(focused_);
    int32_t to[4];
    target_of(focused_, lv_obj_get_parent(motion_.object()), to);
    panel_start(to);
}

void FocusHighlight::focus_to(lv_obj_t* object, bool animated, bool layout_only) {
    auto* highlight = motion_.object();
    if (!highlight || !object || aligning_)
        return;
    if (mode_ == Mode::Opening || mode_ == Mode::Open) {
        focused_ = object; // 展开面板保持盖住宿主，只记录收回目标。
        return;
    }
    if (mode_ == Mode::Closing) {
        if (layout_only)
            return; // 首页恢复时的布局刷新不打断收回动画。
        finish_close(); // 用户换焦点/按下：结束收回，从当前几何按弹簧继续。
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
    int32_t geometry[4];
    target_of(object, host, geometry); // 按下时的挤压在任何重新对齐中都保持，松开即回弹。
    const auto x = geometry[0], y = geometry[1], width = geometry[2], height = geometry[3];
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
        finish_close();
        focus_to(focused_, false);
    } else if (mode_ == Mode::Opening && motion_.object()) {
        lv_anim_delete(motion_.object(), nullptr); // 展开中被其他导航带走：停在原处，返回时 close() 收回。
    }
}
} // namespace page
