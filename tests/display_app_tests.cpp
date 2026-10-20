// HC32 带屏产品：共享 LVGL 应用 + AppPlatform（固件路径）的主机回归。
// 使用固件 lv_conf.h（96 KB LVGL 内存池）并让 C++ new 走 LVGL 池（lvgl_new.cpp），检查内存峰值余量；
// 主机指针为 8 字节，测得峰值高于 32 位单片机，结论偏保守。
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppPlatform.h"
#include "Arduino.h"
#include "HAL.h"
#include "P4App.h"
#include "Utils/I18n/I18n.h"
#include "delay.h"
#include "mcu_config.h"

SystemInfo_t systemInfo;

namespace {
constexpr int kWidth = 294;
constexpr int kHeight = 126;
constexpr uint32_t kMinFreeBytes = 8U * 1024U; // 96 KB 池至少保留的余量
uint32_t now = 0;
bool power_key = false;
int shutdown_requests = 0;
int host_power_level = HIGH;
lv_color_t draw_pixels[kWidth * 20];
uint32_t peak_used = 0;

void
flush(lv_disp_drv_t* driver, const lv_area_t*, lv_color_t*) {
    lv_disp_flush_ready(driver);
}

// 与固件主循环相同的顺序：App_Update() 后 lv_timer_handler()。
void
run(uint32_t ms) {
    for (uint32_t elapsed = 0; elapsed < ms; elapsed += 5) {
        now += 5;
        App_Update();
        lv_timer_handler();
    }
    lv_mem_monitor_t monitor;
    lv_mem_monitor(&monitor);
    if (monitor.max_used > peak_used) {
        peak_used = monitor.max_used;
    }
}

const char*
page() {
    return App_Get()->current_page();
}

bool
on_page(const char* name) {
    return page() && std::strcmp(page(), name) == 0;
}

lv_obj_t*
find_text(lv_obj_t* root, const char* text) {
    if (lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strstr(lv_label_get_text(root), text)) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(root); ++i) {
        if (auto* found = find_text(lv_obj_get_child(root, static_cast<int32_t>(i)), text)) {
            return found;
        }
    }
    return nullptr;
}

void
reset_system_info() {
    std::memset(&systemInfo, 0, sizeof(systemInfo));
    std::snprintf(systemInfo.software_version, sizeof(systemInfo.software_version), "V0.0.0");
    std::snprintf(systemInfo.hardware_version, sizeof(systemInfo.hardware_version), "TEST");
    systemInfo.online_device.bq40z50 = true;
    systemInfo.online_device.mp2762 = true;
    systemInfo.powerMonitor.batteryInfo.sample_valid = true;
    systemInfo.powerMonitor.batteryInfo.Percent = 70;
    systemInfo.powerMonitor.batteryInfo.Voltage_f = 7.8f;
    systemInfo.powerMonitor.batteryInfo.Temp_f = 30.0f;
    systemInfo.positionInfo.coordinate_status = position_fix;
    systemInfo.positionInfo.satellite_number_used = 12;
}

// HAL 判定开机：模拟 Power_OnCheck / 驻留中长按开机。
void
hal_power_on_and_boot() {
    assert(on_page("Pages/Startup"));
    systemInfo.powerMonitor.ShutdownComplete = false;
    systemInfo.powerMonitor.ShutdownGoing = false;
    systemInfo.powerMonitor.panel_power_on = true;
    run(300);
    assert(on_page("Pages/SystemLoading"));
    run(7000);
    assert(on_page("Pages/Dialplate"));
}

// HAL 断电延时结束、外部供电仍在：进入驻留。
void
hal_enter_standby() {
    auto& power = systemInfo.powerMonitor;
    power.ShutdownGoing = false;
    power.ShutdownComplete = true;
    power.panel_power_on = false;
    power.ShutdownReq = power.LinuxPowerOff = power.LowBatteryPowerOff = power.Force_ShutDown = false;
    run(800);
    assert(on_page("Pages/Startup"));
}
} // namespace

extern "C" uint32_t
millis(void) {
    return now;
}

int
digitalRead(int) {
    return host_power_level;
}

void
HAL_NVIC_SystemReset() {
    std::fprintf(stderr, "unexpected app_fatal reset\n");
    std::abort();
}

