#ifndef DIALPLATE_VIEW_H
#define DIALPLATE_VIEW_H

#include "Utils/lv_anim_label/numberFlow.h"
#include "lvgl.h"

namespace page {

class DialplateView {

    typedef struct Transform {
        lv_coord_t width_default;
        lv_coord_t height_default;
        lv_coord_t transform_width;
        lv_coord_t transform_height;
    } TransformInfo_t;

  public:
    struct {
        struct {
            lv_obj_t* cont;
            lv_obj_t* icon_satellite;
            lv_obj_t* icon_radio;
            lv_obj_t* icon_mode;
            NumberFlow* satellite_used;
            NumberFlow* satellite_tacked;
        } top_info;

        struct {
            lv_obj_t* cont;
            lv_obj_t* btn_map;
            lv_obj_t* btn_rec;
            lv_obj_t* btn_menu;
            lv_obj_t* btn_shutdown;
        } btn_cont;

        lv_anim_timeline_t* anim_timeline;
    } ui;

    /** 在已初始化的 LVGL 中创建主界面；LVGL 子对象由 root 持有。 */
    void create(lv_obj_t* root);

    /** 释放动画和 C++ 辅助对象；调用方仍须清理 root 下的 LVGL 子对象。 */
    void destroy();

    void appear_anim_start(bool reverse = false) const;

  private:
    void top_info_create(lv_obj_t* par);

    void btn_cont_create(lv_obj_t* par);

    static lv_obj_t* btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x_ofs, const TransformInfo_t& transform);
};

} // namespace page

#endif // DIALPLATE_VIEW_H
