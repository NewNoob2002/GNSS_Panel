#include "Dialplate.h"
#include "Utils/Log/Log.h"
#include "Utils/PageManager/PageManager.h"
#include "Utils/PageManager/PageUtils.h"

using namespace page;

void Dialplate::on_custom_attr_config() {
    set_custom_load_anim_type(PageManager::LOAD_ANIM_FADE_ON, 320, lv_anim_path_ease_in_out);
}

std::array<lv_obj_t*, 5> Dialplate::controls() const {
    // The satellite icon only opens StarMap on products that have it; otherwise it is display only.
    return {view_.ui.btn_cont.btn_map, view_.ui.btn_cont.btn_rec, view_.ui.btn_cont.btn_menu,
            view_.ui.btn_cont.btn_shutdown, model_.settings().star_map ? view_.ui.top_info.icon_satellite : nullptr};
}

void Dialplate::on_view_load() {
    if (!model_.init())
        app_fatal("Dialplate", "Model initialization failed");
    view_.create(root);
    utils::attach_controls(controls(), clicked, this);
}

void Dialplate::on_view_will_appear() {
    model_.set_status_bar(true);
    view_.update(model_.device());
    model_.set_change_callback([this] {
        view_.update(model_.device());
    });
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.ui.btn_cont.btn_menu);
    view_.appear_anim_start();
    view_.anim_start();
}

void Dialplate::on_view_will_disappear() {
    view_.anim_stop();
    model_.set_change_callback({});
    last_focus_ = utils::leave_focus_group();
}

void Dialplate::on_view_unload() {
    model_.deinit();
    view_.destroy();
    last_focus_ = nullptr;
}

void Dialplate::clicked(lv_event_t* event) {
    auto* self = static_cast<Dialplate*>(lv_event_get_user_data(event));
    auto* button = lv_event_get_current_target(event);
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
        self->page_manager->push(destination);
}
