#ifndef DIALPLATE_VIEW_H
#define DIALPLATE_VIEW_H

#include <array>
#include "DialplateModel.h"
#include "Utils/lv_anim_label/numberFlow.h"
#include "UI/Common/FocusHighlight.h"
#include "UI/Common/PageStyle.h"
#include "lvgl.h"

namespace page {

class DialplateView {
    typedef struct Transform {
        int32_t width_default;
        int32_t height_default;
        int32_t transform_width;
        int32_t transform_height;
    } TransformInfo_t;

  private:
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

  public:
    /** @brief 控件只读引用用于布局验证；Presenter 通过语义控件接口转发输入。 */
    const auto& widgets() const { return ui; }
    enum class Control { Work, Record, Menu, Power, Satellite, None };
    std::array<lv_obj_t*, 5> controls(bool star_map) const;
    lv_obj_t* default_focus() const { return ui.btn_cont.btn_menu; }
    Control control(const lv_obj_t* object) const;
    /** @brief 绑定时立即渲染 Model 自有值；离页先解绑，卸载后不再触碰控件。 */
    bool bind(lv_subject_t* subject);
    void unbind();

    /** 在已初始化的 LVGL 中创建主界面；LVGL 子对象由 root 持有。 */
    void create(lv_obj_t* root);

    /** 释放动画和 C++ 辅助对象；调用方仍须清理 root 下的 LVGL 子对象。 */
    void destroy();

    /** @brief 进入及恢复直接对齐，后续焦点切换保速。 */
    void resume_focus();
    /** @brief 从焦点入口展开为页面背景色面板盖住首页，静止后调用 done；无覆盖框（独立预览）时返回 false。 */
    bool open_focus(FocusHighlight::DoneCallback done, void* user_data) {
        return focus_.open(style::kBackground, done, user_data);
    }
    /** @brief 返回首页时把展开面板收回到原入口；未展开时无操作。 */
    void close_focus() { focus_.close(); }
    const FocusHighlight& focus_highlight() const { return focus_; }
    void appear_anim_start(bool reverse = false) const;
    /** 页面出现/离开时开关电台波形与工作模式（基站播发/移动站测点）动效；实际运行与否还取决于设备状态。 */
    void anim_start();
    void anim_stop();

    /** 卫星数、电台与工作模式状态及录制按钮更新。 */
    void update(const DialplateState& device);

  private:
    void top_info_create(lv_obj_t* par);
    void btn_cont_create(lv_obj_t* par);

    static lv_obj_t* btn_create(lv_obj_t* par, const void* img_src, int32_t x, int32_t y,
                                const TransformInfo_t& transform);
    static void status_card(lv_obj_t* par, int32_t x, const char* image, lv_obj_t*& card, lv_obj_t*& img,
                            lv_obj_t*& label);
    /** 页面可见且工作模式有效时运行动效；否则删除动画，画面保持静态。 */
    void sync_anim();

    static void anim_step_exec(void* var, int32_t value);
    static void radio_draw_event_cb(lv_event_t* e);
    static void mode_draw_event_cb(lv_event_t* e);

    static constexpr int32_t kCardWidth = 54, kCardHeight = 44;
    static constexpr int32_t kIconX = 19, kIconY = 9; // 16 px 图标在卡片内的位置，上方留给动效。
    static constexpr uint32_t kAnimPeriod = 1200, kAnimStep = 50;

    FocusHighlight focus_{};
    uint32_t anim_phase_ = 0;
    bool anim_enabled_ = false;
    bool anim_running_ = false;
    lv_anim_t anim_loop_{};
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;
    DialplateState last_device_{};
};

} // namespace page

#endif // DIALPLATE_VIEW_H
