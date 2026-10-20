#include "RecordConfig.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"

using namespace page;

void RecordConfig::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_LEFT, 300, lv_anim_path_ease_in_out);
}

bool RecordConfig::recording() const {
    return model_.device().record_valid && model_.device().recording;
}

std::array<lv_obj_t*, 6> RecordConfig::controls() const {
    const auto& r = view_.ui.roller;
    const auto& b = view_.ui.btn_cont;
    // Type/interval are fixed on display-only products and while a recording uses them.
    if (!model_.settings().record_settings || recording())
        return {nullptr, nullptr, nullptr, nullptr, b.btn_record, b.btn_return};
    return {r.left_roller.btn_up,    r.left_roller.btn_down, r.right_roller.btn_up,
            r.right_roller.btn_down, b.btn_record,           b.btn_return};
}

void RecordConfig::refresh() {
    const bool editable = model_.settings().record_settings && !recording();
    const auto& r = view_.ui.roller;
    for (auto* button :
         {r.left_roller.btn_up, r.left_roller.btn_down, r.right_roller.btn_up, r.right_roller.btn_down}) {
        if (editable)
            lv_obj_clear_state(button, LV_STATE_DISABLED);
        else
            lv_obj_add_state(button, LV_STATE_DISABLED);
    }
    view_.set_recording(recording());
}

void RecordConfig::on_view_load() {
    if (!model_.init())
        app_fatal("RecordConfig", "Model initialization failed");
    view_.create(root);
    const auto& r = view_.ui.roller;
    utils::attach_controls(std::array{r.left_roller.btn_up, r.left_roller.btn_down, r.right_roller.btn_up,
                                      r.right_roller.btn_down, view_.ui.btn_cont.btn_record,
                                      view_.ui.btn_cont.btn_return},
                           clicked, this);
}

void RecordConfig::on_view_will_appear() {
    model_.set_status_bar(true);
    if (model_.device().record_valid)
        view_.set_selection(model_.device().record_type, model_.device().record_interval);
    refresh();
    model_.set_change_callback([this] {
        refresh();
        utils::focus_controls(controls(), lv_group_get_focused(lv_group_get_default()));
    });
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.ui.btn_cont.btn_return);
}

void RecordConfig::on_view_will_disappear() {
    model_.set_change_callback({});
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
    const bool editable = self->model_.settings().record_settings && !self->recording();
    if (editable && (obj == r.left_roller.btn_up || obj == r.left_roller.btn_down))
        self->view_.scroll(r.left_roller.label, obj == r.left_roller.btn_up ? -1 : 1);
    else if (editable && (obj == r.right_roller.btn_up || obj == r.right_roller.btn_down))
        self->view_.scroll(r.right_roller.label, obj == r.right_roller.btn_up ? -1 : 1);
    else if (obj == self->view_.ui.btn_cont.btn_record && self->recording()) {
        // Select stops; starting needs an explicit Commit (on_commit_request).
        if (!self->model_.stop_record())
            APP_LOG_E("RecordConfig", "stop request rejected");
    } else if (obj == self->view_.ui.btn_cont.btn_return)
        self->page_manager->pop();
}

void RecordConfig::on_commit_request() {
    if (lv_group_get_focused(lv_group_get_default()) != view_.ui.btn_cont.btn_record || recording())
        return;
    if (!model_.start_record(view_.type(), view_.interval()))
        APP_LOG_E("RecordConfig", "start request rejected");
}
