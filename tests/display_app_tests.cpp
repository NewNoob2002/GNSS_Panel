// HC32 带屏产品：共享 LVGL 应用 + AppPlatform（固件路径）的主机回归。
// 使用固件 lv_conf.h（96 KB LVGL 内存池）并让 C++ new 走 LVGL 池（lvgl_new.cpp），检查内存峰值余量；
// 主机指针为 8 字节，测得峰值高于 32 位单片机，结论偏保守。
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "App.h"
#include "AppPlatform.h"
#include "Charger.h"
#include "SystemService.h"
#include <HAL.h>
#include "Resource/ResourcePool.h"
#include "SystemState.h"
#include "I18n.h"
#include "Utils/PageStyle.h"
#include <delay.h>
#include <gpio.h>
#include "mcu_config.h"

namespace {
constexpr int kWidth = 294;
constexpr int kHeight = 126;
constexpr uint32_t kMinFreeBytes = 8U * 1024U; // 96 KB 池至少保留的余量
uint32_t now = 0;
bool power_key = false;
int power_on_requests = 0;
int pin_levels[32] = {};
int reset_reason = RESET;
int standby_calls = 0, charger_power_on_calls = 0;
bool probe_battery_gauge = true, probe_charger = true; // SystemLoading 重新探测 I2C 时的应答
uint16_t draw_pixels[kWidth * 20];
uint32_t peak_used = 0;

void
flush(lv_display_t* driver, const lv_area_t*, uint8_t*) {
    lv_display_flush_ready(driver);
}

// 与固件主循环相同的顺序：App_Update() 后 lv_timer_handler()。
void
run(uint32_t ms, bool update_power = true) {
    for (uint32_t elapsed = 0; elapsed < ms; elapsed += 5) {
        now += 5;
        if (update_power) {
            HAL::HAL_Update();
        }
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
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strstr(lv_label_get_text(root), text)) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_text(lv_obj_get_child(root, static_cast<int32_t>(i)), text)) {
            return found;
        }
    }
    return nullptr;
}

lv_obj_t*
find_image(lv_obj_t* root, const void* source) {
    if (lv_obj_check_type(root, &lv_image_class) && lv_image_get_src(root) == source) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_image(lv_obj_get_child(root, static_cast<int32_t>(i)), source)) {
            return found;
        }
    }
    return nullptr;
}

// 待机页开机进度环的当前值（%）。
int
startup_progress(lv_obj_t* root) {
    if (lv_obj_check_type(root, &lv_arc_class)) {
        return lv_arc_get_value(root);
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (const int value = startup_progress(lv_obj_get_child(root, static_cast<int32_t>(i))); value >= 0) {
            return value;
        }
    }
    return -1;
}

// 状态栏 SD 图标是否以录制色显示。
bool
status_recording() {
    auto* sd = find_image(lv_layer_top(), resource_pool::get_image("ui_sd_card"));
    assert(sd);
    return lv_color_to_u32(lv_obj_get_style_image_recolor(sd, LV_PART_MAIN)) == lv_color_to_u32(page::style::kInfo);
}

void
choose(i18n::TextId id) {
    for (unsigned step = 0; step < 8 && !find_text(App_Get()->focused(), i18n::text(id)); ++step) {
        App_Get()->on_input(App::InputAction::NextFocus);
    }
    assert(find_text(App_Get()->focused(), i18n::text(id)));
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
    State::Host().online = true; // 主控已在通信：加载页第 5 步立即通过
}

/** Model charger outputs while running the real firmware power policy. */
class TestCharger final : public HAL::Charger {
  public:
    /** Identify the virtual charger. */
    const char*
    name() const override {
        return "Test";
    }

    /** Keep board initialization independent of this power sequencing test. */
    void
    begin(BatteryInfo_t&) override {}

    /** Count standby entry without touching charger hardware. */
    void
    enter_standby() override {
        ++standby_calls;
    }