void
HAL::Power_Shutdown(bool completed) {
    if (!completed) {
        systemInfo.powerMonitor.ShutdownReq = true;
        ++shutdown_requests;
    }
}

bool
HAL::Key_PowerPressed() {
    return power_key;
}

bool
HAL::Key_FunctionPressed() {
    return false;
}

int
main() {
    lv_init();
    static lv_disp_draw_buf_t draw;
    lv_disp_draw_buf_init(&draw, draw_pixels, nullptr, kWidth * 20);
    static lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = kWidth;
    driver.ver_res = kHeight;
    driver.draw_buf = &draw;
    driver.flush_cb = flush;
    assert(lv_disp_drv_register(&driver));

    reset_system_info();
    App_Init();
    run(400);
    assert(on_page("Pages/Startup"));
    assert(!find_text(lv_scr_act(), "DEMO")); // 真实数据不显示 DEMO 标记

    // 开机由 HAL 决定：界面长按 2 s 只发出请求，HAL 未确认时不切页。
    power_key = true;
    run(2300);
    assert(on_page("Pages/Startup"));
    power_key = false;
    run(500);
    hal_power_on_and_boot();

    // 逐页进入并返回，检查导航与内存。
    const char* pages[] = {"Pages/WorkSettings", "Pages/RecordConfig", "Pages/SystemInfos", "Pages/SystemDash",
                           "Pages/SystemSettings"};
    for (const char* name : pages) {
        assert(App_Get()->show_page(name));
        run(800);
        assert(on_page(name));
        assert(App_Get()->back());
        run(800);
    }
    assert(App_Get()->show_page("Pages/StarMap") == (PRODUCT_PAGE_STAR_MAP != 0));
    run(800);
    if (PRODUCT_PAGE_STAR_MAP) {
        assert(App_Get()->back());
        run(800);
    }
    assert(on_page("Pages/Dialplate"));

    // 用户关机：SystemDash 电源弹窗（无 Reboot）→ SaveConfig 结束时向 HAL 发出一次关机请求。
    assert(App_Get()->show_page("Pages/SystemDash"));
    run(800);
    App_Get()->on_input(P4App::InputAction::Confirm); // 打开电源弹窗，默认焦点 Cancel
    run(100);
    assert(!find_text(lv_scr_act(), i18n::text(i18n::TextId::RebootDouble)));
    App_Get()->on_input(P4App::InputAction::NextFocus); // → Shutdown
    App_Get()->on_input(P4App::InputAction::Commit);
    run(800);
    assert(on_page("Pages/SaveConfig"));
    run(10000);
    assert(shutdown_requests == 1);
    systemInfo.powerMonitor.ShutdownGoing = true; // HAL 电源策略进入关机
    run(200);
    assert(App_Get()->back() && on_page("Pages/SaveConfig")); // 关机中不能返回
    hal_enter_standby();

    // 主控请求关机：显示原因，不能返回，不再向 HAL 重复请求。
    hal_power_on_and_boot();
    systemInfo.powerMonitor.LinuxPowerOff = true;
    run(800);
    assert(on_page("Pages/SaveConfig"));
    assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::SaveConfigPowerOffBoard)));
    assert(App_Get()->back() && on_page("Pages/SaveConfig"));
    run(10000);
    assert(shutdown_requests == 1);
    hal_enter_standby();

    // 电池过温横幅：仅启用该功能的产品显示。
    hal_power_on_and_boot();
    systemInfo.powerMonitor.batteryInfo.Temp_f = 66.0f;
    run(400);
    assert((find_text(lv_layer_top(), "66.0") != nullptr) == (PRODUCT_PAGE_BATTERY_WARNING != 0));
    systemInfo.powerMonitor.batteryInfo.Temp_f = 30.0f;
    run(400);
    assert(!find_text(lv_layer_top(), "66.0"));

    std::printf("%s: LVGL pool peak %lu / %lu bytes (free margin %lu)\n", PRODUCT_NAME,
                static_cast<unsigned long>(peak_used), static_cast<unsigned long>(LV_MEM_SIZE),
                static_cast<unsigned long>(LV_MEM_SIZE - peak_used));
    assert(peak_used + kMinFreeBytes <= LV_MEM_SIZE);
    return 0;
}
