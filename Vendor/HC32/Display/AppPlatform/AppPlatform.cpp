// HC32 平台入口：通过静态 DeviceManager 连接九个细设备，保留主循环及 HAL 时序。
#include "AppPlatform.h"

#include <HAL.h>
#include "App.h"
#include "BusinessDevice.h"
#include "DomainDevices.h"
#include "Log.h"
#include "PowerDevice.h"
#include "StatusDevice.h"
#include "HAL_Manager.h"
#include "mcu_config.h"
#if defined(HC32_LOG_ENABLED)
#include "AppLogSink.h"
#endif

namespace {

/** 平台侧状态随应用实例重建（主机测试与模拟器会多次创建应用）。 */
struct Devices {
    StatusDevice status;
    PowerDevice power;
    BusinessDevice device;
};

App* app = nullptr;
Devices objects;

uint32_t
now_ms() {
    return lv_tick_get();
}

} // namespace

/** 平台持有九个细设备，初始化不访问硬件。 */
DeviceManager&
hal::manager() {
    auto& manager = domain_devices(objects.power, objects.device, objects.status);
    // App_Init 可重建对象，因此重新标记设备可用；onInit 无硬件副作用。
    manager.init();
    return manager;
}

hal::Capabilities
StatusDevice::capabilities() {
    return {.simulated = false,
            .reboot_available = false, // HC32 产品不提供重启
            .wifi_available = PRODUCT_SHUTDOWN_WIFI_BUTTON != 0,
            .star_map = PRODUCT_PAGE_STAR_MAP != 0,
            .record_settings = PRODUCT_RECORD_SETTINGS != 0};
}

#if !defined(LVGL_SIMULATOR)
// 单片机上无法 abort：记录原因后软件复位。Boot 保留 SWRF，复位后直接开机。
void
app_fatal(const char* module, const char* reason) {
#if defined(HC32_LOG_ENABLED)
    app_log_write(APP_LOG_ERROR, module ? module : "App", "fatal: %s; resetting", reason ? reason : "unknown");
#else
    (void)module;
    (void)reason;
#endif
    HAL_NVIC_SystemReset();
    while (true) {}
}
#endif

// ---- 入口 ------------------------------------------------------------------------------------

void
App_Init() {
    // 模拟器沿用 main() 已安装的 host::log（spdlog）sink，这里不覆盖。
#if !defined(LVGL_SIMULATOR) && defined(HC32_LOG_ENABLED)
    app_log_set_sink(hc32_app_log_sink, nullptr);
#endif
    objects = Devices{};
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
    objects.device.update(now);
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
