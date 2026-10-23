#pragma once

#include "StarMapModel.h"
#include "UI/Resource/ResourcePool.h"
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

    bool bind(lv_subject_t* subject);
    void unbind();

    void apply_language() const;
    void update_activity_indicator();
    /** Constellation counts from domain data; "-" while they are not valid. */
    void update(const StarMapState& state);

  private:
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;
};

} // namespace page
