#include "SystemSettings.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
using namespace page;
void SystemSettings::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_LEFT, 300, lv_anim_path_ease_in_out);
}
void SystemSettings::on_view_load() {
    if (!model_.init())
        app_fatal("SystemSettings", "Model initialization failed");
    view_.create(root);
    utils::attach_controls(std::array{view_.ui.language, view_.ui.wifi, view_.ui.back}, clicked, this);
}
void SystemSettings::on_view_will_appear() {
    // SystemDash owns the full-screen flow; Settings must not reveal the shared StatusBar.
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.settings());
    });
    utils::focus_controls(view_.controls(), last_focus_);
}
void SystemSettings::on_view_will_disappear() {
    model_.set_change_callback({});
    last_focus_ = utils::leave_focus_group();
}
void SystemSettings::on_view_unload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}
void SystemSettings::on_language_changed() {
    view_.apply_language();
}
void SystemSettings::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemSettings*>(lv_event_get_user_data(event));
    if (self->page_manager->is_switching())
        return;
    auto* target = lv_event_get_current_target(event);
    if (target == self->view_.ui.language) {
        if (!self->model_.toggle_language())
            APP_LOG_E("SystemSettings", "language request rejected");
        self->view_.update(self->model_.settings());
    } else if (target == self->view_.ui.wifi) {
        const auto& state = self->model_.settings();
        if (state.wifi_available && !state.wifi_pending && !self->model_.set_wifi(!state.wifi_on))
            APP_LOG_E("SystemSettings", "Wi-Fi request rejected");
        self->view_.update(self->model_.settings());
    } else if (target == self->view_.ui.back && !self->page_manager->pop()) {
        APP_LOG_E("SystemSettings", "Back navigation rejected");
    }
}
