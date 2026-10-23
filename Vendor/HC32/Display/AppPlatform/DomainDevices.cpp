#include "DomainDevices.h"
#include "BusinessDevice.h"
#include "Def/HAL_Battery.h"
#include "Def/HAL_Board.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Init.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Work.h"
#include "PowerDevice.h"
#include "StatusDevice.h"
#include "SystemState.h"
#include "lvgl.h"

namespace {
/** @brief 共用旧快照中的电量计映射，策略电量从只读状态独立采样。 */
class BatteryDevice final : public hal::ReadObject<hal::BatteryState> {
  public:
    explicit BatteryDevice(BusinessDevice& device) : ReadObject(hal::names::Battery), device_(device) {}

    hal::BatteryState
    state() override {
        const auto d = device_.snapshot(lv_tick_get());
        const auto& battery = systemInfo.power.batteryInfo;
        return {d.power_valid,
                battery.Percent,
                d.battery_percent,
                d.battery_voltage,
                battery.sample_valid,
                d.battery_temperature,
                static_cast<hal::ChargeState>(d.charge),
                d.external_power};
    }

  private:
    BusinessDevice& device_;
};

/** @brief 定位状态独立于旧状态栏的基站文字与颜色。 */
class GNSSDevice final : public hal::ReadObject<hal::GNSSState> {
  public:
    explicit GNSSDevice(BusinessDevice& device) : ReadObject(hal::names::GNSS), device_(device) {}

    hal::GNSSState
    state() override {
        const auto d = device_.snapshot(lv_tick_get());
        hal::FixStatus fix = hal::FixStatus::None;
        switch (systemInfo.host.position.coordinate_status) {
            case position_single: fix = hal::FixStatus::Single; break;
            case position_float: fix = hal::FixStatus::Float; break;
            case position_fix: fix = hal::FixStatus::Fixed; break;
            default: break;
        }
        return {d.position_valid,     d.latitude,
                d.longitude,          d.altitude,
                d.satellites_valid,   d.satellites_used,
                d.satellites_tracked, d.constellations_valid,
                d.constellations,     fix};
    }

  private:
    BusinessDevice& device_;
};

/** @brief 保留旧状态栏的运行时钟，不访问应用模拟日期或 UI 文本。 */
class ClockDevice final : public hal::ReadObject<hal::ClockState> {
  public:
    ClockDevice() : ReadObject(hal::names::Clock) {}

    hal::ClockState
    state() override {
        const uint32_t now = lv_tick_get();
        const uint32_t seconds = now / 1000U;
        return {true, (seconds / 3600U) % 24U, (seconds / 60U) % 60U, seconds % 60U, hal::ClockSource::Uptime, now};
    }
};

/** @brief 工作命令复用旧设备的范围校验、协议映射与 State::Request_Work。 */
class WorkDevice final : public hal::WriteObject<hal::WorkState, hal::WorkCommand> {
  public:
    explicit WorkDevice(BusinessDevice& device) : WriteObject(hal::names::Work), device_(device) {}

    hal::WorkState
    state() override {
        const auto d = device_.snapshot(lv_tick_get());
        return {d.work_valid,     d.work_pending,  static_cast<hal::WorkMode>(d.work_mode), d.radio_on,
                d.radio_protocol, d.radio_channel, static_cast<hal::NtripRole>(d.ntrip)};
    }

    int
    execute(const hal::WorkCommand& command) override {
        if (command.target != hal::WorkTarget::Rover && command.target != hal::WorkTarget::Base
            && command.target != hal::WorkTarget::Ntrip) {
            return RES_PARAM_ERROR;
        }
        DataProc::DeviceRequest request;
        request.command = DataProc::DeviceCommand::ApplyWork;
        request.target = static_cast<DataProc::WorkTarget>(command.target);
        request.radio_protocol = command.protocol;
        request.radio_channel = command.channel;
        return device_.execute_device(request) ? RES_OK : RES_PARAM_ERROR;
    }

  private:
    BusinessDevice& device_;
};

/** @brief 录制请求复用旧设备的设置能力、协议映射与确认规则。 */
class RecorderDevice final : public hal::WriteObject<hal::RecorderState, hal::RecorderCommand> {
  public:
    explicit RecorderDevice(BusinessDevice& device) : WriteObject(hal::names::Recorder), device_(device) {}

