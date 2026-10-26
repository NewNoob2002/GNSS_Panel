#include "StarMap.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void StarMap::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(style::page_transition_path);
}

void StarMap::onViewLoad() {
    model_ = std::make_unique<StarMapModel>();
    if (!model_->init())
        app_fatal("StarMap", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_->state().simulated);
    utils::attach_controls(std::array{getRoot()}, clicked, this);
}

void StarMap::onViewWillAppear() {
    model_->set_status_bar(true);
    if (!view_.bind(model_->subject()))
        app_fatal("StarMap", "View binding failed");
    utils::focus_controls(std::array{getRoot()}, getRoot());
}

void StarMap::onViewWillDisappear() {
    view_.unbind();
    utils::leave_focus_group();
}

void StarMap::onViewUnload() {
    view_.destroy();
    model_.reset();
}

void StarMap::onLanguageChanged() {
    view_.apply_language();
    view_.update(model_->state());
}

void StarMap::clicked(lv_event_t* event) {
    auto* self = static_cast<StarMap*>(lv_event_get_user_data(event));
    self->getManager()->pop();
}
