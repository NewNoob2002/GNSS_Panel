#include "WorkSettings.h"
#include "Log.h"
#include "PageManager.h"
#include "Utils/PageUtils.h"
#include "Utils/PageStyle.h"
using namespace page;

void WorkSettings::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}
void WorkSettings::onViewLoad() {
    if (!model_.init())
        app_fatal("WorkSettings", "Model initialization failed");
    view_.create(getRoot());
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
void WorkSettings::onViewWillAppear() {
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
void WorkSettings::onViewWillDisappear() {
    view_.stop();
    model_.set_change_callback({});
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    utils::leave_focus_group();
}
void WorkSettings::onViewUnload() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    model_.set_change_callback({});
    model_.deinit();
    view_.destroy();
}
void WorkSettings::onLanguageChanged() {
    view_.update(target_, model_.device().work_pending, feedback_.text);
    if (getState() != STATE::ACTIVITY)
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
void WorkSettings::onCommitRequest() {
    if (lv_group_get_focused(lv_group_get_default()) == view_.ui.apply)
        apply();
}
void WorkSettings::clicked(lv_event_t* event) {
    auto* self = static_cast<WorkSettings*>(lv_event_get_user_data(event));
    auto* control = lv_event_get_current_target_obj(event);
    if (control == self->view_.ui.back) {
        self->getManager()->pop();
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