    hal::RecorderState
    state() override {
        const auto d = device_.snapshot(lv_tick_get());
        return {d.record_valid,    d.recording,
                d.record_pending,  d.record_type,
                d.record_interval, d.record_name,
                d.record_space_mb, systemInfo.host.record.record_status == On_Off_Status_ON};
    }

    int
    execute(const hal::RecorderCommand& command) override {
        if (command.action != hal::RecordAction::Start && command.action != hal::RecordAction::Stop) {
            return RES_PARAM_ERROR;
        }
        DataProc::DeviceRequest request;
        request.command = command.action == hal::RecordAction::Start ? DataProc::DeviceCommand::StartRecord
                                                                     : DataProc::DeviceCommand::StopRecord;
        request.record_type = command.type;
        request.record_interval = command.interval;
        return device_.execute_device(request) ? RES_OK : RES_PARAM_ERROR;
    }

  private:
    BusinessDevice& device_;
};

/** @brief 沿用原 Wi-Fi 超时状态；写入仅经旧对象的 State::Request_Wifi 路径。 */
class WifiDevice final : public hal::WriteObject<hal::WifiState, hal::WifiCommand> {
  public:
    WifiDevice(BusinessDevice& device, StatusDevice& status)
        : WriteObject(hal::names::WiFi), device_(device), status_(status) {}

    hal::WifiState
    state() override {
        device_.update(lv_tick_get());
        const auto d = device_.snapshot(lv_tick_get());
        return {d.wifi_valid,
                status_.capabilities().wifi_available,
                systemInfo.host.wifi.wifi_status == On_Off_Status_ON,
                device_.wifi_pending(),
                d.wifi_station,
                d.wifi_ssid,
                d.wifi_ip};
    }

    int
    execute(const hal::WifiCommand& command) override {
        if (!status_.capabilities().wifi_available) {
            return RES_UNSUPPORT;
        }
        return device_.request_wifi(command.on) ? RES_OK : RES_UNKNOWN;
    }

  private:
    BusinessDevice& device_;
    StatusDevice& status_;
};

/** @brief 版本与产品能力复用同一取值路径，无产品名判断。 */
class BoardDevice final : public hal::ReadObject<hal::BoardState> {
  public:
    BoardDevice(BusinessDevice& device, StatusDevice& status)
        : ReadObject(hal::names::Board), device_(device), status_(status) {}

    hal::BoardState
    state() override {
        const auto d = device_.snapshot(lv_tick_get());
        return {d.firmware, d.hardware, d.build, d.uptime_s, status_.capabilities()};
    }

  private:
    BusinessDevice& device_;
    StatusDevice& status_;
};

/** @brief 步骤元数据独立于 i18n，实际检查复用 InitSteps 的有界轮询。 */
class InitDevice final : public hal::InitObject {
  public:
    explicit InitDevice(StatusDevice& status) : status_(status) {}

    void
    info(hal::InitInfo& value) override {
        constexpr hal::InitStage stages[] = {hal::InitStage::Configuration, hal::InitStage::BatteryGauge,
                                             hal::InitStage::Charger, hal::InitStage::HostPower,
                                             hal::InitStage::HostLink};
        value.name = status_.initialization_name(value.step);
        value.stage = stages[value.step];
    }

    void
    poll(hal::InitPoll& value) override {
        value.status = static_cast<hal::InitStatus>(
            status_.poll_initialization_step(value.step, value.elapsed_ms, value.detail.data(), value.detail.size()));
    }

  private:
    StatusDevice& status_;
};
} // namespace

DeviceManager&
domain_devices(PowerDevice& power, BusinessDevice& device, StatusDevice& status) {
    static BatteryDevice battery(device);
    static GNSSDevice gnss(device);
    static ClockDevice clock;
    static WorkDevice work(device);
    static RecorderDevice recorder(device);
    static WifiDevice wifi(device, status);
    static BoardDevice board(device, status);
    static InitDevice init(status);
    static DeviceObject* list[] = {&power, &battery, &gnss, &clock, &work, &recorder, &wifi, &board, &init};
    static DeviceManager manager(list, sizeof(list) / sizeof(list[0]));
    return manager;
}
