// SystemLoading 的初始化步骤（system_backend 接口）：检查真实器件状态，失败只显示、不阻塞开机。
#include <cstdio>

#include "Common/SystemBackend.h"
#include "mcu_config.h"

#if !defined(LVGL_SIMULATOR)
#include <Arduino.h>
#endif

namespace {

struct InitStep {
    const char* name;
    i18n::TextId title;
};

constexpr InitStep kInitSteps[] = {
    {"configuration", i18n::TextId::LoadingConfiguration},
    {"battery-gauge", i18n::TextId::CheckingBatteryGauge},
    {"charger", i18n::TextId::CheckingCharger},
    {"host-power", i18n::TextId::PoweringHostBoard},
    {"ui", i18n::TextId::FinalizingStartup},
};
static_assert(sizeof(kInitSteps) / sizeof(kInitSteps[0]) == DataProc::kInitializationStepCount);

} // namespace

const char*
system_backend::initialization_name(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].name : "invalid";
}

i18n::TextId
system_backend::initialization_title(unsigned step) {
    return step < DataProc::kInitializationStepCount ? kInitSteps[step].title : i18n::TextId::Initializing;
}

bool
system_backend::initialize_step(unsigned step, char* detail, std::size_t size) {
    if (!detail || size == 0) {
        return false;
    }
    switch (step) {
        case 0:
            std::snprintf(detail, size, "%s FW %s HW %s", PRODUCT_NAME, systemInfo.software_version,
                          systemInfo.hardware_version);
            return systemInfo.software_version[0] != '\0';
        case 1:
            std::snprintf(detail, size, "BQ40Z50 %s", systemInfo.online_device.bq40z50 ? "online" : "not detected");
            return systemInfo.online_device.bq40z50;
        case 2:
            // V1.5 主板带 MP2762A；V1.3 没有该芯片，由 GPIO 控制充电，属正常配置。
            // 注意：V1.5 上 MP2762A 失效时与 V1.3 无法区分（版本本身由 I2C 扫描推断）。
            if (systemInfo.online_device.mp2762) {
                std::snprintf(detail, size, "MP2762A online (board %s)", systemInfo.hardware_version);
            } else {
                std::snprintf(detail, size, "GPIO charge control (board %s)", systemInfo.hardware_version);
            }
            return true;
        case 3: {
            // 主控在开机时才上电、需数十秒启动，此处只确认供电已打开，不检查通信。
#if defined(LVGL_SIMULATOR)
            const bool powered = true;
#else
            const bool powered = digitalRead(POWER_CONTROL_PIN) == HIGH;
#endif
            std::snprintf(detail, size, "host board power %s", powered ? "on" : "off");
            return powered;
        }
        case 4: std::snprintf(detail, size, "working UI prepared"); return true;
        default: std::snprintf(detail, size, "invalid step %u", step); return false;
    }
}
