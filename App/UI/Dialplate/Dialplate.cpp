#include "Dialplate.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void Dialplate::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

std::array<lv_obj_t*, 5> Dialplate::controls() const {
    // The satellite icon only opens StarMap on products that have it; otherwise it is display only.
    return {view_.ui.btn_cont.btn_map, view_.ui.btn_cont.btn_rec, view_.ui.btn_cont.btn_menu,
            view_.ui.btn_cont.btn_shutdown, model_.settings().star_map ? view_.ui.top_info.icon_satellite : nullptr};
}

void Dialplate::onViewLoad() {
    if (!model_.init())
        app_fatal("Dialplate", "Model initialization failed");
    view_.create(getRoot());
    utils::attach_controls(controls(), clicked, this);
    entrance_pending_ = true;
}

void Dialplate::onViewWillAppear() {
    model_.set_status_bar(true);
    view_.update(model_.device());
    model_.set_change_callback([this] {
        view_.update(model_.device());
    });
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.ui.btn_cont.btn_menu);
    if (entrance_pending_) {
        // Returning from a child slides the page back already; replaying the entrance would double the work.
        entrance_pending_ = false;
        view_.appear_anim_start();
    }
    view_.anim_start();
}

void Dialplate::onViewWillDisappear() {
    view_.anim_stop();
    model_.set_change_callback({});
    last_focus_ = utils::leave_focus_group();
}

void Dialplate::onViewUnload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}

void Dialplate::clicked(lv_event_t* event) {
    auto* self = static_cast<Dialplate*>(lv_event_get_user_data(event));
    auto* button = lv_event_get_current_target_obj(event);
    const char* destination = nullptr;
    if (button == self->view_.ui.btn_cont.btn_map)
        destination = "Pages/WorkSettings";
    else if (button == self->view_.ui.btn_cont.btn_rec)
        destination = "Pages/RecordConfig";
    else if (button == self->view_.ui.btn_cont.btn_menu)
        destination = "Pages/SystemInfos";
    else if (button == self->view_.ui.btn_cont.btn_shutdown)
        destination = "Pages/SystemDash";
    else if (button == self->view_.ui.top_info.icon_satellite)
        destination = "Pages/StarMap";
    if (destination)
        self->getManager()->push(destination);
}
