#include "SystemDash.h"
#include "Utils/Log/Log.h"
#include "PageManager/PageManager.h"
#include "Utils/PageUtils.h"
#include "Utils/PageStyle.h"
using namespace page;

void SystemDash::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_BOTTOM);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}
void SystemDash::onViewLoad() {
    if (!model_.init())
        app_fatal("SystemDash", "Model initialization failed");
    view_.create(getRoot(), model_.settings().reboot_available);
    utils::attach_controls(view_.all_controls(), clicked, this);
    for (auto* action : {view_.ui.shutdown, view_.ui.reboot})
        lv_obj_add_event_cb(action, hold_event, LV_EVENT_ALL, this);
}
void SystemDash::onViewWillAppear() {
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.settings());
    });
    utils::focus_controls(view_.controls(), last_focus_);
    view_.appear();
}
void SystemDash::onViewWillDisappear() {
    view_.stop();
    model_.set_change_callback({});
    // Stop the hold but keep the dialog and its full bar on screen while the next page slides over this one;
    // changing this page now would flash a different picture under the transition.
    if (hold_timer_) {
        lv_timer_delete(hold_timer_);
        hold_timer_ = nullptr;
    }
    lv_anim_delete(this, set_hold_progress);
    last_focus_ = utils::leave_focus_group();
}
void SystemDash::onViewDidDisappear() {
    cancel_hold();
    if (view_.power_open()) {
        // Leaving (SaveConfig or a platform shutdown) ends the confirmation; return lands on the Power row.
        view_.show_power(false);
        last_focus_ = view_.ui.power;
    }
}
void SystemDash::onViewUnload() {
    cancel_hold();
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}
void SystemDash::onLanguageChanged() {
    view_.apply_language();
    view_.update(model_.settings());
    if (getState() != STATE::ACTIVITY)
        view_.stop();
}
void SystemDash::show_power(bool visible) {
    cancel_hold();
    lv_group_remove_all_objs(lv_group_get_default());
    view_.show_power(visible);
    utils::focus_controls(view_.controls(), visible ? view_.ui.shutdown : view_.ui.power);
}
bool SystemDash::onBackRequest() {
    if (!view_.power_open())
        return false;
    show_power(false);
    return true;
}
void SystemDash::cancel_hold() {
    if (hold_timer_) {
        lv_timer_delete(hold_timer_);
        hold_timer_ = nullptr;
    }
    if (holding_)
        lv_anim_delete(this, set_hold_progress);
    holding_ = nullptr;
    view_.set_hold(nullptr, 0);
}
void SystemDash::set_hold_progress(void* self, int32_t percent) {
    auto* page = static_cast<SystemDash*>(self);
    page->view_.set_hold(page->holding_, percent);
}
// Press on Shutdown/Reboot starts the bar; releasing before it fills (or losing the press/focus) closes the
// dialog. Only a completed hold prepares the power action.
void SystemDash::hold_event(lv_event_t* event) {
    auto* self = static_cast<SystemDash*>(lv_event_get_user_data(event));
    auto* target = lv_event_get_current_target_obj(event);
    const auto code = lv_event_get_code(event);
    if (!self->view_.power_open() || self->getManager()->isSwitching())
        return;
    if (code == LV_EVENT_PRESSED) {
        if (self->holding_)
            return;
        self->holding_ = target;
        self->hold_timer_ = lv_timer_create(on_hold_complete, kHoldMs, self);
        if (!self->hold_timer_) {
            APP_LOG_E("SystemDash", "power hold timer allocation failed");
            self->show_power(false);
            return;
        }
        lv_timer_set_repeat_count(self->hold_timer_, 1);
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, self);
        lv_anim_set_exec_cb(&anim, set_hold_progress);
        lv_anim_set_values(&anim, 0, 100);
        lv_anim_set_duration(&anim, kHoldMs);
        if (!lv_anim_start(&anim))
            APP_LOG_W("SystemDash", "power hold animation allocation failed; confirmation still times the hold");
    } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_DEFOCUSED)
               && self->holding_ == target) {
        self->show_power(false);
    }
}
void SystemDash::on_hold_complete(lv_timer_t* timer) {
    auto* self = static_cast<SystemDash*>(lv_timer_get_user_data(timer));
    self->hold_timer_ = nullptr; // A one-shot timer deletes itself after this callback.
    const auto action =
        self->holding_ == self->view_.ui.reboot ? DataProc::PowerAction::Reboot : DataProc::PowerAction::Off;
    self->view_.set_hold(self->holding_, 100);
    lv_anim_delete(self, set_hold_progress);
    if (!self->model_.prepare_power(action)) {
        APP_LOG_E("SystemDash", "power preparation rejected");
        self->show_power(false);
        return;
    }
    if (self->getManager()->push("Pages/SaveConfig") != PageManager::RES_TYPE::OK) {
        if (!self->model_.cancel_power())
            APP_LOG_E("SystemDash", "power rollback failed");
        self->show_power(false);
    }
}
void SystemDash::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemDash*>(lv_event_get_user_data(event));
    if (self->getManager()->isSwitching())
        return;
    auto* target = lv_event_get_current_target_obj(event);
    if (self->view_.power_open()) {
        self->show_power(false); // A short press never executes shutdown/reboot; it dismisses the dialog.
        return;
    }
    if (target == self->view_.ui.power)
        self->show_power(true);
    else if (target == self->view_.ui.language) {
        if (!self->model_.toggle_language())
            APP_LOG_E("SystemDash", "language request rejected");
    } else if (target == self->view_.ui.wifi) {
        const auto state = self->model_.settings();
        if (state.wifi_available && !state.wifi_pending && !self->model_.set_wifi(!state.wifi_on))
            APP_LOG_E("SystemDash", "Wi-Fi request rejected");
    } else if (target == self->view_.ui.back && self->getManager()->pop() != PageManager::RES_TYPE::OK)
        APP_LOG_E("SystemDash", "Return navigation rejected");
}
