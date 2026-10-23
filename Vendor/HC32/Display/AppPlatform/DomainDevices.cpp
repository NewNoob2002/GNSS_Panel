#include "DomainDevices.h"
#include "BusinessDevice.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_Init.h"
#include "PowerDevice.h"
#include "StatusDevice.h"
#include "lvgl.h"

namespace {
class BatteryDevice final : public hal::ReadObject<hal::BatteryState> {
  public:
    explicit BatteryDevice(BusinessDevice& device) : ReadObject(hal::names::Battery), device_(device) {}

    hal::BatteryState
    state() override {
        return device_.battery();
    }

  private:
    BusinessDevice& device_;
};

class GNSSDevice final : public hal::ReadObject<hal::GNSSState> {
  public:
    explicit GNSSDevice(BusinessDevice& device) : ReadObject(hal::names::GNSS), device_(device) {}

    hal::GNSSState
    state() override {
        return device_.gnss();
    }

  private:
    BusinessDevice& device_;
};

/** @brief 运行时钟不访问应用 RAM 日期或 UI 文本。 */
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

/** @brief 协议映射与请求写入只由 BusinessDevice 持有。 */
class WorkDevice final : public hal::WriteObject<hal::WorkState, hal::WorkCommand> {
  public:
    explicit WorkDevice(BusinessDevice& device) : WriteObject(hal::names::Work), device_(device) {}

    hal::WorkState
    state() override {
        return device_.work();
    }

    int
    execute(const hal::WorkCommand& command) override {
        return device_.execute(command) ? RES_OK : RES_PARAM_ERROR;
    }

  private:
    BusinessDevice& device_;
};

class RecorderDevice final : public hal::WriteObject<hal::RecorderState, hal::RecorderCommand> {
  public:
    explicit RecorderDevice(BusinessDevice& device) : WriteObject(hal::names::Recorder), device_(device) {}

    hal::RecorderState
    state() override {
        return device_.recorder();
    }

    int
    execute(const hal::RecorderCommand& command) override {
        return device_.execute(command) ? RES_OK : RES_PARAM_ERROR;
    }

  private:
    BusinessDevice& device_;
};

/** @brief Wi-Fi 回报与超时沿用平台策略。 */
class WifiDevice final : public hal::WriteObject<hal::WifiState, hal::WifiCommand> {
  public:
    explicit WifiDevice(BusinessDevice& device) : WriteObject(hal::names::WiFi), device_(device) {}

    hal::WifiState
    state() override {
        device_.update(lv_tick_get());
        return device_.wifi();
    }

    int
    execute(const hal::WifiCommand& command) override {
        if (!device_.wifi().available) {
            return RES_UNSUPPORT;
        }
        return device_.request_wifi(command.on) ? RES_OK : RES_UNKNOWN;
    }

  private:
    BusinessDevice& device_;
};

class BoardDevice final : public hal::ReadObject<hal::BoardState> {
  public:
    BoardDevice(BusinessDevice& device, StatusDevice& status)
        : ReadObject(hal::names::Board), device_(device), status_(status) {}

    hal::BoardState
    state() override {
        auto value = device_.board(lv_tick_get());
        value.capabilities = status_.capabilities();
        return value;
    }

  private:
    BusinessDevice& device_;
    StatusDevice& status_;
};

/** @brief 元数据不依赖 i18n，实际检查复用 InitSteps 的有界轮询。 */
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
        value.status =
            status_.poll_initialization_step(value.step, value.elapsed_ms, value.detail.data(), value.detail.size());
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
    static WifiDevice wifi(device);
    static BoardDevice board(device, status);
    static InitDevice init(status);
    static DeviceObject* list[] = {&power, &battery, &gnss, &clock, &work, &recorder, &wifi, &board, &init};
    static DeviceManager manager(list, sizeof(list) / sizeof(list[0]));
    return manager;
}
