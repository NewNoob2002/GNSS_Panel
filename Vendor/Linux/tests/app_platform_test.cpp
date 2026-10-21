// Platform-decided power flow: power-on confirmed by the platform, no Reboot, platform shutdown and standby.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "Common/DataProc/DataProc.h"
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "App.h"
#include "Pages/SystemLoading/SystemLoadingModel.h"
#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "Utils/SettingsStyle.h"

namespace {
constexpr int width = 294, height = 126;
lv_color_t pixels[width * 20];
unsigned power_on_requests = 0, off_calls = 0, reboot_calls = 0;
unsigned wifi_requests = 0;
bool requested_wifi = false;
bool reject_device = false;
bool stall_last_step = false; // The last initialization step never finishes (a stuck platform check).
std::vector<DataProc::DeviceRequest> device_requests;
App* active = nullptr;

void flush(lv_disp_drv_t* driver, const lv_area_t*, lv_color_t*) {
    lv_disp_flush_ready(driver);
}
void advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}
void sample_keys(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        advance(10);
        active->poll_keys(lv_tick_get());
    }
}
lv_obj_t* find_text(lv_obj_t* root, const char* text) {
    if (lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN))
        return nullptr;
    if (lv_obj_check_type(root, &lv_label_class) && std::strstr(lv_label_get_text(root), text))
        return root;
    for (unsigned i = 0; i < lv_obj_get_child_cnt(root); ++i)
        if (auto* found = find_text(lv_obj_get_child(root, static_cast<int32_t>(i)), text))
            return found;
    return nullptr;
}
lv_obj_t* activity(lv_obj_t* obj) {
    if (lv_anim_get(obj, page::settings_style::pulse))
        return obj;
    for (unsigned i = 0; i < lv_obj_get_child_cnt(obj); ++i)
        if (auto* found = activity(lv_obj_get_child(obj, static_cast<int32_t>(i))))
            return found;
    return nullptr;
}
bool on_page(const char* name) {
    return active->current_page() && std::strcmp(active->current_page(), name) == 0;
}
void publish_status(bool temperature_valid = true, float temperature = 30.0f) {
    page::StatusBarState status;
    status.position = "RTK FIX"; // Real data: no "DEMO" text anywhere on screen.
    status.battery_valid = true;
    status.battery_percent = 60;
    status.battery_temperature_valid = temperature_valid;
    status.battery_temperature = temperature;
    active->update_status(status);
}
bool banner_shown() {
    return find_text(lv_layer_top(), "BATTERY HOT");
}
DataProc::DeviceSnapshot real_device() {
    DataProc::DeviceSnapshot d;
    d.position_valid = true;
    d.latitude = 22.543096;
    d.longitude = 114.057865;
    d.altitude = 8.25;
    d.satellites_valid = true;
    d.satellites_used = 23;
    d.satellites_tracked = 31;
    d.work_valid = true;
    d.work_mode = DataProc::WorkMode::Base;
    d.radio_on = true;
    d.radio_protocol = 3;
    d.radio_channel = 5;
    d.ntrip = DataProc::NtripRole::None;
    d.record_valid = true;
    d.record_type = 1;
    d.record_interval = 2;
    d.wifi_valid = true;
    d.wifi_station = false;
    std::snprintf(d.wifi_ssid.data(), d.wifi_ssid.size(), "X1PRO-TEST");
    d.wifi_ip = {10, 0, 0, 7};
    d.power_valid = true;
    d.battery_percent = 64.5f;
    d.battery_voltage = 7.71f;
    d.battery_temperature = 31.0f;
    d.charge = DataProc::ChargeState::Fast;
    std::snprintf(d.firmware.data(), d.firmware.size(), "V2.4.4");
    std::snprintf(d.hardware.data(), d.hardware.size(), "HW-B");
    std::snprintf(d.build.data(), d.build.size(), "2026-10-19");
    d.uptime_s = 3725;
    return d;
}
lv_obj_t* focus_text(const char* text) {
    return find_text(active->focused(), text);
}
void choose(i18n::TextId id) {
    for (unsigned step = 0; step < 8 && !focus_text(i18n::text(id)); ++step)
        active->on_input(App::InputAction::NextFocus);
    assert(focus_text(i18n::text(id)));
}
void boot_to_dialplate() {
    assert(on_page("Pages/Startup"));
    assert(SystemService::set_powered_on(true)); // Platform decided (e.g. HAL key timing).
    advance(200);
    assert(on_page("Pages/SystemLoading"));
    advance(7000); // Logo, five steps, completion and the fade to the working root.
    assert(on_page("Pages/Dialplate"));
}
} // namespace

