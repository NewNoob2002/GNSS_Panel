#include "SaveConfig.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void SaveConfig::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_OVER_TOP, style::kPageTransitionMs, lv_anim_path_ease_out);
}

void SaveConfig::on_view_load() {
    if (!model_.init())
        app_fatal("SaveConfig", "Model initialization failed");
    view_.create(root);
}

void SaveConfig::apply_text() {
    view_.update(model_.settings());
}

bool SaveConfig::on_back_request() {
    return model_.settings().shutting_down; // A platform shutdown cannot be left or cancelled.
}

void SaveConfig::on_view_will_appear() {
    model_.set_status_bar(true);
    model_.set_change_callback([this] {
        apply_text();
    });
    view_.appear();
    apply_text();
}

void SaveConfig::on_view_did_appear() {
    // A long host frame cannot execute power completion while navigation is still busy.
    if (!model_.settings().shutting_down)
        lv_anim_start(view_.progress_animation());
    timer_ = lv_timer_create(on_timeout, lv_anim_get_playtime(view_.progress_animation()) + 500, this);
    if (!timer_)
        LV_LOG_ERROR("SaveConfig: preview timer allocation failed");
}

void SaveConfig::cancel_timer() {
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }
}

void SaveConfig::on_view_will_disappear() {
    model_.set_change_callback({});
    view_.stop();
    cancel_timer();
    if (model_.settings().power_pending && !model_.cancel_power())
        LV_LOG_ERROR("SaveConfig: cancellation failed");
    utils::leave_focus_group();
}

void SaveConfig::on_view_unload() {
    model_.deinit();
    cancel_timer();
    view_.stop();
}

void SaveConfig::on_language_changed() {
    apply_text();
}

void SaveConfig::on_timeout(lv_timer_t* timer) {
    auto* self = static_cast<SaveConfig*>(timer->user_data);
    self->timer_ = nullptr;
    lv_timer_del(timer);
    const auto request = self->model_.settings(); // Copy before navigation unloads this Model/View.
    if (!request.power_pending)
        return; // Platform shutdown: the platform reports standby (P4App returns to Startup).
    if (!self->model_.finish_power()) {
        LV_LOG_ERROR("SaveConfig: power service failed");
        return;
    }
    // Off ends with the platform's standby report (P4App resets to Startup); Reboot resumes the working root.
    if (request.power_action == DataProc::PowerAction::Reboot && !self->page_manager->back_home())
        LV_LOG_WARN("SaveConfig: return to the working root rejected");
}
