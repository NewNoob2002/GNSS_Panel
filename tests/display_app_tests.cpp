// HC32 带屏产品：共享 LVGL 应用 + AppPlatform（固件路径）的主机回归。
// 使用固件 lv_conf.h（96 KB LVGL 内存池）并让 C++ new 走 LVGL 池（lvgl_new.cpp），检查内存峰值余量；
// 主机指针为 8 字节，测得峰值高于 32 位单片机，结论偏保守。
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppPlatform.h"
#include "Arduino.h"
#include "Common/SystemService.h"
#include "HAL.h"
#include "P4App.h"
#include "SystemState.h"
#include "Utils/I18n/I18n.h"
#include "delay.h"
#include "mcu_config.h"

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
    State::Reset();
    BoardInfo_t& board = State::Board();
    std::snprintf(board.software_version, sizeof(board.software_version), "V0.0.0");
    std::snprintf(board.hardware_version, sizeof(board.hardware_version), "TEST");
    board.online_device.bq40z50 = true;
    board.online_device.mp2762 = true;
    State::Power().batteryInfo.sample_valid = true;
    State::Power().batteryInfo.Percent = 70;
    State::Power().batteryInfo.Voltage_f = 7.8f;
    State::Power().batteryInfo.Temp_f = 30.0f;
    State::Host().position.coordinate_status = position_fix;
    State::Host().position.satellite_number_used = 12;
}

// HAL 判定开机：模拟 Power_OnCheck / 驻留中长按开机。
void
hal_power_on_and_boot() {
    assert(on_page("Pages/Startup"));
    State::Power().ShutdownComplete = false;
    State::Power().ShutdownGoing = false;
    State::Power().panel_power_on = true;
    run(300);
    assert(on_page("Pages/SystemLoading"));
    run(7000);
    assert(on_page("Pages/Dialplate"));
}

