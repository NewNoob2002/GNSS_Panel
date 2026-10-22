#include "Startup.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void Startup::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_BOTTOM);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void Startup::onViewLoad() {
    if (!model_.init())
        app_fatal("Startup", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_.settings().simulated);
    lv_obj_set_clickable(view_.ui.btn_press, true);
    lv_obj_add_event_cb(view_.ui.btn_press, hold_event, LV_EVENT_ALL, this);
    lv_obj_set_user_data(view_.ui.arc, this);
    lv_anim_set_exec_cb(&view_.ui.arc_anim, set_progress);
}

void Startup::onViewWillAppear() {
    model_.set_change_callback([this] {
        view_.update(model_.status());
        view_.update_device(model_.device());
    });
    model_.set_status_bar(false);
    view_.update(model_.status());
    view_.update_device(model_.device());
    set_progress(view_.ui.arc, 0);
    lv_obj_set_pressed(view_.ui.btn_press, false);
    utils::focus_controls(std::array{view_.ui.btn_press}, view_.ui.btn_press);
}

void Startup::onViewDidAppear() {
    status_timer_ = lv_timer_create(on_status_timeout, 10000, this);
    if (!status_timer_)
        LV_LOG_ERROR("Startup: status delay timer allocation failed");
}

void Startup::cancel_status_timer() {
    if (status_timer_) {
        lv_timer_delete(status_timer_);
        status_timer_ = nullptr;
    }
}

void Startup::on_status_timeout(lv_timer_t* timer) {
    auto* self = static_cast<Startup*>(lv_timer_get_user_data(timer));
    self->status_timer_ = nullptr;
    lv_timer_delete(timer);
    if (self->getManager()->getCurrentPage() == self)
        self->model_.set_status_bar(true);
}

void Startup::cancel_timer() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void Startup::onViewWillDisappear() {
    model_.set_change_callback({});
    cancel_status_timer();
    cancel_timer();
    lv_anim_delete(view_.ui.arc, set_progress);
    view_.destroy();
    utils::leave_focus_group();
}

void Startup::onViewUnload() {
    cancel_status_timer();
    model_.deinit();
    cancel_timer();
    lv_anim_delete(view_.ui.arc, set_progress);
    view_.destroy();
}

void Startup::onLanguageChanged() {
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
        lv_anim_delete(obj, set_progress);
        set_progress(obj, 0);
        lv_anim_set_values(animation, 0, 100);
        constexpr uint32_t hold_ms = 2000;
        lv_anim_set_duration(animation, hold_ms);
        self->timer_ = lv_timer_create(on_timeout, hold_ms, self);
        if (self->timer_)
            lv_anim_start(animation);
        else
            LV_LOG_ERROR("Startup: hold timer allocation failed");
    } else if (self->timer_ || !self->model_.settings().powered_on) {
        // Released early, or the platform has not confirmed power-on yet: the hold restarts from zero.
        self->cancel_timer();
        lv_anim_delete(obj, set_progress);
        lv_anim_set_values(animation, lv_arc_get_value(obj), 0);
        lv_anim_set_duration(animation, 300);
        lv_anim_start(animation);
    }
}

void Startup::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<Startup*>(lv_timer_get_user_data(timer));
    self->timer_ = nullptr;
    lv_timer_delete(timer);
    // Only a request: the platform confirms power-on (now, or after its own key timing) and App then
    // replaces Startup with SystemLoading.
    if (!self->model_.request_power_on())
        APP_LOG_E("Startup", "power-on request rejected");
}
