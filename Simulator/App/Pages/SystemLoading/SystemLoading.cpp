#include "SystemLoading.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void SystemLoading::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_FADE_ON, 360, lv_anim_path_ease_in_out);
}

void SystemLoading::on_view_load() {
    if (!model_.init())
        app_fatal("SystemLoading", "Model initialization failed");
    view_.create(root);
    style::show_demo(root, model_.settings().simulated);
}

void SystemLoading::on_view_will_appear() {
    model_.set_status_bar(false);
    phase_ = Phase::Logo;
    step_ = 0;
    model_.begin_initialization(); // Report reset failures are logged; startup remains non-blocking.
    view_.show_logo();
    APP_LOG_I("SystemLoading", "window=Logo [DEMO]");
}

void SystemLoading::on_view_did_appear() {
    // Start completion timing after the page transition, including a stalled host frame.
    timer_ = lv_timer_create(on_timeout, 800, this); // Full-screen logo before simulated progress.
    if (!timer_)
        LV_LOG_ERROR("SystemLoading: preview timer allocation failed");
}

void SystemLoading::cancel_timer() {
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }
}

void SystemLoading::on_view_will_disappear() {
    cancel_timer();
    view_.destroy(); // Stop child animations even when this page remains cached.
    utils::leave_focus_group();
}

void SystemLoading::on_view_unload() {
    model_.deinit();
    cancel_timer();
    view_.destroy();
}

void SystemLoading::on_language_changed() {
    view_.apply_language();
}

void SystemLoading::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<SystemLoading*>(timer->user_data);
    if (self->phase_ == Phase::Logo) {
        self->phase_ = Phase::Initialization;
        self->view_.show_initialization();
        APP_LOG_I("SystemLoading", "window=Initialization [DEMO]");
        self->view_.show_step(self->step_, self->model_.initialize_step(self->step_));
        lv_timer_set_period(timer, SystemLoadingView::kStepMs);
        lv_timer_reset(timer);
        return;
    }
    if (self->phase_ == Phase::Initialization) {
        if (++self->step_ < SystemLoadingView::kStepCount) {
            // A failed node is presentation/log state, never a gate for the next node.
            self->view_.show_step(self->step_, self->model_.initialize_step(self->step_));
        } else {
            self->phase_ = Phase::Completion;
            lv_timer_set_period(timer, 200); // Retain the 100% window before the completion summary.
        }
        lv_timer_reset(timer);
        return;
    }
    if (self->phase_ == Phase::Completion) {
        self->phase_ = Phase::Ready;
        self->view_.show_ready();
        APP_LOG_I("SystemLoading", "window=Ready [DEMO] status=%s; continuing",
                  self->view_.has_failures() ? "warnings" : "OK");
        lv_timer_set_period(timer, 1000);
        lv_timer_reset(timer);
        return;
    }
    self->timer_ = nullptr;
    lv_timer_del(timer);
    APP_LOG_I("SystemLoading", "window=Dialplate [DEMO]");
    if (!self->page_manager->replace("Pages/Dialplate"))
        LV_LOG_WARN("SystemLoading: preview transition rejected");
}
