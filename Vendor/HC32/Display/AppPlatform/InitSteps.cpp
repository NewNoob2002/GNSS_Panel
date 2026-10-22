// SystemLoading 的初始化步骤（system_backend 接口）：检查真实器件状态，非阻塞轮询，失败只显示、不阻塞开机。
// 每步耗时即实际检查时间；I2C 探测有 WIRE_TIMEOUT 上限，等待主控有 kHostLinkTimeoutMs 上限。
#include <cstdio>

#include <HAL.h>
#include "StatusDevice.h"
#include "mcu_config.h"

#include "GpioPin.h"

namespace {

constexpr GpioPin powerControl(POWER_CONTROL_PIN);

struct InitStep {
    const char* name;
    i18n::TextId title;
};

constexpr InitStep kInitSteps[] = {
    {"configuration", i18n::TextId::LoadingConfiguration},
    {"battery-gauge", i18n::TextId::CheckingBatteryGauge},
    {"charger", i18n::TextId::CheckingCharger},
    {"host-power", i18n::TextId::PoweringHostBoard},
    {"host-link", i18n::TextId::CheckingHostLink},
};
static_assert(sizeof(kInitSteps) / sizeof(kInitSteps[0]) == DataProc::kInitializationStepCount);

constexpr uint8_t kBatteryGaugeAddress = 0x0B; // BQ40Z50
constexpr uint8_t kChargerAddress = 0x5C;      // MP2762A（仅 V1.5 主板）
// 主控上电后需数十秒启动；超时只标红此步并进入首页，关机/强制关机由 HAL 独立处理，不受加载页影响。
constexpr std::uint32_t kHostLinkTimeoutMs = 30000;

using Status = DataProc::InitializationStatus;

Status
result(bool ok) {
    return ok ? Status::Ok : Status::Failed;
}

} // namespace

const char*
StatusDevice::initialization_name(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].name : "invalid";
}

i18n::TextId
StatusDevice::initialization_title(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].title : i18n::TextId::Initializing;
}

DataProc::InitializationStatus
StatusDevice::poll_initialization_step(unsigned step, std::uint32_t elapsed_ms, char* detail, std::size_t size) {
    if (!detail || size == 0) {
        return Status::Failed;
    }
    switch (step) {
        case 0:
            // 只检查编译期写入的产品与版本信息。
            std::snprintf(detail, size, "%s FW %s HW %s", PRODUCT_NAME, systemInfo.board.software_version,
                          systemInfo.board.hardware_version);
            return result(systemInfo.board.software_version[0] != '\0');
        case 1: {
            // 在此重新探测，而非沿用开机扫描结果。
            const bool online = HAL::I2C_ProbeDevice(kBatteryGaugeAddress);
            std::snprintf(detail, size, "BQ40Z50 %s", online ? "online" : "not responding");
            return result(online);
        }
        case 2: {
            // 主板版本由开机扫描推断：扫描到 MP2762A 即 V1.5。V1.5 上现在不应答判为失败；
            // V1.3 没有该芯片，由 GPIO 控制充电，属正常配置。
            const bool v15 = systemInfo.board.online_device.mp2762;
            const bool online = HAL::I2C_ProbeDevice(kChargerAddress);
            if (online) {
                std::snprintf(detail, size, "MP2762A online (board %s)", systemInfo.board.hardware_version);
                return Status::Ok;
            }
            std::snprintf(detail, size, v15 ? "MP2762A not responding (board %s)" : "GPIO charge control (board %s)",
                          systemInfo.board.hardware_version);
            return result(!v15);
        }
        case 3: {
            const bool powered = powerControl.read();
            std::snprintf(detail, size, "host board power %s", powered ? "on" : "off");
            return result(powered);
        }
        case 4:
            // 收到主控第一帧有效协议帧即视为通信正常（panel_protocol 置 host.online，开机时清零）。
            if (systemInfo.host.online) {
                std::snprintf(detail, size, "host link up after %lu ms", static_cast<unsigned long>(elapsed_ms));
                return Status::Ok;
            }
            if (elapsed_ms < kHostLinkTimeoutMs) {
                return Status::Pending;
            }
            std::snprintf(detail, size, "no host frame within %lu s",
                          static_cast<unsigned long>(kHostLinkTimeoutMs / 1000U));
            return Status::Failed;
        default: std::snprintf(detail, size, "invalid step %u", step); return Status::Failed;
    }
}
