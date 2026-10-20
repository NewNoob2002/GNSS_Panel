#include "SaveConfig.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"

using namespace page;

void SaveConfig::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_TOP, 320, lv_anim_path_ease_in_out);
}

void SaveConfig::on_view_load() {
    if (!model_.init())
        app_fatal("SaveConfig", "Model initialization failed");
    view_.create(root);
}

void SaveConfig::apply_text() const {
    const auto& state = model_.settings();
    if (!state.shutting_down || state.power_cause == DataProc::PowerCause::User) {
        view_.apply_language();
        return;
    }
    const auto cause = state.power_cause == DataProc::PowerCause::Host ? i18n::TextId::SaveConfigPowerOffBoard
                       : state.power_cause == DataProc::PowerCause::LowBattery
                           ? i18n::TextId::SaveConfigPowerOffLowBattery
                           : i18n::TextId::SaveConfigPowerOffForced;
    view_.set_power_off_cause(cause);
}

bool SaveConfig::on_back_request() {
    return model_.settings().shutting_down; // A platform shutdown cannot be left or cancelled.
}

void SaveConfig::on_view_will_appear() {
    model_.set_status_bar(true);
    apply_text();
    lv_anim_timeline_start(view_.ui.anim_timeline);
}

void SaveConfig::on_view_did_appear() {
    // A long host frame cannot execute power completion while navigation is still busy.
    lv_anim_start(&view_.ui.sync.bar.anim);
    timer_ = lv_timer_create(on_timeout, lv_anim_get_playtime(&view_.ui.sync.bar.anim) + 500, this);
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
    cancel_timer();
    if (model_.settings().power_pending && !model_.cancel_power())
        LV_LOG_ERROR("SaveConfig: cancellation failed");
    utils::leave_focus_group();
}

void SaveConfig::on_view_unload() {
    model_.deinit();
    cancel_timer();
    view_.destroy();
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
