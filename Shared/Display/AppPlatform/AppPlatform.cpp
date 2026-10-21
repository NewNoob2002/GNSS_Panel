// HC32 平台层入口：创建共享应用，把 HAL/systemInfo 接到它（各方向的转换见 *Bridge），
// 并实现 system_backend 中与电源、业务相关的接口（初始化步骤见 InitSteps.cpp）。
#include "AppPlatform.h"

#include <cstdio>

#include "Common/SystemBackend.h"
#include "DeviceBridge.h"
#include "HAL.h"
#include "App.h"
#include "PowerBridge.h"
#include "StatusBridge.h"
#include "Utils/Log/Log.h"
#include "mcu_config.h"

namespace {

/** 平台侧状态随应用实例重建（主机测试与模拟器会多次创建应用）。 */
struct Bridges {
    StatusBridge status;
    PowerBridge power;
    DeviceBridge device;
};

App* app = nullptr;
Bridges bridges;

uint32_t
now_ms() {
    return lv_tick_get();
}

void
log_sink(AppLogLevel level, const char* module, const char* message, void*) {
    static const char* const names[] = {"D", "I", "W", "E"};
#if defined(LVGL_SIMULATOR)
    std::printf("[%s][%s] %s\n", names[level], module, message);
#else
    // Release 构建中 CORE_DEBUG_PRINTF 为空，日志不占串口。
    CORE_DEBUG_PRINTF("[%s][%s] %s", names[level], module, message);
    (void)names;
    (void)level;
    (void)module;
    (void)message;
#endif
}

} // namespace

// ---- system_backend：HC32 实现 --------------------------------------------------------------

system_backend::Capabilities
system_backend::capabilities() {
    return {.simulated = false,
            .reboot_available = false, // HC32 产品不提供重启
            .wifi_available = PRODUCT_SHUTDOWN_WIFI_BUTTON != 0,
            .star_map = PRODUCT_PAGE_STAR_MAP != 0,
            .record_settings = PRODUCT_RECORD_SETTINGS != 0};
}

// 在 LVGL 线程将 Startup 完整长按请求转交 HAL 电源策略。
bool
system_backend::request_power_on() {
    // Startup 完整长按后请求 HAL 开机；成功时 SystemService 同步发布 powered_on 并进入 SystemLoading。
    return HAL::Power_RequestOn();
}

bool
system_backend::execute_power(DataProc::PowerAction action) {
    if (action != DataProc::PowerAction::Off) {
        return false;
    }
    // 只发出关机请求；断电时序（等待主控确认、延时）由 HAL 电源策略完成，结束后上报待机。
    HAL::Power_Shutdown(false);
    return true;
}

bool
system_backend::request_wifi(bool on) {
    return bridges.device.request_wifi(on, now_ms());
}

DataProc::DeviceSnapshot
system_backend::initial_device() {
    return bridges.device.snapshot(now_ms());
}

bool
system_backend::execute_device(const DataProc::DeviceRequest& request) {
    return bridges.device.execute(request, now_ms());
}

#if !defined(LVGL_SIMULATOR)
// 单片机上无法 abort：记录原因后软件复位。Boot 保留 SWRF，复位后直接开机。
void
app_fatal(const char* module, const char* reason) {
    app_log_write(APP_LOG_ERROR, module ? module : "App", "fatal: %s; resetting", reason ? reason : "unknown");
    HAL_NVIC_SystemReset();
    while (true) {}
}
#endif

// ---- 入口 ------------------------------------------------------------------------------------

void
App_Init() {
    app_log_set_sink(log_sink, nullptr);
    bridges = Bridges{};
    app = new App;
    if (!app->init({.battery_warning = PRODUCT_PAGE_BATTERY_WARNING != 0})) {
        app_fatal("App", "shared LVGL application initialization failed");
    }
}

void
App_Update() {
    if (!app) {
        return;
    }
    const uint32_t now = now_ms();
#if !defined(LVGL_SIMULATOR)
    // 两键电平按 RK3506 语义处理（Function 单击下一项/双击返回，Power 单击选择/双击提交/长按开机）。
    app->on_key(App::Key::Power, HAL::Key_PowerPressed());
    app->on_key(App::Key::Function, HAL::Key_FunctionPressed());
    app->poll_keys(now);
#endif
    app->update();
    bridges.power.update();
    bridges.device.update(now);
    bridges.status.update(*app, now);
}

void
App_Deinit() {
    delete app;
    app = nullptr;
}

App*
App_Get() {
    return app;
}
