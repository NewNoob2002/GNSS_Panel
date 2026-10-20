#pragma once

#include <memory>
#include "../../Utils/lv_anim_label/numberFlow.h"
#include "../../Utils/lv_anim_label/numberFlow_clock.h"
#include "Common/DataProc/DataProc.h"

namespace page {

/** 状态由调用方提供；此 View 不读取板级或业务全局变量。数值仅在 LVGL 线程更新。 */
struct StatusBarState {
    unsigned satellites = 0;
    bool satellites_valid = false;
    const char* position = "DEMO";
    lv_color_t position_color = lv_color_hex(0xf44336);
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    bool clock_valid = false;
    bool recording = false;
    bool wifi = false;
    unsigned battery_percent = 0;
    bool battery_valid = false;
    bool charging = false;
    float battery_voltage = 7.60f;
    bool battery_temperature_valid = false;
    float battery_temperature = 25.0f; // Celsius; drives the optional BatteryWarning banner.
};

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
    static int on_event(Account* account, Account::EventParam_t* event);
    std::unique_ptr<Account> account_;
    lv_obj_t* root_ = nullptr;
    bool visible_ = false;
    lv_obj_t* position_icon_ = nullptr;
    lv_obj_t* position_label_ = nullptr;
    lv_obj_t* sd_icon_ = nullptr;
    lv_obj_t* wifi_icon_ = nullptr;
    lv_obj_t* battery_fill_ = nullptr;
    NumberFlow* satellites_ = nullptr;
    NumberFlow* battery_percent_ = nullptr;
    NumberFlowClock* clock_ = nullptr;
};

} // namespace page
