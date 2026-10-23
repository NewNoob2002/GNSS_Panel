#pragma once

#include <cstring>
#include "UI/Common/Model.h"
#include "Def/DP_Battery.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Init.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Work.h"

namespace page {
/** @brief 系统信息页面显示值。 */
struct SystemInfosState {
    hal::WorkState work{};
    hal::GNSSState gnss{};
    hal::WifiState wifi{};
    hal::BatteryState battery{};
    hal::RecorderState recorder{};
    hal::BoardState version{};
    dp::InitState init{};
};

/** @brief 随页面加载创建、卸载销毁；只订阅七个领域节点，通知 StatusBar。 */
class SystemInfosModel final : public Model<SystemInfosState> {
  public:
    SystemInfosModel() : Model("SystemInfosModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    int onEvent(EventParam_t* event) override;

    struct StatusCompat {
        unsigned battery_percent = 0;
        std::array<char, 16> position{};
    };
    StatusCompat status() const {
        StatusCompat s{};
        s.battery_percent = state_.battery.level_percent;
        std::strcpy(s.position.data(), state_.version.capabilities.simulated ? "DEMO" : "");
        return s;
    }

    struct SettingsCompat {
        bool initialization_started = false;
        dp::InitializationResults initialization{};
    };
    SettingsCompat settings() const {
        return {state_.init.started, state_.init.results};
    }

  private:
    void apply(const dp::Work::State& value);
    void apply(const dp::GNSS::State& value);
    void apply(const dp::WiFi::State& value);
    void apply(const dp::Battery::State& value);
    void apply(const dp::Recorder::State& value);
    void apply(const dp::Version::State& value);
    void apply(const dp::Init::State& value);

    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
};
} // namespace page