// HAL 断电延时结束、外部供电仍在：进入驻留。
void
hal_enter_standby() {
    auto& power = State::Power();
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
        State::Power().ShutdownReq = true;
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

    // 设备数据：主控上报的编码 -> 页面选项索引；定长字段可能不以 NUL 结尾。
    HostReport_t& host = State::Host();
    const PanelRequest_t& request = systemInfo.request;
    host.work.work_mode = rover_mode;
    host.work.radio_status = On_Off_Status_ON;
    host.work.radio_protocol = 5; // TRANSEOT，索引 3
    host.work.radio_channel = 5;
    host.record.record_type = 2;        // Rinex3.02，索引 1
    host.record.record_interval = 0x01; // 60 min，索引 2
    std::memcpy(host.record.record_name, "ABCDEFGHIJKLMNOP", sizeof(host.record.record_name));
    std::snprintf(State::Board().hardware_version, sizeof(systemInfo.board.hardware_version), "V1.5");
    run(600);
    {
        const auto device = DeviceService::snapshot();
        assert(device.work_mode == DataProc::WorkMode::Rover && device.radio_on);
        assert(device.radio_protocol == 3 && device.radio_channel == 5);
        assert(device.ntrip == DataProc::NtripRole::None);
        assert(device.record_type == 1 && device.record_interval == 2 && !device.recording);
        assert(std::strcmp(device.record_name.data(), "ABCDEFGHIJKLMNOP") == 0);
        assert(std::strcmp(device.hardware.data(), "V1.5") == 0);
        assert(device.power_valid && device.battery_voltage == 7.8f);
    }

    // WorkSettings：Base 以当前协议/信道下发（控件顺序 滚轮, Base, Rover, NTRIP, Return）。
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    for (int i = 0; i < 3; ++i) { // Return -> NTRIP -> Rover -> Base
        App_Get()->on_input(P4App::InputAction::PreviousFocus);
    }
    App_Get()->on_input(P4App::InputAction::Confirm);
    run(800);
    assert(on_page("Pages/Dialplate"));
    {
        const WorkConfig_t work = State::Effective_Work();
        assert(request.work_pending && work.work_mode == autobase_mode && work.radio_mode == radio_mode_tx);
        assert(work.radio_protocol == 5 && work.radio_channel == 5 && work.gprs_status == On_Off_Status_OFF);
    }
    // 面板请求与主控上报分开：主控仍上报旧模式时，界面显示待确认的请求。
    assert(host.work.work_mode == rover_mode);
    run(600);
    assert(DeviceService::snapshot().work_mode == DataProc::WorkMode::Base);
    // 主控读走请求后确认，请求值视为主控已采用。
    State::Confirm_Work(State::Work_Revision());
    assert(!request.work_pending && host.work.work_mode == autobase_mode);
    // NTRIP：关闭电台，打开网络基站。
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    App_Get()->on_input(P4App::InputAction::PreviousFocus); // Return -> NTRIP
    App_Get()->on_input(P4App::InputAction::Confirm);
    run(800);
    assert(on_page("Pages/Dialplate"));
    {
        const WorkConfig_t work = State::Effective_Work();
        assert(request.work_pending && work.radio_status == On_Off_Status_OFF);
        assert(work.gprs_status == On_Off_Status_ON && work.NtripServer_status == On_Off_Status_ON);
    }
    assert(DeviceService::snapshot().ntrip == DataProc::NtripRole::Server && !DeviceService::snapshot().radio_on);
    // Rover 选中 CUSTOM 信道（索引 0）：不下发，页面仍返回。
    State::Confirm_Work(State::Work_Revision());
    host.work.radio_channel = 0;
    run(600);
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    for (int i = 0; i < 2; ++i) { // Return -> NTRIP -> Rover
        App_Get()->on_input(P4App::InputAction::PreviousFocus);
    }
    App_Get()->on_input(P4App::InputAction::Confirm);
    run(800);
    assert(on_page("Pages/Dialplate"));
    assert(!request.work_pending && State::Effective_Work().work_mode == autobase_mode);

    // RecordConfig：双击 Power（Commit）开始，单击（Confirm）停止。
    assert(App_Get()->show_page("Pages/RecordConfig"));
    run(800);
    App_Get()->on_input(P4App::InputAction::PreviousFocus); // Return -> 录制按钮
    App_Get()->on_input(P4App::InputAction::Confirm);       // 未录制时单击不开始
    assert(!request.record_op && !request.record_settings_pending);
    App_Get()->on_input(P4App::InputAction::Commit);
    {
        const RecordInfo_t record = State::Effective_Record();
        assert(record.record_status == On_Off_Status_ON && request.record_op && request.record_settings_pending);
        assert(record.record_type == 2 && record.record_interval == 0x01);
    }
    assert(DeviceService::snapshot().recording);
    run(100);
    State::Confirm_Record(State::Record_Revision(), true);
    assert(host.record.record_status == On_Off_Status_ON && !request.record_op);
    App_Get()->on_input(P4App::InputAction::Confirm);
    assert(State::Effective_Record().record_status == On_Off_Status_OFF && request.record_settings_pending);
    assert(App_Get()->back());
    run(800);
    assert(on_page("Pages/Dialplate"));

    // Wi-Fi：发出设定后显示 "..."，主控回报一致后结束；主控不回报时 15 s 后放弃等待。
    assert(SystemService::snapshot().wifi_available == (PRODUCT_SHUTDOWN_WIFI_BUTTON != 0));
    // 无论能否切换，主控回报的状态都要显示。
    host.wifi.wifi_status = On_Off_Status_ON;
    run(50);
    assert(SystemService::snapshot().wifi_on && !SystemService::snapshot().wifi_pending);
    host.wifi.wifi_status = On_Off_Status_OFF;
    run(50);
    assert(!SystemService::snapshot().wifi_on);
    if (PRODUCT_SHUTDOWN_WIFI_BUTTON) {
        auto& wifi = host.wifi;
        assert(App_Get()->show_page("Pages/SystemSettings"));
        run(800);
        App_Get()->on_input(P4App::InputAction::NextFocus); // Language -> Wi-Fi
        App_Get()->on_input(P4App::InputAction::Confirm);
        assert(request.wifi_on == On_Off_Status_ON && request.wifi_pending);
        run(50);
        assert(SystemService::snapshot().wifi_pending && find_text(lv_scr_act(), "..."));
        wifi.wifi_status = On_Off_Status_ON;
        run(50);
        assert(!SystemService::snapshot().wifi_pending && SystemService::snapshot().wifi_on);
        assert(!find_text(lv_scr_act(), "..."));
        App_Get()->on_input(P4App::InputAction::Confirm);
        assert(request.wifi_on == On_Off_Status_OFF);
        run(14000);
        assert(SystemService::snapshot().wifi_pending);
        run(1500);
        assert(!SystemService::snapshot().wifi_pending && SystemService::snapshot().wifi_on);
        assert(App_Get()->back());
        run(800);
    }

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
    State::Power().ShutdownGoing = true; // HAL 电源策略进入关机
    run(200);
    assert(App_Get()->back() && on_page("Pages/SaveConfig")); // 关机中不能返回
    hal_enter_standby();

    // 主控请求关机：显示原因，不能返回，不再向 HAL 重复请求。
    hal_power_on_and_boot();
    State::Power().LinuxPowerOff = true;
    run(800);
    assert(on_page("Pages/SaveConfig"));
    assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::SaveConfigPowerOffBoard)));
    assert(App_Get()->back() && on_page("Pages/SaveConfig"));
    run(10000);
    assert(shutdown_requests == 1);
    hal_enter_standby();

    // V1.3 主板没有 MP2762A（GPIO 控制充电），充电初始化步骤不算失败。
    State::Board().online_device.mp2762 = false;
    std::snprintf(State::Board().hardware_version, sizeof(systemInfo.board.hardware_version), "V1.3");
    hal_power_on_and_boot();
    {
        const auto charger = SystemService::snapshot().initialization[2];
        assert(charger.status == DataProc::InitializationStatus::Ok);
        assert(std::strstr(charger.detail.data(), "GPIO charge control (board V1.3)"));
    }

    // 电池过温横幅：仅启用该功能的产品显示。
    State::Power().batteryInfo.Temp_f = 66.0f;
    run(400);
    assert((find_text(lv_layer_top(), "66.0") != nullptr) == (PRODUCT_PAGE_BATTERY_WARNING != 0));
    State::Power().batteryInfo.Temp_f = 30.0f;
    run(400);
    assert(!find_text(lv_layer_top(), "66.0"));

    std::printf("%s: LVGL pool peak %lu / %lu bytes (free margin %lu)\n", PRODUCT_NAME,
                static_cast<unsigned long>(peak_used), static_cast<unsigned long>(LV_MEM_SIZE),
                static_cast<unsigned long>(LV_MEM_SIZE - peak_used));
    assert(peak_used + kMinFreeBytes <= LV_MEM_SIZE);
    return 0;
}
