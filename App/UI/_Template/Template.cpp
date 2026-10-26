#include "Template.h"
#include "Log.h"
#include "UI/Common/PageStyle.h"
#include "UI/Common/PageUtils.h"

using namespace page;

void Template::onInstalled() {
    setLoadAnimType(PAGE_ANIM::PUSH_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void Template::onViewLoad() {
    model_ = std::make_unique<TemplateModel>();
    if (!model_->init())
        app_fatal("Template", "Model initialization failed");
    view_.create(getRoot());
    utils::attach_controls(std::array{view_.action_control()}, clicked, this);
}

void Template::onViewWillAppear() {
    if (!view_.bind(model_->subject()))
        app_fatal("Template", "View binding failed");
    utils::focus_controls(std::array{view_.action_control()});
}

void Template::onViewWillDisappear() {
    view_.unbind();
    utils::leave_focus_group();
}

void Template::onViewUnload() {
    view_.destroy();
    model_.reset();
}

void Template::clicked(lv_event_t* event) {
    auto* self = static_cast<Template*>(lv_event_get_user_data(event));
    if (!self->model_->toggle_recording())
        APP_LOG_W("Template", "recording command rejected");
}
