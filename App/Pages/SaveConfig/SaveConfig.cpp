#include "SaveConfig.h"
#include "Utils/Log/Log.h"
#include "PageManager/PageManager.h"
#include "Utils/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void SaveConfig::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void SaveConfig::onViewLoad() {
    if (!model_.init())
        app_fatal("SaveConfig", "Model initialization failed");
    view_.create(getRoot());
}

void SaveConfig::apply_text() {
    view_.update(model_.settings());
}

bool SaveConfig::onBackRequest() {
    return model_.settings().shutting_down; // A platform shutdown cannot be left or cancelled.
}

void SaveConfig::onViewWillAppear() {
    model_.set_status_bar(true);
    model_.set_change_callback([this] {
        apply_text();
    });
    view_.appear();
    apply_text();
}

void SaveConfig::onViewDidAppear() {
    // A long host frame cannot execute power completion while navigation is still busy.
    if (!model_.settings().shutting_down)
        lv_anim_start(view_.progress_animation());
    timer_ = lv_timer_create(on_timeout, lv_anim_get_playtime(view_.progress_animation()) + 500, this);
    if (!timer_)
        LV_LOG_ERROR("SaveConfig: preview timer allocation failed");
}

void SaveConfig::cancel_timer() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

void SaveConfig::onViewWillDisappear() {
    model_.set_change_callback({});
    view_.stop();
    cancel_timer();
    if (model_.settings().power_pending && !model_.cancel_power())
        LV_LOG_ERROR("SaveConfig: cancellation failed");
    utils::leave_focus_group();
}

void SaveConfig::onViewUnload() {
    model_.deinit();
    cancel_timer();
    view_.stop();
}

void SaveConfig::onLanguageChanged() {
    apply_text();
}

void SaveConfig::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<SaveConfig*>(lv_timer_get_user_data(timer));
    self->timer_ = nullptr;
    lv_timer_delete(timer);
    const auto request = self->model_.settings(); // Copy before navigation unloads this Model/View.
    if (!request.power_pending)
        return; // Platform shutdown: the platform reports standby (App returns to Startup).
    if (!self->model_.finish_power()) {
        LV_LOG_ERROR("SaveConfig: power service failed");
        return;
    }
    // Off ends with the platform's standby report (App resets to Startup); Reboot resumes the working root.
    if (request.power_action == DataProc::PowerAction::Reboot
        && self->getManager()->backHome() != PageManager::RES_TYPE::OK)
        LV_LOG_WARN("SaveConfig: return to the working root rejected");
}
