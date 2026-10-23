#include "RecordConfig.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"
using namespace page;

void RecordConfig::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void RecordConfig::onViewLoad() {
    model_ = std::make_unique<RecordConfigModel>();
    if (!model_->init())
        app_fatal("RecordConfig", "Model initialization failed");
    view_.create(getRoot());
    utils::attach_controls(view_.controls(), clicked, this);
}

void RecordConfig::onViewWillAppear() {
    model_->set_status_bar(true);
    model_->sync_initial(model_->state().recorder, model_->state().record_settings);
    if (!view_.bind(model_->subject()))
        app_fatal("RecordConfig", "View binding failed");
    utils::focus_controls(view_.controls(), view_.record_control());
    view_.appear();
    timer_ = lv_timer_create(
        [](lv_timer_t* timer) {
            static_cast<RecordConfig*>(lv_timer_get_user_data(timer))->model_->poll_feedback();
        },
        200, this);
    if (!timer_)
        app_fatal("RecordConfig", "Feedback timer allocation failed");
}

void RecordConfig::onViewWillDisappear() {
    view_.stop();
    view_.unbind();
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    utils::leave_focus_group();
}

void RecordConfig::onViewUnload() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
    view_.destroy();
    model_.reset();
}

void RecordConfig::onLanguageChanged() {
    view_.update(model_->state());
    view_.sync_focus();
    if (getState() != STATE::ACTIVITY)
        view_.stop();
}

// 开始与停止一致：单击即切换；双击（Commit）同样生效。等待主控确认期间不发第二个请求。
void RecordConfig::toggle() {
    const auto& recorder = model_->state().recorder;
    if (recorder.pending || !recorder.valid)
        return;
    model_->toggle();
}

void RecordConfig::onCommitRequest() {
    if (view_.control(lv_group_get_focused(lv_group_get_default())) == RecordConfigView::Control::Record)
        toggle();
}

void RecordConfig::clicked(lv_event_t* event) {
    auto* self = static_cast<RecordConfig*>(lv_event_get_user_data(event));
    auto* control = lv_event_get_current_target_obj(event);
    const auto target_ctrl = self->view_.control(control);
    if (target_ctrl == RecordConfigView::Control::Back) {
        self->getManager()->pop();
        return;
    }
    const auto& recorder = self->model_->state().recorder;
    if (recorder.pending || !recorder.valid)
        return;
    if (target_ctrl == RecordConfigView::Control::Record) {
        self->toggle();
        return;
    }
    if (!self->model_->state().record_settings || recorder.recording)
        return;
    if (target_ctrl == RecordConfigView::Control::Type)
        self->model_->cycle_type();
    else if (target_ctrl == RecordConfigView::Control::Interval)
        self->model_->cycle_interval();
}
