#include "Arduino.h"
#include "Charger.h"
#include "HAL.h"
#include "PowerPolicy.h"
#include "ots.h"

static PowerPolicy power_policy;
static bool power_key_down;
static uint32_t power_key_started_ms;

#define FORCE_SHUTDOWN() (systemInfo.powerMonitor.Force_ShutDown)

static bool POWER_GET_REASON = false;
en_flag_status_t softwareReset = RESET;

// 产品电源扩展点：默认无动作，X1 用于电量补偿。
__attribute__((weak)) void
HAL::Power_ProductInit() {}

__attribute__((weak)) void
HAL::Power_ProductUpdate() {}

__attribute__((weak)) uint16_t
HAL::Power_AdjustPercent(uint16_t actualPercent) {
    return actualPercent;
}

void
ots_callback(void) {
    systemInfo.powerMonitor.batteryInfo.fOtsTemp = OTS_CalculateTemp();
}

void
HAL::Power_Init() {
    softwareReset = RMU_GetStatus(RMU_FLAG_SW);
    pinMode(POWER_CONTROL_PIN, OUTPUT, HIGH);
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
    OtsInitConfig(ots_callback);
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
    Power_Monitor_t& power = systemInfo.powerMonitor;
    power.ShutdownReq = false;
    power.ShutdownEnsure = false;
    power.Force_ShutDown = false;
    power.LinuxPowerOff = false;
    power.LowBatteryPowerOff = false;
    power.poweroff_flag = 0;
    power.reset_flag = 0;
}

// 断电延时到期后，仅外部供电时 MCU 仍在运行：进入关机驻留，继续充电并等待电源键开机。
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
    systemInfo.powerMonitor.panel_power_on = false;
    power_key_down = digitalRead(POWER_KEY_PIN) == HIGH;
    power_key_started_ms = now;
}

// 驻留中长按开机：状态与冷启动一致。主控已断电重启，清除其上报的数据；
// 电池采样、器件在线状态与版本信息保留。
static void
power_resume_from_standby(uint32_t now) {
    power_policy = PowerPolicy{};
    POWER_GET_REASON = false;
    // 驻留期间按键可能再次置位强制关机等请求，开机前一并清除。
    clear_shutdown_requests();
    systemInfo.online_device.eg25_board = false;
    systemInfo.eg25_overtime = false;
    systemInfo.wifiInfo = {};
    systemInfo.recordInfo = {};
    systemInfo.positionInfo = {};
    systemInfo.starMapInfo = {};
    systemInfo.radioInfo = {};
    systemInfo.ntripInfo = {};
    systemInfo.work_mode = rover_mode;
    Power_Monitor_t& power = systemInfo.powerMonitor;
    power.ShutdownGoing = false;
    power.ShutdownComplete = false;
    power.ExternalPower = 0;
    power.ExternalPowerChange = 0;
    power.panel_power_on = true;
    power.pannel_power_on_time = now;
    CORE_DEBUG_PRINTF("Power: resume from standby");
    power_on_outputs();
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
        if (systemInfo.powerMonitor.panel_power_on) {
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
        systemInfo.powerMonitor.ShutdownReq = true;
    } else {
        CORE_DEBUG_PRINTF("Shutdown Done");
        systemInfo.powerMonitor.ShutdownGoing = false;
        systemInfo.powerMonitor.ShutdownComplete = true;
        digitalWrite(POWER_CONTROL_PIN, LOW);
    }
}

void
HAL::Power_PowerOffMonitor() {
    const uint32_t now = millis();
    if (!systemInfo.powerMonitor.panel_power_on) {
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
            systemInfo.powerMonitor.panel_power_on = true;
            systemInfo.powerMonitor.pannel_power_on_time = now;
        }
    }
    const bool requested = systemInfo.powerMonitor.ShutdownReq || systemInfo.powerMonitor.LowBatteryPowerOff
                           || systemInfo.powerMonitor.Force_ShutDown || systemInfo.powerMonitor.LinuxPowerOff
                           || systemInfo.powerMonitor.ShutdownEnsure;
    if (power_policy.state == PowerPolicy::Running && requested) {
        digitalWrite(CHARGE_LED_PIN, LOW);
        digitalWrite(POWER_LED_PIN, HIGH);
        digitalWrite(FUNCTION_LED_PIN, HIGH);
        systemInfo.powerMonitor.poweroff_flag = 1;
        systemInfo.powerMonitor.ShutdownGoing = true;
        systemInfo.powerMonitor.ShutdownReq = false;
    }
    if (power_policy.UpdateShutdown(now, requested, systemInfo.powerMonitor.ShutdownEnsure, FORCE_SHUTDOWN())) {
        power_enter_standby(now);
    }
    if ((systemInfo.powerMonitor.ShutdownEnsure) || FORCE_SHUTDOWN() || systemInfo.powerMonitor.LowBatteryPowerOff) {
        if (!POWER_GET_REASON) {
            POWER_GET_REASON = true;
            CORE_DEBUG_PRINTF("POWER OFF .................Power Off Cause: %s", Power_GetPowerOffCause());
        }
    }
}

bool
HAL::Power_ShutdownSoftReset() {
    if (systemInfo.powerMonitor.reset_flag) {
        systemInfo.powerMonitor.reset_flag = false;
        return true;
    }
    return false;
}

void
HAL::Power_Update() {
    WatchDog_Feed();
    OtsStart();
    Power_ProductUpdate();
    Power_GetInfo(&systemInfo.powerMonitor);
}

void
HAL::Power_GetInfo(Power_Monitor_t* info) {
    if (info == nullptr) {
        return;
    }
    if (millis() - systemInfo.powerMonitor.BatteryLastHandleTime >= 3000) {
        systemInfo.powerMonitor.BatteryLastHandleTime = millis();
        if (!systemInfo.online_device.bq40z50) {
            digitalToggle(POWER_LED_PIN);
        } else {
            Battery_Sample(info->batteryInfo);
        }
    }
    if (info->batteryInfo.sample_valid && millis() - info->batteryInfo.last_sample_ms > 10000U) {
        info->batteryInfo.sample_valid = false;
    }
    info->batteryInfo.ChargerDetect = digitalRead(CHARGER_CTRL_PIN);
    Charger_Protect(info->batteryInfo);
    Charger_Get().update(info->batteryInfo);
    const bool low = info->panel_power_on && info->batteryInfo.sample_valid && info->batteryInfo.Percent <= 9U
                     && info->batteryInfo.chargeStatus == notCharge && !info->ExternalPower;
    if (low && systemInfo.recordInfo.record_status != On_Off_Status_OFF) {
        systemInfo.recordInfo.record_status = On_Off_Status_OFF;
        systemInfo.recordInfo.record_op = 1;
    }
    if (power_policy.LowBatteryExpired(millis(), low)) {
        info->LowBatteryPowerOff = true;
    }
}

void
HAL::WatchDog_Feed() {
    if (millis() - systemInfo.powerMonitor.WatchDogLastFeedTime >= WATCHDOG_FEED_TIME) {
        systemInfo.powerMonitor.WatchDogLastFeedTime = millis();
        digitalToggle(WATCHDOG_FEED_PIN);
    }
}

const char*
HAL::Power_GetPowerOffCause() {
    if (systemInfo.powerMonitor.LinuxPowerOff) {
        return "Shutdown Board";
    } else if (systemInfo.powerMonitor.LowBatteryPowerOff) {
        return "Shutdown LowBattery";
    } else if (FORCE_SHUTDOWN()) {
        return "Shutdown Force";
    }
    return "Shutdown PushKey";
}