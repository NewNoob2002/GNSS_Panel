#pragma once

#include <memory>
#include "Utils/lv_anim_label/numberFlow.h"
#include "Utils/lv_anim_label/numberFlow_clock.h"
#include "StatusBarModel.h"

namespace page {

/** @brief 自治状态栏 View；控件与动画仅由本对象在应用线程修改。 */
class StatusBar {
  public:
    StatusBar() = default;
    ~StatusBar() {
        destroy();
    }
    StatusBar(const StatusBar&) = delete;
    StatusBar& operator=(const StatusBar&) = delete;
    /** 注册并创建屏外视图；显示/隐藏通知触发 500ms y=−26↔0 动画，不改变根对象 hidden flag。
     * 反向请求从当前位置继续；应用通道必须活到 destroy() 之后。重复注册返回 false。 */
    bool create(lv_obj_t* parent);
    /** 创建成功后在 LVGL 线程更新；隐藏时仍保存新数据，不自动显示根对象。 */
    void update(const StatusBarState& state);
    void destroy();
    lv_obj_t* root() const {
        return root_;
    }

  private:
    static void set_y(void* obj, int32_t y);
    static bool on_presentation(void* owner, const DataProc::StatusBarPresentation& request);
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    /** SD 图标跟随录制状态：滑出状态栏 -> 换色 -> 滑回；录制中滑回后持续闪烁。目标变化可随时打断。 */
    void sd_show(bool recording);
    static void sd_slide(void* obj, int32_t y);
    static void sd_blink(void* obj, int32_t opa);
    static void sd_hidden(lv_anim_t* animation);
    static void sd_entered(lv_anim_t* animation);
    std::unique_ptr<StatusBarModel> model_;
    lv_observer_t* observer_ = nullptr;
    lv_obj_t* root_ = nullptr;
    bool visible_ = false;
    lv_obj_t* position_icon_ = nullptr;
    lv_obj_t* position_label_ = nullptr;
    lv_obj_t* sd_icon_ = nullptr;
    bool sd_recording_ = false; // 最新上报的录制状态，滑出结束时按它换色。
    lv_obj_t* wifi_icon_ = nullptr;
    lv_obj_t* battery_fill_ = nullptr;
    NumberFlow* satellites_ = nullptr;
    NumberFlow* battery_percent_ = nullptr;
    NumberFlowClock* clock_ = nullptr;
};

} // namespace page
