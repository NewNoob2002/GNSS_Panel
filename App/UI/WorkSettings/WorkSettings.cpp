#include "WorkSettings.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"
using namespace page;

void WorkSettings::onInstalled() {
    setLoadAnimType(PAGE_ANIM::PUSH_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void WorkSettings::onViewLoad() {
    model_ = std::make_unique<WorkSettingsModel>();
    if (!model_->init())
        app_fatal("WorkSettings", "Model initialization failed");
    view_.create(getRoot());
    utils::attach_controls(view_.controls(), clicked, this);
}

void WorkSettings::onViewWillAppear() {
    model_->set_status_bar(true);
    model_->sync_initial(model_->state().work);
    if (!view_.bind(model_->subject()))
        app_fatal("WorkSettings", "View binding failed");
    utils::focus_controls(view_.controls(), view_.default_focus(model_->state().work.pending));
    view_.appear();
    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<WorkSettings*>(lv_timer_get_user_data(timer))->model_->poll_feedback();
        },
        200, this);
    if (!timer_)
        app_fatal("WorkSettings", "Feedback timer allocation failed");
}

void WorkSettings::onViewWillDisappear() {
    view_.stop();
    view_.unbind();
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
    view_.destroy();
    model_.reset();
}

void WorkSettings::onLanguageChanged() {
    view_.update(model_->state());
    view_.sync_focus();
    if (getState() != STATE::ACTIVITY)
        view_.stop();
}

void WorkSettings::apply() {
    if (model_->state().work.pending)
        return;
    if (model_->state().target != hal::WorkTarget::Ntrip && model_->state().channel == 0) {
        model_->select_channel_feedback();
        return;
    }
    model_->apply();
}

void WorkSettings::onCommitRequest() {
    if (view_.control(lv_group_get_focused(lv_group_get_default())) == WorkSettingsView::Control::Apply)
        apply();
}

void WorkSettings::clicked(lv_event_t* event) {
    auto* self = static_cast<WorkSettings*>(lv_event_get_user_data(event));
    auto* control = lv_event_get_current_target_obj(event);
    const auto target_ctrl = self->view_.control(control);
    if (target_ctrl == WorkSettingsView::Control::Back) {
        self->getManager()->pop();
        return;
    }
    if (self->model_->state().work.pending)
        return;
    if (target_ctrl == WorkSettingsView::Control::Apply) {
        self->apply();
        return;
    }
    if (target_ctrl == WorkSettingsView::Control::Preset) {
        self->model_->cycle_target();
        utils::focus_controls(self->view_.controls(), lv_group_get_focused(lv_group_get_default()));
    } else if (target_ctrl == WorkSettingsView::Control::Protocol) {
        self->model_->cycle_protocol();
    } else if (target_ctrl == WorkSettingsView::Control::Channel) {
        self->model_->cycle_channel();
    }
}
