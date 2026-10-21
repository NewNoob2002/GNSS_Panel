#include "SystemInfos.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void SystemInfos::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_TOP, 280, lv_anim_path_ease_in_out);
}

void SystemInfos::on_view_load() {
    if (!model_.init())
        app_fatal("SystemInfos", "Model initialization failed");
    view_.create(root);
    style::show_demo(root, model_.settings().simulated);
    utils::attach_controls(std::array{root}, clicked, this);
}

void SystemInfos::on_view_will_appear() {
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.device(), model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.device(), model_.settings());
    });
    view_.group_init();
}

void SystemInfos::on_view_will_disappear() {
    model_.set_change_callback({});
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    utils::leave_focus_group();
}

void SystemInfos::on_view_unload() {
    model_.deinit();
    view_.destroy();
}

void SystemInfos::on_language_changed() {
    view_.apply_language();
    view_.update(model_.device(), model_.settings());
}

void SystemInfos::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemInfos*>(lv_event_get_user_data(event));
    if (!self->page_manager->is_switching())
        self->page_manager->pop();
}
