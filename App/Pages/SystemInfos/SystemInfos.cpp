#include "SystemInfos.h"
#include "Log.h"
#include "PageManager.h"
#include "Utils/PageUtils.h"
#include "Utils/PageStyle.h"

using namespace page;

void SystemInfos::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void SystemInfos::onViewLoad() {
    if (!model_.init())
        app_fatal("SystemInfos", "Model initialization failed");
    view_.create(getRoot());
    style::show_demo(getRoot(), model_.settings().simulated);
    utils::attach_controls(std::array{getRoot()}, clicked, this);
}

void SystemInfos::onViewWillAppear() {
    model_.set_status_bar(false);
    view_.apply_language();
    view_.update(model_.device(), model_.settings());
    model_.set_change_callback([this] {
        view_.update(model_.device(), model_.settings());
    });
    view_.group_init();
}

void SystemInfos::onViewWillDisappear() {
    model_.set_change_callback({});
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    utils::leave_focus_group();
}

void SystemInfos::onViewUnload() {
    model_.deinit();
    view_.destroy();
}

void SystemInfos::onLanguageChanged() {
    view_.apply_language();
    view_.update(model_.device(), model_.settings());
}

void SystemInfos::clicked(lv_event_t* event) {
    auto* self = static_cast<SystemInfos*>(lv_event_get_user_data(event));
    if (!self->getManager()->isSwitching())
        self->getManager()->pop();
}
