#include "RecordConfig.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"
using namespace page;

void RecordConfig::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_OVER_TOP, style::kPageTransitionMs, lv_anim_path_ease_out);
}
void RecordConfig::on_view_load() {
    if (!model_.init())
        app_fatal("RecordConfig", "Model initialization failed");
    view_.create(root);
    utils::attach_controls(view_.controls(), clicked, this);
}
void RecordConfig::refresh() {
    const auto& device = model_.device();
    const bool configurable = model_.settings().record_settings;
    const bool editable = configurable && device.record_valid && !device.recording && !device.record_pending;
    const bool changed =
        lv_obj_has_state(view_.ui.type, LV_STATE_DISABLED) == editable
        || lv_obj_has_state(view_.ui.record, LV_STATE_DISABLED) != (device.record_pending || !device.record_valid);
    feedback_.poll(device.record_pending);
    if (!editable && device.record_valid)
        view_.set_selection(device.record_type, device.record_interval);
    view_.update(device, configurable, feedback_.text);
    if (changed)
        utils::focus_controls(view_.controls(), lv_group_get_focused(lv_group_get_default()));
}
void RecordConfig::on_view_will_appear() {
    model_.set_status_bar(true);
    view_.set_selection(model_.device().record_type, model_.device().record_interval);
    feedback_ = {};
    if (model_.device().record_pending)
        feedback_.begin();
    model_.set_change_callback([this] {
        refresh();
    });
    refresh();
    utils::focus_controls(view_.controls(), view_.ui.record);
    view_.appear();
    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<RecordConfig*>(timer->user_data)->refresh();
        },
        200, this);
    if (!timer_)
        app_fatal("RecordConfig", "Feedback timer allocation failed");
}
void RecordConfig::on_view_will_disappear() {
    view_.stop();
    model_.set_change_callback({});
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }
    utils::leave_focus_group();
}
void RecordConfig::on_view_unload() {
    if (timer_) {
        lv_timer_del(timer_);
        timer_ = nullptr;
    }
    model_.set_change_callback({});
    model_.deinit();
    view_.destroy();
}
void RecordConfig::on_language_changed() {
    view_.update(model_.device(), model_.settings().record_settings, feedback_.text);
    if (priv.state != PAGE_STATE_ACTIVITY)
        view_.stop();
}
// 开始与停止一致：单击即切换；双击（Commit）同样生效。等待主控确认期间不发第二个请求。
void RecordConfig::toggle() {
    const auto& device = model_.device();
    if (device.record_pending || !device.record_valid)
        return;
    const bool start = !device.recording;
    feedback_.begin();
    const bool accepted = start ? model_.start_record(view_.type(), view_.interval()) : model_.stop_record();
    feedback_.finish(accepted, model_.device().record_pending);
    refresh();
}
void RecordConfig::on_commit_request() {
    if (lv_group_get_focused(lv_group_get_default()) == view_.ui.record)
        toggle();
}
void RecordConfig::clicked(lv_event_t* event) {
    auto* self = static_cast<RecordConfig*>(lv_event_get_user_data(event));
    auto* control = lv_event_get_current_target(event);
    if (control == self->view_.ui.back) {
        self->page_manager->pop();
        return;
    }
    const auto& device = self->model_.device();
    if (device.record_pending || !device.record_valid)
        return;
    if (control == self->view_.ui.record) {
        self->toggle();
        return;
    }
    if (!self->model_.settings().record_settings || device.recording)
        return;
    self->feedback_ = {};
    if (control == self->view_.ui.type)
        self->view_.cycle_type();
    else if (control == self->view_.ui.interval)
        self->view_.cycle_interval();
    self->refresh();
}
