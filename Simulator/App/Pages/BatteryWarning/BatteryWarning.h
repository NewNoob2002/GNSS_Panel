#pragma once
#include <memory>
#include "Common/DataProc/DataProc.h"
#include "lvgl.h"

namespace page {
/** Battery over-temperature banner below the StatusBar (top layer). Appears at 65 C and clears below 60 C;
 * invalid temperature hides it. Subscribes to Status; create/destroy and updates on the LVGL thread only.
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
    static int on_event(Account* account, Account::EventParam_t* event);
    void update(bool valid, float temperature);
    void update_text();
    void start_effects();
    void stop_effects();
    std::unique_ptr<Account> account_;
    lv_obj_t *banner_ = nullptr, *label_ = nullptr, *marker_ = nullptr, *shine_ = nullptr;
    bool active_ = false;
    float temperature_ = 0.0f;
};
} // namespace page
