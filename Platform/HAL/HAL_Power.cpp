#include "Arduino.h"
#include "Charger.h"
#include "HAL.h"
#include "PowerPolicy.h"
#include "SystemState.h"
#include "product_config.h"

// 电源时序：开机判定、关机流程、关机驻留与原地开机、低电量关机。只依赖 GPIO、时基与充电控制的
// 待机/开机切换，固件、主机测试与 PC 模拟器共用同一份（电池与充电监控见 HAL_Battery.cpp）。

static PowerPolicy power_policy;
#if !PRODUCT_HAS_DISPLAY
static bool power_key_down;
static uint32_t power_key_started_ms;
#endif

#define FORCE_SHUTDOWN() (State::Power().Force_ShutDown)

static bool POWER_GET_REASON = false;
en_flag_status_t softwareReset = RESET;

// 产品电源扩展点：默认无动作，X1 用于电量补偿。
__attribute__((weak)) void
HAL::Power_ProductInit() {}

void
HAL::Power_Init() {
    softwareReset = RMU_GetStatus(RMU_FLAG_SW);
    // Type-C 独立供电给 MCU，确认开机前保持 Linux 主板断电；电池供电时仅按住 Power 键维持 MCU，
    // 提前松开不得锁存电源。仅软件复位立即保持供电，供 Boot 保留 SWRF 和 app_fatal 恢复使用。
    pinMode(POWER_CONTROL_PIN, OUTPUT, softwareReset == SET ? HIGH : LOW);
    RMU_ClearStatus();
    if (softwareReset == SET) {
        CORE_DEBUG_PRINTF("Power: Software Reset");
    } else {
        CORE_DEBUG_PRINTF("Power: Waiting Keep Press...");
    }
    pinMode(POWER_LED_PIN, OUTPUT);
    pinMode(FUNCTION_LED_PIN, OUTPUT);
    pinMode(CHARGE_LED_PIN, OUTPUT);

    pinMode(WATCHDOG_FEED_PIN, OUTPUT);
    Power_ProductInit();
}

// 开机后恢复电源保持与指示灯；冷启动和关机驻留恢复共用。
static void
power_on_outputs() {
    digitalWrite(POWER_LED_PIN, HIGH);
    digitalWrite(FUNCTION_LED_PIN, HIGH);
    digitalWrite(POWER_CONTROL_PIN, HIGH);
    HAL::Charger_Get().power_on();
}

static void
clear_shutdown_requests() {
    Power_Monitor_t& power = State::Power();
    power.ShutdownReq = false;
    power.ShutdownEnsure = false;
    power.Force_ShutDown = false;
    power.LinuxPowerOff = false;
    power.LowBatteryPowerOff = false;
    power.poweroff_flag = 0;
    power.reset_flag = 0;
}

// 断电延时到期后，仅外部供电时 MCU 仍在运行：进入关机驻留，继续充电并等待开机确认。
// 清除关机请求，界面据 ShutdownComplete 回到待机页。不能软件复位：Boot 保留 SWRF，
// 复位会使设备直接开机，且 Boot 升级窗口会额外停留约 1 s。
static void
power_enter_standby(uint32_t now) {
    HAL::Power_Shutdown(true);
    digitalWrite(POWER_LED_PIN, LOW);
    digitalWrite(FUNCTION_LED_PIN, LOW);
    // 与冷启动等待开机时一致（见 Power_OnCheck）。
    HAL::Charger_Get().enter_standby();
    clear_shutdown_requests();
    State::Power().panel_power_on = false;
#if !PRODUCT_HAS_DISPLAY
    power_key_down = digitalRead(POWER_KEY_PIN) == HIGH;
    power_key_started_ms = now;
#else
    (void)now;
#endif
}

// 驻留中确认开机：带屏产品由 Startup 请求，X1 由 HAL 长按判定；状态与冷启动一致。
// 主控已断电重启，清除其上报的数据；电池采样、器件在线状态与版本信息保留。
static void
power_resume_from_standby(uint32_t now) {
    power_policy = PowerPolicy{};
    POWER_GET_REASON = false;
    // 驻留期间按键可能再次置位强制关机等请求，开机前一并清除。
    clear_shutdown_requests();
    State::Reset_HostData();
    Power_Monitor_t& power = State::Power();
    power.ShutdownGoing = false;
    power.ShutdownComplete = false;
    power.ExternalPower = 0;
    power.ExternalPowerChange = 0;
    power.panel_power_on = true;
    power.pannel_power_on_time = now;
    CORE_DEBUG_PRINTF("Power: resume from standby");
    power_on_outputs();
}

/**
 * @brief Accept a Startup hold on the LVGL thread, or resume from shutdown standby.
 * @return False while shutdown is pending or in progress; true if accepted or already on.
 * Cold boot defers rail activation to Power_OnCheck; standby restores outputs immediately.
 */