// Test platform: power-on is decided outside the UI, Reboot is not offered, data is real.

namespace {
/** 演示/测试电源设备，保留原即时或延后确认行为。 */
class BackendPower : public system_backend::PowerObject {
  public:
    /** 读取服务已有的电源报告，无硬件操作。 */
    system_backend::PowerReport
    power_report() override {
        const auto s = SystemService::snapshot();
        return {s.powered_on, s.shutting_down, s.standby, s.power_cause};
    }

    /** 原后端开机操作。 */
    bool request_power_on() override;
    /** 原后端关机/重启操作。 */
    bool execute_power(DataProc::PowerAction) override;
};

/** 演示/测试业务设备，保留原请求结果及快照。 */
class BackendDevice : public system_backend::BusinessObject {
  public:
    /** 返回设备及服务已知的 Wi-Fi 状态。 */
    system_backend::DeviceReport device_report() override;
    /** 原后端业务请求。 */
    bool execute_device(const DataProc::DeviceRequest&) override;
    /** 原后端 Wi-Fi 请求。 */
    bool request_wifi(bool) override;
};

/** 演示/测试状态设备，提供能力与初始化检查。 */
class BackendStatus : public system_backend::StatusObject {
  public:
    /** 返回已发布状态，测试可继续直接注入快照。 */
    DataProc::StatusSnapshot
    status_snapshot() override {
        return {};
    }

    /** 原后端能力。 */
    system_backend::Capabilities capabilities() override;
    /** 原后端初始化元数据。 */
    const char* initialization_name(unsigned) override;
    /** 原后端步骤标题。 */
    i18n::TextId initialization_title(unsigned) override;
    /** 原后端非阻塞检查。 */
    DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t) override;
};
} // namespace

/** 三个静态设备不依赖 LVGL 或实际屏幕即可初始化。 */
DeviceManager&
system_backend::devices() {
    static BackendPower power;
    static BackendDevice device;
    static BackendStatus status;
    static ::DeviceObject* list[] = {&power, &device, &status};
    static DeviceManager manager(list, 3);
    static const bool initialized = (manager.init(), true);
    (void)initialized;
    return manager;
}

system_backend::Capabilities
BackendStatus::capabilities() {
    return {.simulated = false,
            .reboot_available = false,
            .wifi_available = true,
            .star_map = false,
            .record_settings = false};
}

bool
BackendPower::request_power_on() {
    ++power_on_requests;
    return false;
}

bool
BackendPower::execute_power(DataProc::PowerAction action) {
    ++(action == DataProc::PowerAction::Off ? off_calls : reboot_calls);
    return true; // Completion is reported later with SystemService::enter_standby().
}

i18n::TextId
BackendStatus::initialization_title(unsigned step) {
    constexpr i18n::TextId titles[] = {i18n::TextId::LoadingConfiguration, i18n::TextId::CheckingBatteryGauge,
                                       i18n::TextId::CheckingCharger, i18n::TextId::PoweringHostBoard,
                                       i18n::TextId::CheckingHostLink};
    return titles[step];
}

bool
BackendDevice::request_wifi(bool on) {
    ++wifi_requests;
    requested_wifi = on;
    return true; // Applied later by the test through SystemService::set_wifi().
}