    /** Count output restoration after power-on acceptance. */
    void
    power_on() override {
        ++charger_power_on_calls;
    }

    /** Leave the explicitly seeded battery sample unchanged. */
    void
    update(BatteryInfo_t&) override {}

  protected:
    /** Accept charger enable changes on the virtual board. */
    bool
    apply_enabled(bool, BatteryInfo_t&) override {
        return true;
    }
};

/** Finish the Startup hold in the cold-boot wait, keeping the rail off until it returns. */
void
cold_boot_idle() {
    run(5);
    assert(now < 6000);
    assert(gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    if (systemInfo.power.panel_power_on) {
        assert(SystemService::snapshot().powered_on);
    }
}

/** Resume through the actual Startup hold and verify loading before reaching the home page. */
void
hal_power_on_and_boot() {
    assert(on_page("Pages/Startup"));
    const int before = power_on_requests;
    power_key = true;
    run(1500);
    assert(power_on_requests == before && !systemInfo.power.panel_power_on);
    assert(on_page("Pages/Startup") && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    power_key = false;
    run(100); // 进度尚未归零时重新按下，驻留恢复也必须重新计满 2 s。
    assert(startup_progress(lv_screen_active()) > 0);
    power_key = true;
    run(1995);
    assert(power_on_requests == before && !systemInfo.power.panel_power_on);
    assert(on_page("Pages/Startup") && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    run(300);
    assert(power_on_requests == before + 1 && systemInfo.power.panel_power_on);
    assert(on_page("Pages/SystemLoading") && gpio_read(POWER_CONTROL_PIN) == GPIO_HIGH);
    power_key = false;
    assert(!systemInfo.host.online); // 驻留恢复清除主控数据，随后模拟重新通信。
    State::Host().online = true;
    run(7000);
    assert(on_page("Pages/Dialplate"));
}

/** Complete a real HAL shutdown with external power keeping the MCU alive in standby. */
void
hal_enter_standby() {
    State::Power().ExternalPower = 1;
    State::Power().Force_ShutDown = true;
    HAL::Power_PowerOffMonitor();
    assert(systemInfo.power.ShutdownComplete && !systemInfo.power.panel_power_on);
    assert(gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    power_key = false;
    run(800);
    assert(on_page("Pages/Startup"));
}
} // namespace

extern "C" uint32_t
millis(void) {
    return now;
}

/** 读取虚拟板上独立的 Power 键与主控供电电平。 */
int
gpio_read(gpio_pin_t pin) {
    return pin == POWER_KEY_PIN ? (power_key ? GPIO_HIGH : GPIO_LOW) : pin_levels[pin];
}

/** 记录固件的输出电平写入。 */
void
gpio_write(gpio_pin_t pin, uint32_t value) {
    pin_levels[pin] = value == GPIO_HIGH;
}

/** 应用 Power_Init 指定的初始电平。 */
void
gpio_set_mode(gpio_pin_t pin, gpio_mode_t, uint8_t value) {
    gpio_write(pin, value);
}

/** 翻转虚拟输出电平。 */
void
gpio_toggle(gpio_pin_t pin) {
    pin_levels[pin] ^= 1U;
}

/** Return the seeded reset reason. */
en_flag_status_t
RMU_GetStatus(int) {
    return reset_reason;
}

/** Clear the virtual reset flags after firmware reads them. */
void
RMU_ClearStatus() {
    reset_reason = RESET;
}

/** Run the real firmware power monitor before the application update. */
void
HAL::HAL_Update() {
    Power_PowerOffMonitor();
}

/** Supply the virtual charger for real HAL power sequencing. */
HAL::Charger&
HAL::Charger_Get() {
    static TestCharger charger;
    return charger;
}

extern "C" bool __real__ZN3HAL15Power_RequestOnEv();

/** Count UI requests while preserving the real HAL implementation and return value. */
extern "C" bool
__wrap__ZN3HAL15Power_RequestOnEv() {
    ++power_on_requests;
    return __real__ZN3HAL15Power_RequestOnEv();
}

void
HAL_NVIC_SystemReset() {
    std::fprintf(stderr, "unexpected app_fatal reset\n");
    std::abort();
}

bool
HAL::Key_PowerPressed() {
    return power_key;
}

bool
HAL::Key_FunctionPressed() {
    return false;
}

bool
HAL::I2C_ProbeDevice(unsigned char address) {
    return address == 0x0B ? probe_battery_gauge : address == 0x5C && probe_charger;
}

int
main() {
    lv_init();
    lv_tick_set_cb(millis);
    lv_display_t* display = lv_display_create(kWidth, kHeight);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_pixels, NULL, sizeof(draw_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);

    reset_system_info();
    HAL::Power_Init();
    assert(gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    // 冷启动：界面开始轮询时键已按下，进度必须从这次按压开始计。
    power_key = true;
    App_Init();
    run(1000);
    assert(on_page("Pages/Startup"));
    assert(!find_text(lv_screen_active(), "DEMO")); // 真实数据不显示 DEMO 标记
    assert(startup_progress(lv_screen_active()) >= 30);

    run(500);
    assert(power_on_requests == 0 && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    power_key = false; // 约 1.5 s 松开，再在回退动画结束前重新按下。
    run(100);
    assert(on_page("Pages/Startup") && startup_progress(lv_screen_active()) > 0);
    assert(power_on_requests == 0 && !systemInfo.power.panel_power_on);
    assert(!SystemService::snapshot().powered_on && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    power_key = true;
    run(1995);
    assert(power_on_requests == 0 && !systemInfo.power.panel_power_on);
    assert(on_page("Pages/Startup") && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    HAL::Power_OnCheck(cold_boot_idle);
    assert(power_on_requests == 1 && systemInfo.power.panel_power_on);
    assert(gpio_read(POWER_CONTROL_PIN) == GPIO_HIGH && charger_power_on_calls == 1 && standby_calls == 1);
    power_key = false;
    run(300);
    assert(on_page("Pages/SystemLoading"));
    run(7000);
    assert(on_page("Pages/Dialplate"));

    // 逐页进入并返回，检查导航与内存。
    const char* pages[] = {"Pages/WorkSettings", "Pages/RecordConfig", "Pages/SystemInfos", "Pages/SystemDash"};
    assert(!App_Get()->show_page("Pages/SystemSettings"));
    for (const char* name : pages) {
        assert(App_Get()->show_page(name));
        run(800);
        assert(on_page(name));
        if (std::strcmp(name, "Pages/SystemInfos") == 0) {
            assert(lv_obj_get_parent(App_Get()->focused()) == lv_screen_active());
            assert(lv_group_get_obj_count(lv_group_get_default()) == 7);
            App_Get()->on_input(App::InputAction::NextFocus);
            assert(find_text(App_Get()->focused(), i18n::text(i18n::TextId::SystemWorkTitle)));
            App_Get()->on_input(App::InputAction::Confirm);
        } else {
            assert(App_Get()->back());
        }
        run(800);
        assert(on_page("Pages/Dialplate"));
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

    // 选择预设仅改草稿，点击 Apply 才下发；发出后留页等待主控读走。
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    App_Get()->on_input(App::InputAction::Confirm); // Radio rover -> Radio base
    assert(!request.work_pending);
    choose(i18n::TextId::Apply);
    App_Get()->on_input(App::InputAction::Confirm);
    run(800);
    assert(on_page("Pages/WorkSettings"));
    {
        const WorkConfig_t work = State::Effective_Work();
        assert(request.work_pending && work.work_mode == autobase_mode && work.radio_mode == radio_mode_tx);
        assert(work.radio_protocol == 5 && work.radio_channel == 5 && work.gprs_status == On_Off_Status_OFF);
    }
    assert(host.work.work_mode == rover_mode);
    assert(DeviceService::snapshot().work_pending);
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestWaiting)));
    State::Confirm_Work(State::Work_Revision());
    run(600);
    assert(!DeviceService::snapshot().work_pending && host.work.work_mode == autobase_mode);
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestReceived)));
    assert(App_Get()->back());
    run(800);

    // 网络基站不需要电台字段，选择后同样通过 Apply 下发。
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    App_Get()->on_input(App::InputAction::Confirm); // Radio base -> Network base
    assert(lv_group_get_obj_count(lv_group_get_default()) == 3);
    choose(i18n::TextId::Apply);
    App_Get()->on_input(App::InputAction::Confirm);
    run(800);
    {
        const WorkConfig_t work = State::Effective_Work();
        assert(request.work_pending && work.radio_status == On_Off_Status_OFF);
        assert(work.gprs_status == On_Off_Status_ON && work.NtripServer_status == On_Off_Status_ON);
    }
    assert(DeviceService::snapshot().ntrip == DataProc::NtripRole::Server && !DeviceService::snapshot().radio_on);
    State::Confirm_Work(State::Work_Revision());
    run(600);
    assert(App_Get()->back());
    run(800);
    host.work.radio_channel = 0;
    run(600);
    assert(App_Get()->show_page("Pages/WorkSettings"));
    run(800);
    App_Get()->on_input(App::InputAction::Confirm); // Network -> Radio rover
    choose(i18n::TextId::Apply);
    App_Get()->on_input(App::InputAction::Confirm);
    run(800);
    assert(on_page("Pages/WorkSettings") && !request.work_pending);
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::SelectChannel)));
    assert(App_Get()->back());
    run(800);

    // RecordConfig：开始与停止都是单击（Confirm）；双击（Commit）同样有效，等待期间不重复发送。
    assert(App_Get()->show_page("Pages/RecordConfig"));
    run(800);
    assert(find_text(App_Get()->focused(), i18n::text(i18n::TextId::StartupStart)));
    assert(!request.record_op && !request.record_settings_pending);
    App_Get()->on_input(App::InputAction::Confirm);
    {
        const RecordInfo_t record = State::Effective_Record();
        assert(record.record_status == On_Off_Status_ON && request.record_op && request.record_settings_pending);
        assert(record.record_type == 2 && record.record_interval == 0x01);
    }
    assert(DeviceService::snapshot().recording && DeviceService::snapshot().record_pending);
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestWaiting)));
    run(400);
    assert(!status_recording()); // 主控确认前状态栏仍按实际（未录制）显示
    const auto revision = State::Record_Revision();
    App_Get()->on_input(App::InputAction::Commit);
    assert(State::Record_Revision() == revision);
    State::Confirm_Record(State::Record_Revision(), true);
    run(600);
    assert(!DeviceService::snapshot().record_pending && status_recording());
    assert(host.record.record_status == On_Off_Status_ON && !request.record_op);
    choose(i18n::TextId::Stop);
    App_Get()->on_input(App::InputAction::Confirm);
    assert(State::Effective_Record().record_status == On_Off_Status_OFF && request.record_settings_pending);
    run(400);
    assert(status_recording()); // 停止请求确认前，状态栏仍显示录制中
    State::Confirm_Record(State::Record_Revision(), true);
    run(600);
    assert(!status_recording() && host.record.record_status == On_Off_Status_OFF);
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
        assert(App_Get()->show_page("Pages/SystemDash"));
        run(800);
        App_Get()->on_input(App::InputAction::NextFocus); // Language -> Wi-Fi
        App_Get()->on_input(App::InputAction::Confirm);
        assert(request.wifi_on == On_Off_Status_ON && request.wifi_pending);
        run(50);
        assert(SystemService::snapshot().wifi_pending && find_text(lv_screen_active(), "..."));
        wifi.wifi_status = On_Off_Status_ON;
        run(50);
        assert(!SystemService::snapshot().wifi_pending && SystemService::snapshot().wifi_on);
        assert(!find_text(lv_screen_active(), "..."));
        App_Get()->on_input(App::InputAction::Confirm);
        assert(request.wifi_on == On_Off_Status_OFF);
        run(14000);
        assert(SystemService::snapshot().wifi_pending);
        run(1500);
        assert(!SystemService::snapshot().wifi_pending && SystemService::snapshot().wifi_on);
        assert(App_Get()->back());
        run(800);
    }

    // 用户关机：SystemDash 电源弹窗（无 Reboot），长按 Power 进度满 100% 确认，短按/提前松开关闭弹窗；
    // SaveConfig 结束时向 HAL 发出一次关机请求。
    assert(App_Get()->show_page("Pages/SystemDash"));
    run(800);
    choose(i18n::TextId::PowerTitle);
    App_Get()->on_input(App::InputAction::Confirm); // 打开电源弹窗，默认焦点 Shutdown
    run(100);
    assert(find_text(App_Get()->focused(), i18n::text(i18n::TextId::ShutdownAction)));
    assert(!find_text(lv_screen_active(), i18n::text(i18n::TextId::RebootAction)));
    // 按键走固件路径：App_Update() 按 HAL::Key_PowerPressed() 电平轮询，弹窗打开时页面接管按下/松开。
    power_key = true;
    run(1000);
    power_key = false; // 未满 100% 松开：关闭弹窗，不准备关机，也不会被当作单击重新打开
    run(800);
    assert(find_text(App_Get()->focused(), i18n::text(i18n::TextId::PowerTitle)));
    assert(!SystemService::snapshot().power_pending);
    App_Get()->on_input(App::InputAction::Confirm);
    run(100);
    power_key = true;
    run(1500);
    assert(on_page("Pages/SystemDash") && !SystemService::snapshot().power_pending);
    run(800); // 满 2 s：准备关机并进入 SaveConfig
    assert(on_page("Pages/SaveConfig"));
    assert(SystemService::snapshot().power_pending);
    assert(SystemService::snapshot().power_action == DataProc::PowerAction::Off);
    assert(SystemService::snapshot().power_calls == 0);
    power_key = false;
    run(10000);
    assert(SystemService::snapshot().power_calls == 1);
    assert(systemInfo.power.ShutdownGoing); // HAL 已开始等待主控确认。
    run(200);
    assert(App_Get()->back() && on_page("Pages/SaveConfig")); // 关机中不能返回
    hal_enter_standby();

    // 主控请求关机：显示原因，不能返回，不再向 HAL 重复请求。
    const int before_resume = power_on_requests;
    power_key = true;
    now += 2500;
    HAL::Power_PowerOffMonitor(); // 只运行 HAL：原始电平按住超过 2 s 仍不得恢复。
    assert(on_page("Pages/Startup") && !systemInfo.power.panel_power_on);
    assert(power_on_requests == before_resume && gpio_read(POWER_CONTROL_PIN) == GPIO_LOW);
    power_key = false;
    run(100);
    hal_power_on_and_boot();
    State::Power().LinuxPowerOff = true;
    run(800);
    assert(on_page("Pages/SaveConfig"));
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::SaveConfigPowerOffBoard)));
    assert(App_Get()->back() && on_page("Pages/SaveConfig"));
    run(10000);
    assert(SystemService::snapshot().power_calls == 1);
    hal_enter_standby();

    // V1.3 主板没有 MP2762A（GPIO 控制充电），充电初始化步骤不算失败。
    State::Board().online_device.mp2762 = false;
    probe_charger = false;
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

    // SystemLoading 自检非阻塞：I2C 在加载页重新探测；V1.5 主板 MP2762A 不应答判失败；主控不通信最多等 30 s；
    // 失败都不阻止进入首页，等待期间平台关机照常进行。
    /** Request loading through Startup while leaving the host offline for initialization checks. */
    const auto power_on_to_loading = [] {
        power_key = true;
        run(2300);
        power_key = false;
        assert(systemInfo.power.panel_power_on && gpio_read(POWER_CONTROL_PIN) == GPIO_HIGH);
        assert(on_page("Pages/SystemLoading"));
    };
    hal_enter_standby();
    State::Board().online_device.mp2762 = true; // 开机扫描判定 V1.5
    std::snprintf(State::Board().hardware_version, sizeof(systemInfo.board.hardware_version), "V1.5");
    probe_charger = false;
    State::Host().online = false;
    power_on_to_loading();
    run(10000);
    assert(on_page("Pages/SystemLoading")); // 仍在等主控，主循环与界面照常运行
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::CheckingHostLink)));
    run(25000); // 第 5 步约在加载开始 2 s 后开始，等满 30 s，再经过完成与就绪页（约 1.5 s）
    assert(on_page("Pages/Dialplate"));
    {
        const auto report = SystemService::snapshot().initialization;
        assert(report[1].status == DataProc::InitializationStatus::Ok);
        assert(report[2].status == DataProc::InitializationStatus::Failed);
        assert(std::strstr(report[2].detail.data(), "MP2762A not responding (board V1.5)"));
        assert(report[4].status == DataProc::InitializationStatus::Failed);
        assert(std::strstr(report[4].detail.data(), "no host frame within 30 s"));
    }
    // 主控一通信即继续，不等满 30 s。
    hal_enter_standby();
    probe_charger = true;
    State::Host().online = false;
    power_on_to_loading();
    run(4000);
    assert(on_page("Pages/SystemLoading"));
    State::Host().online = true;
    run(2500);
    assert(on_page("Pages/Dialplate"));
    {
        const auto link = SystemService::snapshot().initialization[4];
        assert(link.status == DataProc::InitializationStatus::Ok && std::strstr(link.detail.data(), "host link up"));
    }
    // 等待主控期间强制关机（Function 长按，HAL 置 Force_ShutDown）：立即进入关机页，随后正常待机。
    hal_enter_standby();
    State::Host().online = false;
    power_on_to_loading();
    run(4000);
    assert(on_page("Pages/SystemLoading"));
    State::Power().Force_ShutDown = true;
    // 单独验证 PowerDevice 收到强制关机事件时的页面；下一次 HAL 监控会立即完成强制断电。
    run(800, false);
    assert(on_page("Pages/SaveConfig"));
    assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::SaveConfigPowerOffForced)));
    hal_enter_standby();
    State::Host().online = true;

    // 模拟软件复位重建应用：HAL 立即保持主控供电，PowerDevice 无需 Startup 请求即可切到加载页。
    hal_power_on_and_boot(); // 先恢复 Running 电源策略，模拟复位后的静态初值。
    App_Deinit();
    reset_system_info();
    reset_reason = SET;
    const int before_reset = power_on_requests;
    HAL::Power_Init();
    assert(gpio_read(POWER_CONTROL_PIN) == GPIO_HIGH && reset_reason == RESET);
    App_Init();
    HAL::Power_OnCheck();
    assert(systemInfo.power.panel_power_on && !power_key && power_on_requests == before_reset);
    run(300);
    assert(on_page("Pages/SystemLoading"));
    run(7000);
    assert(on_page("Pages/Dialplate") && power_on_requests == before_reset);

    std::printf("%s: LVGL pool peak %lu / %lu bytes (free margin %lu)\n", PRODUCT_NAME,
                static_cast<unsigned long>(peak_used), static_cast<unsigned long>(LV_MEM_SIZE),
                static_cast<unsigned long>(LV_MEM_SIZE - peak_used));
    assert(peak_used + kMinFreeBytes <= LV_MEM_SIZE);
    return 0;
}
