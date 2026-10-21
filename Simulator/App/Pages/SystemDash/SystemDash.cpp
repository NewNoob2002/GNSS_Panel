#include "SystemDash.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
using namespace page;

void SystemDash::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_OVER_BOTTOM, 280, lv_anim_path_ease_out);
}
void SystemDash::on_view_load() {
    if (!model_.init())
        app_fatal("SystemDash", "Model initialization failed");
    view_.create(root, model_.settings().reboot_available);
    utils::attach_controls(view_.all_controls(), clicked, this);
}
void SystemDash::on_view_will_appear() {
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.settings());
    });
    utils::focus_controls(view_.controls(), view_.power_open() ? view_.ui.cancel : last_focus_);
    view_.appear();
}
void SystemDash::on_view_will_disappear() {
    view_.stop();
    model_.set_change_callback({});
    last_focus_ = utils::leave_focus_group();
}
void SystemDash::on_view_unload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}
void SystemDash::on_language_changed() {
    view_.apply_language();
    view_.update(model_.settings());
    if (priv.state != PAGE_STATE_ACTIVITY)
        view_.stop();
}
void SystemDash::show_power(bool visible) {
    lv_group_remove_all_objs(lv_group_get_default());
    view_.show_power(visible);
    utils::focus_controls(view_.controls(), visible ? view_.ui.cancel : view_.ui.power);
}
bool SystemDash::on_back_request() {
    if (!view_.power_open())
        return false;
    show_power(false);
    return true;
}
void SystemDash::on_commit_request() {
    if (!view_.power_open())
        return;
    auto* focused = lv_group_get_focused(lv_group_get_default());
    if (focused == view_.ui.cancel) {
        show_power(false);
        return;
    }
    if (focused != view_.ui.shutdown && focused != view_.ui.reboot)
        return;
    const auto action = focused == view_.ui.shutdown ? DataProc::PowerAction::Off : DataProc::PowerAction::Reboot;
    if (!model_.prepare_power(action)) {
        APP_LOG_E("SystemDash", "power preparation rejected");
        return;
    }
    if (!page_manager->push("Pages/SaveConfig")) {
        if (!model_.cancel_power())
            APP_LOG_E("SystemDash", "power rollback failed");
    }
}
void SystemDash::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemDash*>(lv_event_get_user_data(event));
    if (self->page_manager->is_switching())
        return;
    auto* target = lv_event_get_current_target(event);
    if (self->view_.power_open()) {
        if (target == self->view_.ui.cancel)
            self->show_power(false);
        return; // A single Select can NEVER execute shutdown/reboot.
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
    } else if (target == self->view_.ui.back && !self->page_manager->pop())
        APP_LOG_E("SystemDash", "Return navigation rejected");
}
