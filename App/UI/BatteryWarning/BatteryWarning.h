#pragma once
#include <memory>
#include "BatteryWarningModel.h"
#include "lvgl.h"

namespace page {
/** @brief 顶层电池过温告警：65°C 出现、低于 60°C 消除，无效温度隐藏。
 * 仅订阅 Battery/Settings；控件、observer 和动画均在应用线程创建、更新与销毁。
 */
class BatteryWarning {
  public:
    static constexpr float kCriticalC = 65.0f;
    static constexpr float kClearC = 60.0f;
    BatteryWarning() = default;
    BatteryWarning(const BatteryWarning&) = delete;
    BatteryWarning& operator=(const BatteryWarning&) = delete;
    ~BatteryWarning() {
        destroy();
    }
    bool create(lv_obj_t* parent);
    void destroy();
    void apply_language();
    bool active() const {
        return active_;
    }

  private:
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    void update(bool valid, float temperature);
    void update_text();
    void start_effects();
    void stop_effects();
    std::unique_ptr<BatteryWarningModel> model_;
    lv_observer_t* observer_ = nullptr;
    lv_obj_t *banner_ = nullptr, *label_ = nullptr, *marker_ = nullptr, *shine_ = nullptr;
    bool active_ = false;
    float temperature_ = 0.0f;
};
} // namespace page
