#include "WorkSettings.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"

using namespace page;

void WorkSettings::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_LEFT, 300, lv_anim_path_ease_in_out);
}

std::array<lv_obj_t*, 5> WorkSettings::controls() const {
    const auto& r = view_.ui.roller;
    return {r.left_roller.btn_up, r.left_roller.btn_down, r.right_roller.btn_up, r.right_roller.btn_down, r.btn_reset};
}

void WorkSettings::on_view_load() {
    if (!model_.init())
        app_fatal("WorkSettings", "Model initialization failed");
    view_.create(root);
    for (auto* obj : {view_.ui.btn_cont.btn_base, view_.ui.btn_cont.btn_rover, view_.ui.btn_cont.btn_ntrip})
        lv_obj_add_state(obj, LV_STATE_DISABLED);
    utils::attach_controls(controls(), clicked, this);
}

void WorkSettings::on_view_will_appear() {
    model_.set_status_bar(true);
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.ui.roller.btn_reset);
}

void WorkSettings::on_view_will_disappear() {
    last_focus_ = utils::leave_focus_group();
}

void WorkSettings::on_view_unload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}

void WorkSettings::on_language_changed() {
    view_.apply_language();
}

void WorkSettings::clicked(lv_event_t* event) {
    auto* self = static_cast<WorkSettings*>(lv_event_get_user_data(event));
    auto* obj = lv_event_get_current_target(event);
    const auto& r = self->view_.ui.roller;
    if (obj == r.left_roller.btn_up || obj == r.left_roller.btn_down)
        self->view_.scroll(r.left_roller.label, obj == r.left_roller.btn_up ? -1 : 1);
    else if (obj == r.right_roller.btn_up || obj == r.right_roller.btn_down)
        self->view_.scroll(r.right_roller.label, obj == r.right_roller.btn_up ? -1 : 1);
    else if (obj == r.btn_reset)
        self->page_manager->pop();
}
