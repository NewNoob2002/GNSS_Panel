#include "Startup.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void Startup::onInstalled() {
    setLoadAnimType(PAGE_ANIM::NONE);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void Startup::onViewLoad() {
    model_ = std::make_unique<StartupModel>();
    if (!model_->init())
        app_fatal("Startup", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_->state().simulated);
    lv_obj_add_event_cb(view_.hold_control(), hold_event, LV_EVENT_ALL, this);
}

void Startup::onViewWillAppear() {
    model_->set_status_bar(false);
    if (!view_.bind(model_->subject()))
        app_fatal("Startup", "View binding failed");
    view_.reset_hold();
    utils::focus_controls(std::array{view_.hold_control()}, view_.hold_control());
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
    if (self->getManager()->getCurrentPage() == self && self->model_)
        self->model_->set_status_bar(true);
}

void Startup::cancel_timer() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void Startup::onViewWillDisappear() {
    cancel_status_timer();
    cancel_timer();
    view_.destroy();
    utils::leave_focus_group();
}

void Startup::onViewUnload() {
    cancel_status_timer();
    cancel_timer();
    view_.destroy();
    model_.reset();
}

void Startup::onLanguageChanged() {
    view_.apply_language();
}

void Startup::hold_event(lv_event_t* event) {
    auto* self = static_cast<Startup*>(lv_event_get_user_data(event));
    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST
        && code != LV_EVENT_DEFOCUSED)
        return;
    if (code == LV_EVENT_PRESSED) {
        if (self->timer_)
            return;
        constexpr uint32_t hold_ms = 2000;
        self->timer_ = lv_timer_create(on_timeout, hold_ms, self);
        if (self->timer_)
            self->view_.start_hold_anim(hold_ms);
        else
            LV_LOG_ERROR("Startup: hold timer allocation failed");
    } else if (self->timer_ || !self->model_->state().powered_on) {
        // Released early, or the platform has not confirmed power-on yet: the hold restarts from zero.
        self->cancel_timer();
        self->view_.reset_hold_anim(300);
    }
}

void Startup::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<Startup*>(lv_timer_get_user_data(timer));
    self->timer_ = nullptr;
    lv_timer_delete(timer);
    // Only a request: the platform confirms power-on (now, or after its own key timing) and App then
    // replaces Startup with SystemLoading.
    if (!self->model_->request_power_on())
        APP_LOG_E("Startup", "power-on request rejected");
}
