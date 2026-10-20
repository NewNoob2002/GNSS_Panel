#ifndef SAVECONFIG_VIEW_H
#define SAVECONFIG_VIEW_H

#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "Utils/lv_ext/lv_anim_timeline_wrapper.h"
#include "lvgl.h"

namespace page {
class SaveConfigView {
  public:
    void create(lv_obj_t* root);

    void destroy();

    void apply_language() const;

    void set_power_off_cause(i18n::TextId cause_id) const;

    struct {
        struct {
            lv_obj_t* cont;
            lv_obj_t* label;
            lv_obj_t* brand_cont;
            lv_obj_t* brand_shine;
            lv_obj_t* percent_label;
            lv_obj_t* dots[3];
            lv_anim_t dot_anims[3];
            lv_anim_t shine_anim;

            struct {
                lv_obj_t* track;
                lv_obj_t* obj;
                lv_anim_t anim;
            } bar;
        } sync;

        lv_anim_timeline_t* anim_timeline;
    } ui;
};
} // namespace page

#endif // !SHUTDOWN_VIEW_H
