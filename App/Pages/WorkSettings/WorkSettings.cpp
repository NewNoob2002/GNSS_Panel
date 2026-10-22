#include "WorkSettings.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"
#include "Utils/PageStyle.h"
using namespace page;

void WorkSettings::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_MOVE_LEFT, style::kPageTransitionMs, lv_anim_path_ease_out);
}
void WorkSettings::on_view_load() {
    if (!model_.init())
        app_fatal("WorkSettings", "Model initialization failed");
    view_.create(root);
    utils::attach_controls(view_.controls(), clicked, this);
}
void WorkSettings::refresh() {
    const bool pending = model_.device().work_pending;
    const bool controls_changed =
        lv_obj_is_disabled(view_.ui.apply) != pending
        || lv_obj_is_hidden(view_.ui.protocol) != (target_ == DataProc::WorkTarget::Ntrip);
    feedback_.poll(pending);
    view_.update(target_, pending, feedback_.text);
    if (controls_changed)
        utils::focus_controls(view_.controls(), lv_group_get_focused(lv_group_get_default()));
}
void WorkSettings::on_view_will_appear() {
    model_.set_status_bar(true);
    const auto& device = model_.device();
    target_ = device.ntrip == DataProc::NtripRole::Server && !device.radio_on ? DataProc::WorkTarget::Ntrip
              : device.work_mode == DataProc::WorkMode::Base                  ? DataProc::WorkTarget::Base
                                                                              : DataProc::WorkTarget::Rover;
    view_.set_selection(device.radio_protocol, device.radio_channel);
    feedback_ = {};
    if (device.work_pending)
        feedback_.begin();
    model_.set_change_callback([this] {
        refresh();
    });
    refresh();
    utils::focus_controls(view_.controls(), device.work_pending ? view_.ui.back : view_.ui.preset);
    view_.appear();
    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<WorkSettings*>(lv_timer_get_user_data(timer))->refresh();
        },
        200, this);
    if (!timer_)
        app_fatal("WorkSettings", "Feedback timer allocation failed");
}
void WorkSettings::on_view_will_disappear() {
    view_.stop();
    model_.set_change_callback({});
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    utils::leave_focus_group();
}
void WorkSettings::on_view_unload() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    model_.set_change_callback({});
    model_.deinit();
    view_.destroy();
}
void WorkSettings::on_language_changed() {
    view_.update(target_, model_.device().work_pending, feedback_.text);
    if (priv.state != PAGE_STATE_ACTIVITY)
        view_.stop();
}
void WorkSettings::apply() {
    if (model_.device().work_pending)
        return;
    if (target_ != DataProc::WorkTarget::Ntrip && view_.channel() == 0) {
        feedback_.text = i18n::TextId::SelectChannel;
        refresh();
        return;
    }
    feedback_.begin();
    const bool accepted = model_.apply_work(target_, view_.protocol(), view_.channel());
    feedback_.finish(accepted, model_.device().work_pending);
    refresh();
}
void WorkSettings::on_commit_request() {
    if (lv_group_get_focused(lv_group_get_default()) == view_.ui.apply)
        apply();
}
void WorkSettings::clicked(lv_event_t* event) {
    auto* self = static_cast<WorkSettings*>(lv_event_get_user_data(event));
    auto* control = lv_event_get_current_target_obj(event);
    if (control == self->view_.ui.back) {
        self->page_manager->pop();
        return;
    }
    if (self->model_.device().work_pending)
        return;
    if (control == self->view_.ui.apply) {
        self->apply();
        return;
    }
    self->feedback_ = {};
    if (control == self->view_.ui.preset) {
        self->target_ = self->target_ == DataProc::WorkTarget::Rover  ? DataProc::WorkTarget::Base
                        : self->target_ == DataProc::WorkTarget::Base ? DataProc::WorkTarget::Ntrip
                                                                      : DataProc::WorkTarget::Rover;
    } else if (control == self->view_.ui.protocol)
        self->view_.cycle_protocol();
    else if (control == self->view_.ui.channel)
        self->view_.cycle_channel();
    self->refresh();
}