system_backend::DeviceReport
BackendDevice::device_report() {
    return {}; // Nothing known until the platform publishes.
}

bool
BackendDevice::execute_device(const DataProc::DeviceRequest& request) {
    if (reject_device)
        return false;
    device_requests.push_back(request);
    auto device = DeviceService::snapshot();
    if (request.command == DataProc::DeviceCommand::ApplyWork)
        device.work_pending = true;
    else
        device.record_pending = true;
    return DeviceService::update(device);
}

const char*
BackendStatus::initialization_name(unsigned) {
    return "device";
}

DataProc::InitializationStatus
BackendStatus::poll_initialization_step(unsigned step, std::uint32_t elapsed_ms, char* detail, std::size_t size) {
    // Checks take different times; the last one waits for the host and may never answer.
    if ((step == 1 && elapsed_ms < 500) || (step == 4 && stall_last_step))
        return DataProc::InitializationStatus::Pending;
    std::snprintf(detail, size, "device %u online", step + 1);
    return DataProc::InitializationStatus::Ok;
}

int main() {
    lv_init();
    lv_disp_draw_buf_t draw;
    lv_disp_draw_buf_init(&draw, pixels, nullptr, width * 20);
    lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = width;
    driver.ver_res = height;
    driver.draw_buf = &draw;
    driver.flush_cb = flush;
    auto* display = lv_disp_drv_register(&driver);
    assert(display);
    {
        App app;
        active = &app;
        assert(app.init({.battery_warning = true}));
        publish_status();
        advance(400);
        assert(on_page("Pages/Startup"));
        assert(!SystemService::snapshot().simulated && !find_text(lv_scr_act(), "DEMO"));

        // Cold boot: the Power key that powered the board is already held when the UI starts polling. Startup's
        // hold must still count from it (gesture cancellation only stops it from becoming a click).
        app.on_key(App::Key::Power, true);
        sample_keys(2300);
        assert(power_on_requests == 1 && on_page("Pages/Startup"));
        app.on_key(App::Key::Power, false);
        sample_keys(600);

        // A completed hold only requests power-on; without confirmation the page stays and resets on release.
        sample_keys(40);
        app.on_key(App::Key::Power, true);
        sample_keys(2200);
        assert(power_on_requests == 2 && on_page("Pages/Startup"));
        app.on_key(App::Key::Power, false);
        sample_keys(600);
        assert(on_page("Pages/Startup") && find_text(lv_scr_act(), "0%"));

        // A quick re-press during the reset animation must still require a fresh 2 s hold.
        const unsigned before_repress = power_on_requests;
        app.on_key(App::Key::Power, true);
        sample_keys(1500);
        app.on_key(App::Key::Power, false);
        sample_keys(100);
        app.on_key(App::Key::Power, true);
        sample_keys(1990);
        assert(power_on_requests == before_repress && on_page("Pages/Startup"));
        sample_keys(100);
        assert(power_on_requests == before_repress + 1 && on_page("Pages/Startup"));
        app.on_key(App::Key::Power, false);
        sample_keys(600);

        // Platform step titles fit the 246px SystemLoading headline in both languages.
        for (auto language : {i18n::Language::English, i18n::Language::Russian}) {
            assert(i18n::set_language(language) || i18n::get_language() == language);
            for (unsigned step = 0; step < DataProc::kInitializationStepCount; ++step) {
                const char* title = i18n::text(system_backend::initialization_title(step));
                const auto* font = resource_pool::get_font("oswaldBold_18");
                assert(lv_txt_get_width(title, std::strlen(title), font, 0, LV_TEXT_FLAG_NONE) <= 246);
            }
        }
        assert(i18n::set_language(i18n::Language::English));

        boot_to_dialplate();
        assert(!find_text(lv_scr_act(), "DEMO"));
        // SystemLoading built the power-off page ahead (home is the current page); other pages load on entry.
        assert(DataProc::Broker()->search("SaveConfigModel"));
        assert(!DataProc::Broker()->search("WorkSettingsModel"));

        // Before the platform publishes, device fields show N/A; afterwards pages show the real data.
        assert(find_text(lv_scr_act(), "N/A") || on_page("Pages/Dialplate"));
        auto device = real_device();
        assert(DeviceService::update(device));
        advance(100);

        // SystemInfos cards from device data (formats as on the former HC32 pages).
        assert(app.show_page("Pages/SystemInfos"));
        advance(500);
        assert(find_text(lv_scr_act(), "22.543096\n114.057865\n8.25 m"));
        assert(find_text(lv_scr_act(), "AP-X1PRO-TEST\nOFF\n10.0.0.7"));
        assert(find_text(lv_scr_act(), "64.5%\n7.71 V\n31.0 C"));
        assert(find_text(lv_scr_act(), "V2.4.4\n01:02:05\n0\n2026-10-19"));
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::WorkModeBase)));
        assert(app.back());
        advance(500);

        // 预设选择不发送；应用后留页，显示等待、超时和主控接收状态。
        device_requests.clear();
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        assert(focus_text(i18n::text(i18n::TextId::RadioBase)));
        app.on_input(App::InputAction::Confirm); // Base -> Network base
        assert(device_requests.empty() && find_text(lv_scr_act(), i18n::text(i18n::TextId::NetworkBase)));
        assert(lv_group_get_obj_count(lv_group_get_default()) == 3); // Preset, Apply, Back.
        choose(i18n::TextId::Apply);
        app.on_input(App::InputAction::Confirm);
        assert(on_page("Pages/WorkSettings") && device_requests.size() == 1);
        assert(device_requests.back().target == DataProc::WorkTarget::Ntrip);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestWaiting)));
        auto* work_dot = activity(lv_scr_act());
        assert(work_dot);
        const auto initial_opacity = lv_obj_get_style_bg_opa(work_dot, 0);
        advance(350);
        assert(lv_obj_get_style_bg_opa(work_dot, 0) != initial_opacity);
        assert(lv_group_get_obj_count(lv_group_get_default()) == 1);
        app.on_input(App::InputAction::Commit);
        assert(device_requests.size() == 1);
        advance(15500);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestTimeout)));
        device.work_pending = false;
        assert(DeviceService::update(device));
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestReceived)));
        assert(!lv_anim_get(work_dot, page::settings_style::pulse));
        assert(app.back());
        advance(500);
        device.radio_channel = 0;
        assert(DeviceService::update(device));
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        choose(i18n::TextId::Apply);
        app.on_input(App::InputAction::Confirm);
        assert(on_page("Pages/WorkSettings") && device_requests.size() == 1);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::SelectChannel)));
        assert(app.back());
        advance(500);
        device.radio_channel = 5;
        assert(DeviceService::update(device));
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        choose(i18n::TextId::Apply);
        reject_device = true;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests.size() == 1 && find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestRejected)));
        reject_device = false;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests.size() == 2 && device_requests.back().radio_protocol == 3
               && device_requests.back().radio_channel == 5);
        assert(DeviceService::update(device));
        advance(100);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestReceived)));
        assert(app.back());
        advance(500);

        // 固定参数只读；单击即开始/停止（双击同样有效），等待期间禁止重复请求。
        assert(app.show_page("Pages/RecordConfig"));
        advance(500);
        assert(focus_text(i18n::text(i18n::TextId::StartupStart)));
        reject_device = true;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests.size() == 2 && find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestRejected)));
        reject_device = false;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests.size() == 3 && device_requests.back().command == DataProc::DeviceCommand::StartRecord);
        assert(device_requests.back().record_type == 1 && device_requests.back().record_interval == 2);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestWaiting)));
        auto* record_dot = activity(lv_scr_act());
        assert(record_dot);
        app.on_input(App::InputAction::Commit); // 等待期间按钮禁用、焦点在返回；双击也不会重复发送。
        assert(device_requests.size() == 3);
        advance(15500);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestTimeout)));
        device.recording = true;
        assert(DeviceService::update(device));
        advance(100);
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::RequestReceived)));
        choose(i18n::TextId::Stop);
        app.on_input(App::InputAction::Confirm);
        assert(device_requests.size() == 4 && device_requests.back().command == DataProc::DeviceCommand::StopRecord);
        device.recording = false;
        assert(DeviceService::update(device));
        assert(!lv_anim_get(record_dot, page::settings_style::pulse));
        assert(app.back());
        advance(500);
        assert(on_page("Pages/Dialplate"));

        // Wi-Fi: pending ("...") until the platform reports the applied state.
        assert(app.show_page("Pages/SystemDash"));
        advance(500);
        app.on_input(App::InputAction::NextFocus); // Language -> Wi-Fi
        assert(focus_text(i18n::text(i18n::TextId::SystemWifiTitle)));
        app.on_input(App::InputAction::Confirm);
        assert(wifi_requests == 1 && requested_wifi && SystemService::snapshot().wifi_pending);
        advance(50);
        assert(find_text(lv_scr_act(), "..."));
        auto* wifi_dot = activity(lv_scr_act());
        assert(wifi_dot);
        app.on_input(App::InputAction::Confirm); // Ignored while pending.
        assert(wifi_requests == 1);
        assert(SystemService::set_wifi(true, false));
        assert(!lv_anim_get(wifi_dot, page::settings_style::pulse));
        advance(50);
        assert(!find_text(lv_scr_act(), "...") && !SystemService::snapshot().wifi_pending);
        assert(SystemService::snapshot().wifi_on);
        assert(app.back());
        advance(500);
        assert(on_page("Pages/Dialplate"));

        // No StarMap: not registered, and the Dialplate satellite icon is not a focus stop.
        assert(!app.show_page("Pages/StarMap") && on_page("Pages/Dialplate"));
        {
            lv_obj_t* start = app.focused();
            unsigned stops = 0;
            do {
                app.on_input(App::InputAction::NextFocus);
                ++stops;
            } while (app.focused() != start && stops < 10);
            assert(stops == 4); // Work, Record, Menu, Power.
        }
        // Display-only record settings: the rollers are not focus stops (record button and Return only).
        assert(app.show_page("Pages/RecordConfig"));
        advance(500);
        {
            lv_obj_t* start = app.focused();
            assert(start);
            unsigned stops = 0;
            do {
                app.on_input(App::InputAction::NextFocus);
                ++stops;
            } while (app.focused() != start && stops < 10);
            assert(stops == 2);
        }
        assert(app.back());
        advance(500);
        assert(on_page("Pages/Dialplate"));

        // Over-temperature banner with hysteresis: 65 C on, below 60 C off, invalid data off.
        assert(!banner_shown());
        publish_status(true, 66.0f);
        advance(300);
        assert(banner_shown() && find_text(lv_layer_top(), "66.0"));
        publish_status(true, 62.0f);
        advance(100);
        assert(banner_shown() && find_text(lv_layer_top(), "62.0"));
        publish_status(true, 59.5f);
        advance(100);
        assert(!banner_shown());
        publish_status(true, 70.0f);
        advance(100);
        assert(banner_shown());
        publish_status(false, 70.0f);
        advance(100);
        assert(!banner_shown());
        publish_status();

        // Power dialog without Reboot: Shutdown only; a press dismisses it, a 2 s hold confirms.
        assert(app.show_page("Pages/SystemDash"));
        advance(400);
        choose(i18n::TextId::PowerTitle);
        app.on_input(App::InputAction::Confirm);
        advance(100);
        assert(find_text(app.focused(), i18n::text(i18n::TextId::ShutdownAction)));
        assert(!find_text(lv_scr_act(), i18n::text(i18n::TextId::RebootAction)));
        assert(lv_group_get_obj_count(lv_group_get_default()) == 1);
        app.on_input(App::InputAction::Commit); // Double-click no longer confirms anything.
        advance(100);
        assert(on_page("Pages/SystemDash") && !SystemService::snapshot().power_pending);
        assert(find_text(app.focused(), i18n::text(i18n::TextId::ShutdownAction)));
        app.on_input(App::InputAction::Press); // Released before 100%: the dialog closes, nothing prepared.
        advance(1500);
        app.on_input(App::InputAction::Release);
        advance(100);
        assert(focus_text(i18n::text(i18n::TextId::PowerTitle)) && !SystemService::snapshot().power_pending);
        assert(!find_text(lv_scr_act(), i18n::text(i18n::TextId::ShutdownAction)));
        app.on_input(App::InputAction::Confirm);
        advance(100);
        // Service-level guard: without platform reboot, even SystemDash's own Reboot request is rejected.
        DataProc::SystemRequest reboot{};
        reboot.command = DataProc::SystemCommand::PreparePower;
        reboot.action = DataProc::PowerAction::Reboot;
        auto* dash = DataProc::Broker()->search("SystemDashModel");
        assert(dash && dash->notify("System", &reboot, sizeof(reboot)) == DataNode::RES_PARAM_ERROR);
        assert(!SystemService::snapshot().power_pending);
        assert(find_text(app.focused(), i18n::text(i18n::TextId::ShutdownAction)));

        // User shutdown: a completed hold prepares it, SaveConfig executes it, then the page waits for standby.
        app.on_input(App::InputAction::Press);
        advance(1500);
        assert(on_page("Pages/SystemDash") && !SystemService::snapshot().power_pending);
        advance(600);
        assert(SystemService::snapshot().power_pending);
        // The dialog stays unchanged under the sliding SaveConfig; it closes only after the transition.
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::ShutdownAction)));
        advance(400);
        assert(on_page("Pages/SaveConfig"));
        app.on_input(App::InputAction::Release);
        advance(10000);
        assert(off_calls == 1 && reboot_calls == 0 && on_page("Pages/SaveConfig"));
        assert(SystemService::begin_shutdown(DataProc::PowerCause::User)); // e.g. HAL ShutdownGoing.
        advance(100);
        assert(app.back() && on_page("Pages/SaveConfig")); // Back is consumed during a shutdown.
        assert(SystemService::enter_standby());
        advance(600);
        assert(on_page("Pages/Startup") && !app.back());

        // A platform step that never finishes cannot block startup: it is recorded as failed after the page's
        // limit and the working root is still reached (power on/off keep working meanwhile).
        stall_last_step = true;
        assert(SystemService::set_powered_on(true));
        advance(200);
        assert(on_page("Pages/SystemLoading"));
        advance(page::SystemLoadingModel::kStepLimitMs);
        assert(on_page("Pages/SystemLoading"));
        advance(5000);
        assert(on_page("Pages/Dialplate"));
        const auto stalled = SystemService::snapshot().initialization[4]; // snapshot() returns a copy.
        assert(stalled.status == DataProc::InitializationStatus::Failed
               && std::strstr(stalled.detail.data(), "no result within"));
        stall_last_step = false;

        // Platform shutdown (host request): SaveConfig shows the cause, cannot be left, standby returns.
        assert(SystemService::begin_shutdown(DataProc::PowerCause::Host));
        advance(600);
        assert(on_page("Pages/SaveConfig"));
        assert(find_text(lv_scr_act(), i18n::text(i18n::TextId::SaveConfigPowerOffBoard)));
        assert(app.back() && on_page("Pages/SaveConfig"));
        advance(10000);
        assert(off_calls == 1 && on_page("Pages/SaveConfig")); // Nothing is executed for a platform shutdown.
        assert(SystemService::enter_standby());
        advance(600);
        assert(on_page("Pages/Startup"));
        assert(!SystemService::begin_shutdown(DataProc::PowerCause::Forced)); // Already in standby.
        active = nullptr;
    }
    lv_disp_remove(display);
    std::puts("app_platform: OK");
    return 0;
}
