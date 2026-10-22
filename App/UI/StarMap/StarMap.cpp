#include "StarMap.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void StarMap::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void StarMap::onViewLoad() {
    if (!model_.init())
        app_fatal("StarMap", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_.settings().simulated);
    utils::attach_controls(std::array{getRoot()}, clicked, this);
}

void StarMap::onViewWillAppear() {
    model_.set_status_bar(true);
    view_.update(model_.device());
    model_.set_change_callback([this] {
        view_.update(model_.device());
    });
    utils::focus_controls(std::array{getRoot()}, getRoot());
}

void StarMap::onViewWillDisappear() {
    model_.set_change_callback({});
    utils::leave_focus_group();
}

void StarMap::onViewUnload() {
    model_.deinit();
    view_.destroy();
}

void StarMap::onLanguageChanged() {
    view_.apply_language();
}

void StarMap::clicked(lv_event_t* event) {
    auto* self = static_cast<StarMap*>(lv_event_get_user_data(event));
    self->getManager()->pop();
}
