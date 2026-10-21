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
    /** 页面出现/离开时开关电台波形与工作模式（基站播发/移动站测点）动效；实际运行与否还取决于设备状态。 */
    void anim_start();
    void anim_stop();

    /** 卫星数、电台与工作模式状态及录制按钮更新。 */
    void update(const DataProc::DeviceSnapshot& device);

  private:
    void top_info_create(lv_obj_t* par);
    void btn_cont_create(lv_obj_t* par);

    static lv_obj_t* btn_create(lv_obj_t* par, const void* img_src, lv_coord_t x, lv_coord_t y,
                                const TransformInfo_t& transform);
    static void status_card(lv_obj_t* par, lv_coord_t x, const char* image, lv_obj_t*& card, lv_obj_t*& img,
                            lv_obj_t*& label);
    /** 页面可见且工作模式有效时运行动效；否则删除动画，画面保持静态。 */
    void sync_anim();

    static void anim_step_exec(void* var, int32_t value);
    static void radio_draw_event_cb(lv_event_t* e);
    static void mode_draw_event_cb(lv_event_t* e);

    static constexpr lv_coord_t kCardWidth = 54, kCardHeight = 44;
    static constexpr lv_coord_t kIconX = 19, kIconY = 9; // 16 px 图标在卡片内的位置，上方留给动效。
    static constexpr uint32_t kAnimPeriod = 1200, kAnimStep = 50;

    uint32_t anim_phase_ = 0;
    bool anim_enabled_ = false;
    bool anim_running_ = false;
    lv_anim_t anim_loop_{};
    DataProc::DeviceSnapshot last_device_{};
};

} // namespace page

#endif // DIALPLATE_VIEW_H
