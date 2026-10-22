#ifndef STAR_MAP_VIEW_H
#define STAR_MAP_VIEW_H

#include "DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"
#include "I18n.h"
#include "lvgl.h"

namespace page {

class StarMapView {
  public:
    struct {
        lv_obj_t* label_title;
        lv_obj_t* activity_indicator;
        lv_obj_t* divider;

        struct {
            lv_obj_t* cont;
            lv_obj_t* accent;
            lv_obj_t* label_name;
            lv_obj_t* label_val;
        } constell[7];
    } ui;

    bool activity_indicator_on = false;

    void create(lv_obj_t* root);
    void destroy();
    void apply_language() const;
    void update_activity_indicator();
    /** Constellation counts from device data; "-" while they are not valid. */
    void update(const DataProc::DeviceSnapshot& device);
};

} // namespace page

#endif // STAR_MAP_VIEW_H
