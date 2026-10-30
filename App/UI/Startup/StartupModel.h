#pragma once

#include "UI/Common/Model.h"
#include <array>
#include "Def/DP_Battery.h"
#include "Def/DP_Power.h"
#include "Def/DP_Version.h"

namespace page {
/** @brief 开机页面实际显示的值；不包含聚合快照或借用的回调指针。 */
struct StartupState {
    unsigned battery_percent = 0;
    bool battery_valid = false;
    float battery_voltage = 7.60f;
    int16_t battery_current_ma = 0;
    bool battery_current_valid = false;
    bool charging = false;
    std::array<char, 16> firmware{};
    std::array<char, 16> hardware{};
    bool simulated = false;
    bool powered_on = false;
};

/** @brief 电压/充电功率的最终显示文本；Model 与 View 共用同一量化规则。 */
std::array<char, 24> startup_battery_reading(const StartupState& state);

/** @brief 随页面加载创建、卸载销毁；只订阅 Battery、Version、Power，通知 StatusBar。 */
class StartupModel final : public Model<StartupState> {
  public:
    StartupModel() : Model("StartupModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});
    /** @brief 长按完成后请求开机；确认作为 PowerState::powered_on 送达。 */
    bool request_power_on();
    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::Battery::State& value);
    void apply(const dp::Version::State& value);
    void apply(const dp::Power::State& value);
    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
};
} // namespace page
