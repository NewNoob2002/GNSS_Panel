#include "DomainDevices.h"
#include "Def/HAL_Battery.h"
#include "Def/HAL_Board.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Init.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Work.h"
#include "DemoStatus.h"

namespace {
using Business = system_backend::BusinessObject;
using Status = system_backend::StatusObject;

// 两套旧模拟源互相矛盾，3.7c 统一为单一模拟模型；本步业务字段取 Device，
// Battery.level_percent/temperature_valid 与 Clock 取 Status，保持旧数据路径和画面不变。
/** @brief 两个旧模拟源分别提供业务电量计值和状态栏电量。 */
class DemoBatteryDevice final : public hal::ReadObject<hal::BatteryState> {
  public:
    DemoBatteryDevice(Business& device, Status& status)
        : ReadObject(hal::names::Battery), device_(device), status_(status) {}

    hal::BatteryState
    state() override {
        const auto d = device_.device_report().snapshot;
        const auto s = status_.status_snapshot();
        return {d.power_valid,
                s.battery_percent,
                d.battery_percent,
                d.battery_voltage,
                s.battery_temperature_valid,
                d.battery_temperature,
                static_cast<hal::ChargeState>(d.charge),
                d.external_power};
    }

  private:
    Business& device_;
    Status& status_;
};

/** @brief Demo 的有效位置为单点定位，不从 UI 的 DEMO 文本反推定位状态。 */
class DemoGNSSDevice final : public hal::ReadObject<hal::GNSSState> {
  public:
    explicit DemoGNSSDevice(Business& device) : ReadObject(hal::names::GNSS), device_(device) {}

    hal::GNSSState
    state() override {
        const auto d = device_.device_report().snapshot;
        return {d.position_valid,     d.latitude,
                d.longitude,          d.altitude,
                d.satellites_valid,   d.satellites_used,
                d.satellites_tracked, d.constellations_valid,
                d.constellations,     d.position_valid ? hal::FixStatus::Single : hal::FixStatus::None};
    }

  private:
    Business& device_;
};

/** @brief 时间与旧确定性/随机模拟采样时间完全一致。 */
class DemoClockDevice final : public hal::ReadObject<hal::ClockState> {
  public:
    explicit DemoClockDevice(Status& status) : ReadObject(hal::names::Clock), status_(status) {}

    hal::ClockState
    state() override {
        const auto s = status_.status_snapshot();
        return {s.clock_valid, s.hour, s.minute, s.second, hal::ClockSource::Uptime, DemoStatus::uptime_ms()};
    }

  private:
    Status& status_;
};

/** @brief 工作命令继续修改原 Demo 状态，并复用旧范围校验。 */
class DemoWorkDevice final : public hal::WriteObject<hal::WorkState, hal::WorkCommand> {
  public:
    explicit DemoWorkDevice(Business& device) : WriteObject(hal::names::Work), device_(device) {}

    hal::WorkState
    state() override {
        const auto d = device_.device_report().snapshot;
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
        return hal::write(&device_, request);
    }

  private:
    Business& device_;
};

/** @brief 录制命令继续修改原 Demo 状态。 */
class DemoRecorderDevice final : public hal::WriteObject<hal::RecorderState, hal::RecorderCommand> {
  public:
    explicit DemoRecorderDevice(Business& device) : WriteObject(hal::names::Recorder), device_(device) {}

    hal::RecorderState
    state() override {
        const auto d = device_.device_report().snapshot;
        return {d.record_valid,    d.recording,   d.record_pending, d.record_type,
                d.record_interval, d.record_name, d.record_space_mb};
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
        return hal::write(&device_, request);
    }

  private:
    Business& device_;
};

/** @brief Demo 无 Wi-Fi 操作能力，保留原模拟连接信息。 */
class DemoWifiDevice final : public hal::WriteObject<hal::WifiState, hal::WifiCommand> {
  public:
    DemoWifiDevice(Business& device, Status& status)
        : WriteObject(hal::names::WiFi), device_(device), status_(status) {}

    hal::WifiState
    state() override {
        const auto r = device_.device_report();
        const auto& d = r.snapshot;
        return {
            d.wifi_valid, status_.capabilities().wifi_available, r.wifi_on, r.wifi_pending, d.wifi_station, d.wifi_ssid,
            d.wifi_ip};
    }

    int
    execute(const hal::WifiCommand&) override {
        return RES_UNSUPPORT;
    }

  private:
    Business& device_;
    Status& status_;
};

/** @brief 板卡信息由原 Demo 快照与能力提供。 */
class DemoBoardDevice final : public hal::ReadObject<hal::BoardState> {
  public:
    DemoBoardDevice(Business& device, Status& status)
        : ReadObject(hal::names::Board), device_(device), status_(status) {}

    hal::BoardState
    state() override {
        const auto d = device_.device_report().snapshot;
        return {d.firmware, d.hardware, d.build, d.uptime_s, status_.capabilities()};
    }

  private:
    Business& device_;
    Status& status_;
};

/** @brief 初始化耗时和结果保持不变，步骤语义不依赖本地化 ID。 */
class DemoInitDevice final : public hal::InitObject {
  public:
    explicit DemoInitDevice(Status& status) : status_(status) {}

    void
    info(hal::InitInfo& value) override {
        constexpr hal::InitStage stages[] = {hal::InitStage::Configuration, hal::InitStage::Services,
                                             hal::InitStage::GNSS, hal::InitStage::Network,
                                             hal::InitStage::Finalization};
        value.name = status_.initialization_name(value.step);
        value.stage = stages[value.step];
    }

    void
    poll(hal::InitPoll& value) override {
        value.status = static_cast<hal::InitStatus>(
            status_.poll_initialization_step(value.step, value.elapsed_ms, value.detail.data(), value.detail.size()));
    }

  private:
    Status& status_;
};
} // namespace

DeviceManager&
demo_domain_devices(system_backend::PowerObject& power, Business& device, Status& status) {
    static DemoBatteryDevice battery(device, status);
    static DemoGNSSDevice gnss(device);
    static DemoClockDevice clock(status);
    static DemoWorkDevice work(device);
    static DemoRecorderDevice recorder(device);
    static DemoWifiDevice wifi(device, status);
    static DemoBoardDevice board(device, status);
    static DemoInitDevice init(status);
    static DeviceObject* list[] = {&power, &device,   &status, &battery, &gnss, &clock,
                                   &work,  &recorder, &wifi,   &board,   &init};
    static DeviceManager manager(list, sizeof(list) / sizeof(list[0]));
    return manager;
}
