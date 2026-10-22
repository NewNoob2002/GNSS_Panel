#ifndef STARTUP_VIEW_H
#define STARTUP_VIEW_H

#include "DataProc/DataProc_Def.h"
#include "UI/Resource/ResourcePool.h"
#include "I18n.h"
#include "lvgl.h"

namespace page {
class StartupView {
  public:
    /** Create the standby battery readout and transparent hold-to-start target. */
    void create(lv_obj_t* root);

    /** Stop the charging animation before the page is hidden or destroyed. */
    void destroy() const;

    /** Firmware/board versions from device data; empty text shows N/A. */
    void update_device(const DataProc::DeviceSnapshot& device) const;
    /** Refresh the split digits/unit readout and battery slot from live measurements. */
    void update(const DataProc::StatusSnapshot& status);

    /** Refresh the start and hold labels without changing the hold target. */
    void apply_language() const;

    struct {
        lv_obj_t* arc;
        lv_obj_t* arc_percent;
        lv_anim_t arc_anim;

        lv_obj_t* btn_press;
        lv_obj_t* btn_label;
        lv_obj_t* hold_hint;

        lv_obj_t* bat_caption;
        lv_obj_t* bat_charge;
        lv_obj_t* bat_unit;
        lv_obj_t* bat_voltage;
        lv_obj_t* bat_fill;

        lv_obj_t* firmware;
        lv_obj_t* hardware;
    } ui;
};
} // namespace page

#endif // STARTUP_VIEW_H
