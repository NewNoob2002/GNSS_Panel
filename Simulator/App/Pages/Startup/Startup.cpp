#include "Startup.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void Startup::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_OVER_BOTTOM, style::kPageTransitionMs, lv_anim_path_ease_out);
}

void Startup::on_view_load() {
    if (!model_.init())
        app_fatal("Startup", "Model initialization failed");
    view_.create(root);
    style::show_demo(root, model_.settings().simulated);
    lv_obj_add_flag(view_.ui.btn_press, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(view_.ui.btn_press, hold_event, LV_EVENT_ALL, this);
    lv_obj_set_user_data(view_.ui.arc, this);
    lv_anim_set_exec_cb(&view_.ui.arc_anim, set_progress);
}

void Startup::on_view_will_appear() {
    model_.set_change_callback([this] {
        view_.update(model_.status());
        view_.update_device(model_.device());
    });
    model_.set_status_bar(false);
    view_.update(model_.status());
    view_.update_device(model_.device());
    set_progress(view_.ui.arc, 0);
    lv_obj_clear_state(view_.ui.btn_press, LV_STATE_PRESSED);
    utils::focus_controls(std::array{view_.ui.btn_press}, view_.ui.btn_press);
}

void Startup::on_view_did_appear() {
    status_timer_ = lv_timer_create(on_status_timeout, 10000, this);
    if (!status_timer_)
        LV_LOG_ERROR("Startup: status delay timer allocation failed");
}

void Startup::cancel_status_timer() {
    if (status_timer_) {
        lv_timer_del(status_timer_);
        status_timer_ = nullptr;
    }
}

void Startup::on_status_timeout(lv_timer_t* timer) {
    auto* self = static_cast<Startup*>(timer->user_data);
    self->status_timer_ = nullptr;
    lv_timer_del(timer);
    if (self->page_manager->get_current_page() == self)
        self->model_.set_status_bar(true);
}

void Startup::cancel_timer() {
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }
}

void Startup::on_view_will_disappear() {
    model_.set_change_callback({});
    cancel_status_timer();
    cancel_timer();
    lv_anim_del(view_.ui.arc, set_progress);
    view_.destroy();
    utils::leave_focus_group();
}

void Startup::on_view_unload() {
    cancel_status_timer();
    model_.deinit();
    cancel_timer();
    lv_anim_del(view_.ui.arc, set_progress);
    view_.destroy();
}

void Startup::on_language_changed() {
    view_.apply_language();
}

void Startup::set_progress(void* obj, int32_t value) {
    auto* self = static_cast<Startup*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    lv_arc_set_value(static_cast<lv_obj_t*>(obj), static_cast<int16_t>(value));
    lv_label_set_text_fmt(self->view_.ui.arc_percent, "%ld%%", static_cast<long>(value));
}

void Startup::hold_event(lv_event_t* event) {
    auto* self = static_cast<Startup*>(lv_event_get_user_data(event));
    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST
        && code != LV_EVENT_DEFOCUSED)
        return;
    auto* obj = self->view_.ui.arc;
    auto* animation = &self->view_.ui.arc_anim;
    if (code == LV_EVENT_PRESSED) {
        if (self->timer_)
            return;
        lv_anim_del(obj, set_progress);
        set_progress(obj, 0);
        lv_anim_set_values(animation, 0, 100);
        constexpr uint32_t hold_ms = 2000;
        lv_anim_set_time(animation, hold_ms);
        self->timer_ = lv_timer_create(on_timeout, hold_ms, self);
        if (self->timer_)
            lv_anim_start(animation);
        else
            LV_LOG_ERROR("Startup: hold timer allocation failed");
    } else if (self->timer_ || !self->model_.settings().powered_on) {
        // Released early, or the platform has not confirmed power-on yet: the hold restarts from zero.
        self->cancel_timer();
        lv_anim_del(obj, set_progress);
        lv_anim_set_values(animation, lv_arc_get_value(obj), 0);
        lv_anim_set_time(animation, 300);
        lv_anim_start(animation);
    }
}

void Startup::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<Startup*>(timer->user_data);
    self->timer_ = nullptr;
    lv_timer_del(timer);
    // Only a request: the platform confirms power-on (now, or after its own key timing) and P4App then
    // replaces Startup with SystemLoading.
    if (!self->model_.request_power_on())
        APP_LOG_E("Startup", "power-on request rejected");
}
