#ifndef DIALPLATE_VIEW_H
#define DIALPLATE_VIEW_H

#include "Common/DataProc/DataProc_Def.h"
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
            lv_obj_t* radio_card;
            lv_obj_t* radio_img;
            lv_obj_t* radio_label;
            lv_obj_t* mode_card;
            lv_obj_t* mode_img;
            lv_obj_t* mode_label;
            NumberFlow* satellite_used;
            NumberFlow* satellite_tracked;
            lv_obj_t* satellite_separator;
            lv_obj_t* satellite_missing;
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
    /** 启动/停止电台波形与工作模式信标手绘动效。 */
    void anim_start();
    void anim_stop();

    /** 卫星数、电台与工作模式状态及录制按钮更新。 */
    void update(const DataProc::DeviceSnapshot& device);

  private:
    void top_info_create(lv_obj_t* par);
    void btn_cont_create(lv_obj_t* par);

    static lv_obj_t* btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x, lv_coord_t y,
                                const TransformInfo_t& transform);

    static void anim_step_exec(void* var, int32_t value);
    static void radio_draw_event_cb(lv_event_t* e);
    static void mode_draw_event_cb(lv_event_t* e);

    uint32_t anim_phase_ = 0;
    bool anim_running_ = false;
    lv_anim_t anim_loop_{};
    DataProc::DeviceSnapshot last_device_{};
};

} // namespace page

#endif // DIALPLATE_VIEW_H
