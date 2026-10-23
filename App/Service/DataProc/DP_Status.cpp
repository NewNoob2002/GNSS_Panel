#include "Aggregate.h"
#include "Def/DP_Battery.h"
#include "Def/DP_Clock.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Settings.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Work.h"
#include <cstdio>
namespace {
class StatusNode final : public dp::Aggregate<DataProc::StatusSnapshot, dp::Battery, dp::Clock, dp::GNSS,
    dp::Recorder, dp::Settings, dp::Version, dp::WiFi, dp::Work> {
public:
    explicit StatusNode(DataBroker* broker) : Aggregate(dp::names::Status, broker) {}
    bool init() override {
        if (!Aggregate::init()) return false;
        startTimer(200);
        return hasTimer();
    }
    int onEvent(EventParam_t* event) override {
        if (event && event->event == EVENT_TIMER) {
            /** @brief 其余八页仍经 model_utils 读取聚合快照；3.7f 前保留 200 ms 兼容重发。 */
            const auto snapshot = state_;
            (void)publish(&snapshot, sizeof(snapshot));
            return RES_OK;
        }
        return Aggregate::onEvent(event);
    }
private:
    DataProc::StatusSnapshot compose() const override {
        DataProc::StatusSnapshot value;
        const auto& battery = get<dp::Battery>();
        const auto& clock = get<dp::Clock>().clock;
        const auto& gnss = get<dp::GNSS>();
        // LVGL Material palette 的 main RGB 值；核心不能依赖 LVGL。
        i18n::TextId position = i18n::TextId::PositionNone;
        value.position_color_rgb = 0xf44336;
        if (get<dp::Work>().mode == hal::WorkMode::Base) {
            position = i18n::TextId::PositionBase;
            value.position_color_rgb = 0x2196f3;
        } else {
            switch (gnss.fix) {
            case hal::FixStatus::Single: position = i18n::TextId::PositionSingle; value.position_color_rgb = 0xffeb3b; break;
            case hal::FixStatus::Float: position = i18n::TextId::PositionFloat; value.position_color_rgb = 0xffeb3b; break;
            case hal::FixStatus::Fixed: position = i18n::TextId::PositionFix; value.position_color_rgb = 0x4caf50; break;
            default: break;
            }
        }
        std::snprintf(value.position.data(), value.position.size(), "%s", i18n::text(position));
        // Demo 原来明确显示 DEMO，保持此徽标；定位枚举仍用于真实平台。
        if (get<dp::Version>().capabilities.simulated) {
            std::snprintf(value.position.data(), value.position.size(), "%s", "DEMO");
            value.position_color_rgb = 0xf44336;
        }
        value.satellites = gnss.satellites_used;
        value.satellites_valid = gnss.satellites_valid;
        value.battery_percent = battery.level_percent;
        value.battery_valid = battery.valid;
        value.battery_voltage = battery.voltage;
        value.battery_temperature_valid = battery.temperature_valid;
        value.battery_temperature = battery.temperature;
        value.charging = battery.charge != hal::ChargeState::NotCharging;
        value.hour = clock.hour; value.minute = clock.minute; value.second = clock.second; value.clock_valid = clock.time_valid;
        value.recording = get<dp::Recorder>().reported_recording;
        value.wifi = get<dp::WiFi>().on;
        return value;
    }
};
}
DP_CREATE(Status, StatusNode)
