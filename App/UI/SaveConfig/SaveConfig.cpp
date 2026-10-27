#include "SaveConfig.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void SaveConfig::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void SaveConfig::onViewLoad() {
    model_ = std::make_unique<SaveConfigModel>();
    if (!model_->init())
        app_fatal("SaveConfig", "Model initialization failed");
    view_.create(getRoot());
}

bool SaveConfig::onBackRequest() {
    return model_->state().shutting_down;
}

void SaveConfig::onViewWillAppear() {
    model_->set_status_bar(true);
    if (!view_.bind(model_->subject()))
        app_fatal("SaveConfig", "View binding failed");
    view_.appear();
}

void SaveConfig::onViewDidAppear() {
    // A long host frame cannot execute power completion while navigation is still busy.
    if (!model_->state().shutting_down)
        lv_anim_start(view_.progress_animation());
    timer_ = lv_timer_create(on_timeout, lv_anim_get_playtime(view_.progress_animation()) + 500, this);
    if (!timer_)
        APP_LOG_E("SaveConfig", "preview timer allocation failed");
}

void SaveConfig::cancel_timer() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void SaveConfig::onViewWillDisappear() {
    view_.stop();
    view_.unbind();
    cancel_timer();
    if (model_->state().power_pending && !model_->cancel_power())
        APP_LOG_E("SaveConfig", "cancellation failed");
    utils::leave_focus_group();
}

void SaveConfig::onViewUnload() {
    cancel_timer();
    view_.destroy();
    model_.reset();
}

void SaveConfig::onLanguageChanged() {
    view_.update(model_->state());
}

void SaveConfig::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<SaveConfig*>(lv_timer_get_user_data(timer));
    self->timer_ = nullptr;
    lv_timer_delete(timer);
    const auto request = self->model_->state(); // Copy before navigation unloads this Model/View.
    if (!request.power_pending)
        return; // Platform shutdown: the platform reports standby (App returns to Startup).
    if (!self->model_->finish_power()) {
        APP_LOG_E("SaveConfig", "power service failed");
        return;
    }
    // Off ends with the platform's standby report (App resets to Startup); Reboot resumes the working root.
    if (request.power_action == hal::PowerCommand::Reboot
        && self->getManager()->backHome() != PageManager::RES_TYPE::OK)
        APP_LOG_W("SaveConfig", "return to the working root rejected");
}
