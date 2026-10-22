#include "SystemLoading.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void SystemLoading::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void SystemLoading::onViewLoad() {
    if (!model_.init())
        app_fatal("SystemLoading", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_.settings().simulated);
}

void SystemLoading::onViewWillAppear() {
    model_.set_status_bar(false);
    phase_ = Phase::Logo;
    step_ = 0;
    model_.begin_initialization(); // Report reset failures are logged; startup remains non-blocking.
    view_.show_logo();
    APP_LOG_I("SystemLoading", "window=Logo%s", demo_tag());
}

void SystemLoading::onViewDidAppear() {
    // Start completion timing after the page transition, including a stalled host frame.
    timer_ = lv_timer_create(on_timeout, 800, this); // Full-screen logo before simulated progress.
    if (!timer_)
        LV_LOG_ERROR("SystemLoading: preview timer allocation failed");
}

void SystemLoading::cancel_timer() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void SystemLoading::onViewWillDisappear() {
    cancel_timer();
    view_.destroy(); // Stop child animations even when this page remains cached.
    utils::leave_focus_group();
}

void SystemLoading::onViewUnload() {
    model_.deinit();
    cancel_timer();
    view_.destroy();
}

void SystemLoading::onLanguageChanged() {
    view_.apply_language();
}

void SystemLoading::begin_step(unsigned step) {
    step_ = step;
    step_status_ = DataProc::InitializationStatus::Pending;
    step_started_ = lv_tick_get();
    view_.begin_step(step);
    poll_initialization(nullptr); // First poll (elapsed 0) at once: instant checks need no extra tick.
}

void SystemLoading::poll_initialization(lv_timer_t* timer) {
    const uint32_t elapsed = lv_tick_elaps(step_started_);
    if (step_status_ == DataProc::InitializationStatus::Pending) {
        // Non-blocking: the backend reports Pending until its check (or its own timeout) is done.
        step_status_ = model_.poll_step(step_, timer ? elapsed : 0);
        if (step_status_ == DataProc::InitializationStatus::Pending) {
            view_.show_waiting(elapsed);
            return;
        }
        step_finished_ = lv_tick_get();
        // A failed node is presentation/log state, never a gate for the next node.
        view_.finish_step(step_, step_status_ == DataProc::InitializationStatus::Ok);
    }
    if (!timer || elapsed < kMinStepMs || lv_tick_elaps(step_finished_) < kResultMs)
        return;
    if (step_ + 1 < SystemLoadingView::kStepCount) {
        begin_step(step_ + 1);
        return;
    }
    phase_ = Phase::Completion;
    lv_timer_set_period(timer, 200); // Retain the 100% window before the completion summary.
    lv_timer_reset(timer);
}

void SystemLoading::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<SystemLoading*>(lv_timer_get_user_data(timer));
    if (self->phase_ == Phase::Logo) {
        self->phase_ = Phase::Initialization;
        self->view_.show_initialization();
        APP_LOG_I("SystemLoading", "window=Initialization%s", self->demo_tag());
        self->begin_step(0);
        lv_timer_set_period(timer, kPollMs);
        lv_timer_reset(timer);
        return;
    }
    if (self->phase_ == Phase::Initialization) {
        self->poll_initialization(timer);
        return;
    }
    if (self->phase_ == Phase::Completion) {
        self->phase_ = Phase::Ready;
        self->view_.show_ready();
        APP_LOG_I("SystemLoading", "window=Ready%s status=%s; continuing", self->demo_tag(),
                  self->view_.has_failures() ? "warnings" : "OK");
        lv_timer_set_period(timer, 1000);
        lv_timer_reset(timer);
        return;
    }
    self->timer_ = nullptr;
    lv_timer_delete(timer);
    APP_LOG_I("SystemLoading", "window=Dialplate%s", self->demo_tag());
    if (self->getManager()->replace("Pages/Dialplate") != PageManager::RES_TYPE::OK)
        LV_LOG_WARN("SystemLoading: preview transition rejected");
}
