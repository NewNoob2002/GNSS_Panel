#pragma once

#include "UI/Common/Model.h"
#include "Def/DP_Clock.h"
#include "Def/DP_Power.h"
#include "Def/DP_Settings.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"

namespace page {
/** @brief 系统面板页面状态。 */
struct SystemDashState {
    i18n::Language language{};
    bool wifi_available = false;
    bool wifi_on = false;
    bool wifi_pending = false;
    bool reboot_available = true;
    bool simulated = false;
    uint64_t power_token = 0;
    DataProc::DateTime date{};
};

/** @brief 随页面加载创建、卸载销毁；只订阅领域节点，通知 StatusBar。 */
class SystemDashModel final : public Model<SystemDashState> {
  public:
    SystemDashModel() : Model("SystemDashModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    bool toggle_language();
    bool set_wifi(bool on);
    bool prepare_power(hal::PowerCommand action);
    bool cancel_power();

    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::Settings::State& value);
    void apply(const dp::WiFi::State& value);
    void apply(const dp::Version::State& value);
    void apply(const dp::Power::State& value);
    void apply(const dp::Clock::State& value);

    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
};
} // namespace page