bool
HAL::Power_RequestOn() {
    if (power_policy.state == PowerPolicy::Off) {
        power_resume_from_standby(millis());
        return true;
    }
    const Power_Monitor_t& power = systemInfo.power;
    if (power_policy.state != PowerPolicy::Running || power.ShutdownGoing || power.ShutdownReq || power.ShutdownEnsure
        || power.LowBatteryPowerOff || power.Force_ShutDown || power.LinuxPowerOff) {
        return false;
    }
    if (!power.panel_power_on) {
        State::Power().panel_power_on = true;
        State::Power().pannel_power_on_time = millis();
    }
    return true;
}

void
HAL::Power_OnCheck(void (*idle)()) {
    Charger_Get().enter_standby();
    while (true) {
        HAL::HAL_Update();
        // 带屏产品在等待开机时继续刷新界面；无屏产品不传入。
        if (idle) {
            idle();
        }
        if (State::Power().panel_power_on) {
            CORE_DEBUG_PRINTF("MCU PowerDone");
            break;
        }
        if (softwareReset) {
            CORE_DEBUG_PRINTF("softwareRst PowerDone");
            break;
        }
        __WFI();
    }
    CORE_DEBUG_PRINTF("Power: Done");
    power_on_outputs();
}

void
HAL::Power_Shutdown(bool en) {
    if (!en) {
        CORE_DEBUG_PRINTF("Shutdown Req");
        State::Power().ShutdownReq = true;
    } else {
        CORE_DEBUG_PRINTF("Shutdown Done");
        State::Power().ShutdownGoing = false;
        State::Power().ShutdownComplete = true;
        digitalWrite(POWER_CONTROL_PIN, LOW);
    }
}

void
HAL::Power_PowerOffMonitor() {
    const uint32_t now = millis();
    if (!State::Power().panel_power_on) {
#if PRODUCT_HAS_DISPLAY
        // 带屏产品只由 Startup 的完整长按请求开机；软件复位保留直接开机路径。
        if (softwareReset == SET && power_policy.state != PowerPolicy::Off) {
            State::Power().panel_power_on = true;
            State::Power().pannel_power_on_time = now;
        }
#else
        const bool pressed = digitalRead(POWER_KEY_PIN) == HIGH;
        if (pressed && !power_key_down) {
            power_key_started_ms = now;
        }
        power_key_down = pressed;
        const bool long_press = pressed && now - power_key_started_ms >= 2000U;
        if (power_policy.state == PowerPolicy::Off) {
            if (long_press) {
                power_resume_from_standby(now);
            }
        } else if (softwareReset == SET || long_press) {
            State::Power().panel_power_on = true;
            State::Power().pannel_power_on_time = now;
        }
#endif
    }
    const bool requested = State::Power().ShutdownReq || State::Power().LowBatteryPowerOff
                           || State::Power().Force_ShutDown || State::Power().LinuxPowerOff
                           || State::Power().ShutdownEnsure;
    if (power_policy.state == PowerPolicy::Running && requested) {
        digitalWrite(CHARGE_LED_PIN, LOW);
        digitalWrite(POWER_LED_PIN, HIGH);
        digitalWrite(FUNCTION_LED_PIN, HIGH);
        State::Power().poweroff_flag = 1;
        State::Power().ShutdownGoing = true;
        State::Power().ShutdownReq = false;
    }
    // 低电量：停止录制；持续 3 min 后关机。
    const Power_Monitor_t& power = systemInfo.power;
    const BatteryInfo_t& battery = power.batteryInfo;
    const bool low = power.panel_power_on && battery.sample_valid && battery.Percent <= 9U
                     && battery.chargeStatus == notCharge && !power.ExternalPower;
    if (low) {
        State::Request_RecordStop();
    }
    if (power_policy.LowBatteryExpired(now, low)) {
        State::Power().LowBatteryPowerOff = true;
    }
    if (power_policy.UpdateShutdown(now, requested, State::Power().ShutdownEnsure, FORCE_SHUTDOWN())) {
        power_enter_standby(now);
    }
    if ((State::Power().ShutdownEnsure) || FORCE_SHUTDOWN() || State::Power().LowBatteryPowerOff) {
        if (!POWER_GET_REASON) {
            POWER_GET_REASON = true;
            CORE_DEBUG_PRINTF("POWER OFF .................Power Off Cause: %s", Power_GetPowerOffCause());
        }
    }
}

bool
HAL::Power_ShutdownSoftReset() {
    if (State::Power().reset_flag) {
        State::Power().reset_flag = false;
        return true;
    }
    return false;
}

const char*
HAL::Power_GetPowerOffCause() {
    if (State::Power().LinuxPowerOff) {
        return "Shutdown Board";
    } else if (State::Power().LowBatteryPowerOff) {
        return "Shutdown LowBattery";
    } else if (FORCE_SHUTDOWN()) {
        return "Shutdown Force";
    }
    return "Shutdown PushKey";
}