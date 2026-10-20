#include "RecordConfig.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"

using namespace page;

void RecordConfig::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_LEFT, 300, lv_anim_path_ease_in_out);
}

std::array<lv_obj_t*, 5> RecordConfig::controls() const {
    const auto& r = view_.ui.roller;
    if (!model_.settings().record_settings) // Display-only products: the current values cannot be changed.
        return {nullptr, nullptr, nullptr, nullptr, view_.ui.btn_cont.btn_return};
    return {r.left_roller.btn_up, r.left_roller.btn_down, r.right_roller.btn_up, r.right_roller.btn_down,
            view_.ui.btn_cont.btn_return};
}

void RecordConfig::on_view_load() {
    if (!model_.init())
        app_fatal("RecordConfig", "Model initialization failed");
    view_.create(root);
    lv_obj_add_state(view_.ui.btn_cont.btn_record, LV_STATE_DISABLED);
    if (!model_.settings().record_settings) {
        const auto& r = view_.ui.roller;
        for (auto* button :
             {r.left_roller.btn_up, r.left_roller.btn_down, r.right_roller.btn_up, r.right_roller.btn_down})
            lv_obj_add_state(button, LV_STATE_DISABLED);
    }
    utils::attach_controls(controls(), clicked, this);
}

void RecordConfig::on_view_will_appear() {
    model_.set_status_bar(true);
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.ui.btn_cont.btn_return);
}

void RecordConfig::on_view_will_disappear() {
    last_focus_ = utils::leave_focus_group();
}

void RecordConfig::on_view_unload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}

void RecordConfig::on_language_changed() {
    view_.apply_language();
}

void RecordConfig::clicked(lv_event_t* event) {
    auto* self = static_cast<RecordConfig*>(lv_event_get_user_data(event));
    auto* obj = lv_event_get_current_target(event);
    const auto& r = self->view_.ui.roller;
    if (obj == r.left_roller.btn_up || obj == r.left_roller.btn_down)
        self->view_.scroll(r.left_roller.label, obj == r.left_roller.btn_up ? -1 : 1);
    else if (obj == r.right_roller.btn_up || obj == r.right_roller.btn_down)
        self->view_.scroll(r.right_roller.label, obj == r.right_roller.btn_up ? -1 : 1);
    else if (obj == self->view_.ui.btn_cont.btn_return)
        self->page_manager->pop();
}
