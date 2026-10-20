#pragma once

#include <array>
#include "lvgl.h"

namespace page::utils {

/** Register UI-thread short-click controls; the page/root owns their lifetime. */
template <size_t N>
void attach_controls(const std::array<lv_obj_t*, N>& controls, lv_event_cb_t callback, void* context) {
    for (auto* obj : controls) {
        if (!obj)
            continue;
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(obj, callback, LV_EVENT_SHORT_CLICKED, context);
    }
}

/** Rebuild the shared focus group after the previous page has left it. */
template <size_t N> void focus_controls(const std::array<lv_obj_t*, N>& controls, lv_obj_t* preferred = nullptr) {
    auto* group = lv_group_get_default();
    lv_group_remove_all_objs(group);
    lv_group_set_wrap(group, true);
    lv_obj_t* focus = nullptr;
    for (auto* obj : controls) {
        if (!obj)
            continue;
        lv_group_add_obj(group, obj);
        if (!focus || obj == preferred)
            focus = obj;
    }
    if (focus)
        lv_group_focus_obj(focus);
}

inline lv_obj_t* leave_focus_group() {
    auto* group = lv_group_get_default();
    auto* focused = lv_group_get_focused(group);
    lv_group_remove_all_objs(group);
    return focused;
}

} // namespace page::utils
