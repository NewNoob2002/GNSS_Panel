/** @file lv_spring.cpp @brief LVGL 单定时器弹簧及固定目标曲线。 */
#include "lv_spring.h"
#include <algorithm>
#include <cmath>

namespace app {
int32_t lv_anim_path_spring(const lv_anim_t* animation) {
    if (animation->duration <= 0 || animation->act_time >= animation->duration)
        return animation->end_value;
    if (animation->act_time <= 0)
        return animation->start_value;
    /** 形状与实际动画时长解耦；解析取值不借用 animation.user_data。 */
    const float t = static_cast<float>(animation->act_time) / static_cast<float>(animation->duration) * 0.25f;
    constexpr float alpha = 20.4f;
    const float omega = std::sqrt(576.0f - alpha * alpha);
    const float progress = 1.0f - std::exp(-alpha * t) * (std::cos(omega * t) + alpha / omega * std::sin(omega * t));
    const float result = static_cast<float>(animation->start_value)
                         + (static_cast<float>(animation->end_value) - static_cast<float>(animation->start_value)) * progress;
    if (result >= 2147483648.0f)
        return INT32_MAX;
    if (result <= -2147483648.0f)
        return INT32_MIN;
    return static_cast<int32_t>(result);
}

LvSpring::LvSpring(SpringParams params)
    : values_{SpringValue(0, params), SpringValue(0, params), SpringValue(0, params), SpringValue(0, params)} {}

bool LvSpring::bind(lv_obj_t* object) {
    unbind();
    if (!object || !lv_obj_add_event_cb(object, on_delete, LV_EVENT_DELETE, this))
        return false;
    object_ = object;
    lv_obj_update_layout(object_);
    snap(lv_obj_get_x(object_), lv_obj_get_y(object_), lv_obj_get_width(object_), lv_obj_get_height(object_));
    return true;
}

void LvSpring::stop() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void LvSpring::unbind() {
    stop();
    if (object_)
        lv_obj_remove_event_cb_with_user_data(object_, on_delete, this);
    object_ = nullptr;
}

void LvSpring::snap(int32_t x, int32_t y, int32_t width, int32_t height) {
    stop();
    const int32_t geometry[]{x, y, std::max(width, int32_t{1}), std::max(height, int32_t{1})};
    for (unsigned i = 0; i < values_.size(); ++i)
        values_[i].snap(static_cast<float>(geometry[i]));
    apply();
}

bool LvSpring::settled() const {
    for (const auto& value : values_)
        if (!value.settled())
            return false;
    return true;
}

bool LvSpring::retarget(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!object_)
        return false;
    const int32_t geometry[]{x, y, std::max(width, int32_t{1}), std::max(height, int32_t{1})};
    for (unsigned i = 0; i < values_.size(); ++i)
        values_[i].retarget(static_cast<float>(geometry[i]));
    if (settled()) {
        snap(x, y, width, height);
        return true;
    }
    if (!timer_) {
        last_tick_ = lv_tick_get();
        timer_ = lv_timer_create(on_timer, 16, this);
        if (!timer_) {
            snap(x, y, width, height);
            return false;
        }
    }
    return true;
}

void LvSpring::apply() {
    if (!object_)
        return;
    const auto coordinate = [&](Axis axis) { return static_cast<int32_t>(std::round(values_[axis].position())); };
    lv_obj_set_pos(object_, coordinate(X), coordinate(Y));
    lv_obj_set_size(object_, std::max(coordinate(Width), int32_t{1}), std::max(coordinate(Height), int32_t{1}));
}

void LvSpring::on_timer(lv_timer_t* timer) {
    auto* self = static_cast<LvSpring*>(lv_timer_get_user_data(timer));
    const auto now = lv_tick_get();
    const float elapsed = static_cast<float>(now - self->last_tick_);
    self->last_tick_ = now;
    for (auto& value : self->values_)
        value.step(elapsed);
    self->apply();
    if (self->settled())
        self->stop();
}

void LvSpring::on_delete(lv_event_t* event) {
    auto* self = static_cast<LvSpring*>(lv_event_get_user_data(event));
    self->stop();
    self->object_ = nullptr;
}
} // namespace app
