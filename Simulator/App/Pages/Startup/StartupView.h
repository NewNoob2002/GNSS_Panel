#ifndef STARTUP_VIEW_H
#define STARTUP_VIEW_H

#include "Common/DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "lvgl.h"

namespace page {
class StartupView {
  public:
    void create(lv_obj_t* root);

    void destroy() const;

    void update(const DataProc::StatusSnapshot& status);

    void apply_language() const;

    struct {
        lv_obj_t* arc;
        lv_obj_t* arc_percent;
        lv_anim_t arc_anim;

        lv_obj_t* btn_press;
        lv_obj_t* btn_label;
        lv_obj_t* hold_hint;

        lv_obj_t* bat_icon;
        lv_obj_t* bat_caption;
        lv_obj_t* bat_charge;
        lv_obj_t* bat_voltage;
        lv_obj_t* bat_level;
        lv_obj_t* bat_fill;
    } ui;
};
} // namespace page

#endif // STARTUP_VIEW_H
