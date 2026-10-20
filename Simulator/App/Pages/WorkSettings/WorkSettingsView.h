#ifndef WorkSettings_VIEW_H
#define WorkSettings_VIEW_H

#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "lvgl.h"

namespace page {
class WorkSettingsView {
  public:
    struct {
        struct {
            lv_obj_t* cont;

            struct {
                lv_obj_t* cont;
                lv_obj_t* label;

                lv_obj_t* btn_cont;
                lv_obj_t* btn_up;
                lv_obj_t* btn_down;
            } left_roller;

            struct {
                lv_obj_t* cont;
                lv_obj_t* label;

                lv_obj_t* btn_cont;
                lv_obj_t* btn_up;
                lv_obj_t* btn_down;
            } right_roller;

            lv_obj_t* btn_reset;
        } roller;

        struct {
            lv_obj_t* cont;
            lv_obj_t* btn_base;
            lv_obj_t* btn_rover;
            lv_obj_t* btn_ntrip;
        } btn_cont;

    } ui;

    WorkSettingsView() = default;

    ~WorkSettingsView() = default;

    void create(lv_obj_t* root);

    void destroy();

    void apply_language() const;
    /** 仅更新本页滚轮预览，不写入电台配置。delta 为 -1（上一项）或 1（下一项）。 */
    void scroll(lv_obj_t* label, int delta);

    void roller_create(lv_obj_t* par);

    static void roller_style_init(lv_obj_t* obj);

    void btn_cont_create(lv_obj_t* par);

    static lv_obj_t* btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs, lv_coord_t y_ofs);

    static void roller_to_index(lv_obj_t* obj, uint8_t index);

  private:
    static int8_t left_roller_index;
    static int8_t right_roller_index;
};

} // namespace page

#endif // WorkSettings_VIEW_H
