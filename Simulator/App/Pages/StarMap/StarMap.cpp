#include "StarMap.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void StarMap::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_TOP, style::kPageTransitionMs, lv_anim_path_ease_out);
}

void StarMap::on_view_load() {
    if (!model_.init())
        app_fatal("StarMap", "Model initialization failed");
    view_.create(root);
    style::show_demo(root, model_.settings().simulated);
    utils::attach_controls(std::array{root}, clicked, this);
}

void StarMap::on_view_will_appear() {
    model_.set_status_bar(true);
    view_.update(model_.device());
    model_.set_change_callback([this] {
        view_.update(model_.device());
    });
    utils::focus_controls(std::array{root}, root);
}

void StarMap::on_view_will_disappear() {
    model_.set_change_callback({});
    utils::leave_focus_group();
}

void StarMap::on_view_unload() {
    model_.deinit();
    view_.destroy();
}

void StarMap::on_language_changed() {
    view_.apply_language();
}

void StarMap::clicked(lv_event_t* event) {
    auto* self = static_cast<StarMap*>(lv_event_get_user_data(event));
    self->page_manager->pop();
}
