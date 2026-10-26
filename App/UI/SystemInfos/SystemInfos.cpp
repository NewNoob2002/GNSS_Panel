#include "SystemInfos.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void SystemInfos::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void SystemInfos::onViewLoad() {
    model_ = std::make_unique<SystemInfosModel>();
    if (!model_->init())
        app_fatal("SystemInfos", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_->state().version.capabilities.simulated);
    utils::attach_controls(std::array{getRoot()}, clicked, this);
}

void SystemInfos::onViewWillAppear() {
    model_->set_status_bar(false);
    view_.apply_language();
    if (!view_.bind(model_->subject()))
        app_fatal("SystemInfos", "View binding failed");
    view_.group_init();
}

void SystemInfos::onViewWillDisappear() {
    view_.unbind();
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    utils::leave_focus_group();
}

void SystemInfos::onViewUnload() {
    view_.destroy();
    model_.reset();
}

void SystemInfos::onLanguageChanged() {
    view_.apply_language();
    view_.update(model_->state());
}

void SystemInfos::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemInfos*>(lv_event_get_user_data(event));
    if (!self->getManager()->isSwitching())
        self->getManager()->pop();
}
