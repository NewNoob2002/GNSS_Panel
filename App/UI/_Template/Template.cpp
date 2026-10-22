#include "Template.h"
#include "Log.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void Template::onInstalled() {
    setLoadAnimType(PAGE_ANIM::MOVE_LEFT);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

void Template::onViewLoad() {
    if (!model_.init())
        app_fatal("Template", "Model initialization failed");
    view_.create(getRoot());
}

void Template::onViewUnload() {
    model_.deinit();
    view_.destroy();
}
