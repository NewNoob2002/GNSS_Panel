// Platform-decided power flow: power-on confirmed by the platform, no Reboot, platform shutdown and standby.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "App.h"
#include "DataProc/DataProc.h"
#include "DomainFixture.hpp"
#include "I18n.h"
#include "HAL_Manager.h"
#include "hal_device_checks.h"
#include "UI/Common/SettingsStyle.h"
#include "UI/Resource/ResourcePool.h"
#include "UI/SystemLoading/SystemLoadingModel.h"
#include "Def/DP_Power.h"

namespace {
bool power_on();
constexpr int width = 294, height = 126;
uint16_t pixels[width * 20];
unsigned power_on_requests = 0, off_calls = 0, reboot_calls = 0;
unsigned wifi_requests = 0;
bool requested_wifi = false;
bool reject_device = false;
bool stall_last_step = false; // The last initialization step never finishes (a stuck platform check).
std::vector<hal::WorkCommand> work_requests;
std::vector<hal::RecorderCommand> record_requests;
unsigned device_requests = 0;
App* active = nullptr;

void
flush(lv_display_t* driver, const lv_area_t*, uint8_t*) {
    lv_display_flush_ready(driver);
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
    if (lv_obj_is_hidden(root)) {
        return nullptr;
    }
    if (lv_obj_check_type(root, &lv_label_class) && std::strstr(lv_label_get_text(root), text))
        return root;
    for (unsigned i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_text(lv_obj_get_child(root, static_cast<int32_t>(i)), text))
            return found;
    }
    return nullptr;
}
lv_obj_t* activity(lv_obj_t* obj) {
    if (lv_anim_get(obj, page::settings_style::pulse))
        return obj;
    for (unsigned i = 0; i < lv_obj_get_child_count(obj); ++i) {
        if (auto* found = activity(lv_obj_get_child(obj, static_cast<int32_t>(i))))
            return found;
    }
    return nullptr;
}
bool on_page(const char* name) {
    return active->current_page() && std::strcmp(active->current_page(), name) == 0;
}
void publish_status(bool temperature_valid = true, float temperature = 30.0f) {
    fixture::source.battery = {true, 60, 60.f, 7.6f, temperature_valid, temperature, hal::ChargeState::NotCharging,
                               false};
    fixture::source.gnss.fix = hal::FixStatus::Fixed;
    lv_tick_inc(200);
    DataProc_RunLoop();
}
bool banner_shown() {
    return find_text(lv_layer_top(), "BATTERY HOT");
}

fixture::State
real_device() {
    fixture::State d;
    d.gnss.position_valid = true;
    d.gnss.fix = hal::FixStatus::Fixed;
    d.gnss.latitude = 22.543096;
    d.gnss.longitude = 114.057865;
    d.gnss.altitude = 8.25;
    d.gnss.satellites_valid = true;
    d.gnss.satellites_used = 23;
    d.gnss.satellites_tracked = 31;
    d.work.valid = true;
    d.work.mode = hal::WorkMode::Base;
    d.work.radio_on = true;
    d.work.protocol = 3;
    d.work.channel = 5;
    d.work.ntrip = hal::NtripRole::None;
    d.recorder.valid = true;
    d.recorder.type = 1;
    d.recorder.interval = 2;
    d.wifi.valid = true;
    d.wifi.station = false;
    std::snprintf(d.wifi.ssid.data(), d.wifi.ssid.size(), "X1PRO-TEST");
    d.wifi.ip = {10, 0, 0, 7};
    d.battery.valid = true;
    d.battery.level_percent = 64;
    d.battery.gauge_percent = 64.5f;
    d.battery.temperature_valid = true;
    d.battery.voltage = 7.71f;
    d.battery.temperature = 31.0f;
    d.battery.charge = hal::ChargeState::Fast;
    std::snprintf(d.board.firmware.data(), d.board.firmware.size(), "V2.4.4");
    std::snprintf(d.board.hardware.data(), d.board.hardware.size(), "HW-B");
    std::snprintf(d.board.build.data(), d.board.build.size(), "2026-10-19");
    d.board.uptime_s = 3725;
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
    assert(power_on()); // Platform decided (e.g. HAL key timing).
    advance(200);
    assert(on_page("Pages/SystemLoading"));
    advance(7000); // Logo, five steps, completion and the fade to the working root.
    assert(on_page("Pages/Dialplate"));
}
} // namespace

// 可变测试平台实现九个细设备，异步结果通过修改 source 后推进定时器上报。
DeviceManager&
hal::manager() {
    return fixture::devices();
}

namespace {
bool
request_on() {
    ++power_on_requests;
    return false;
}

bool
execute_power(hal::PowerCommand action) {
    ++(action == hal::PowerCommand::Off ? off_calls : reboot_calls);
    return true;
}

bool
request_wifi(bool on) {
    ++wifi_requests;
    requested_wifi = on;
    fixture::source.wifi.pending = true;
    return true;
}

bool
execute_work(const hal::WorkCommand& request) {
    if (reject_device) {
        return false;
    }
    ++device_requests;
    work_requests.push_back(request);
    fixture::source.work.pending = true;
    return true;
}

bool
execute_record(const hal::RecorderCommand& request) {
    if (reject_device) {
        return false;
    }
    ++device_requests;
    record_requests.push_back(request);
    fixture::source.recorder.pending = true;
    return true;
}

hal::InitStatus
poll(unsigned step, uint32_t elapsed, char* detail, size_t size) {
    if ((step == 1 && elapsed < 500) || (step == 4 && stall_last_step)) {
        return hal::InitStatus::Pending;
    }
    std::snprintf(detail, size, "device %u online", step + 1);
    return hal::InitStatus::Ok;
}

bool
set_device(const fixture::State& value) {
    if (!fixture::set_domains(value)) {
        return false;
    }
    lv_tick_inc(1000);
    DataProc_RunLoop();
    return true;
}

bool
power_on() {
    fixture::source.power = {true, false, false, hal::PowerCause::User};
    lv_tick_inc(20);
    DataProc_RunLoop();
    return true;
}

bool
shutdown(hal::PowerCause cause) {
    auto& p = fixture::source.power;
    if (p.standby) {
        return false;
    }
    p.shutting_down = true;
    p.cause = cause;
    lv_tick_inc(20);
    DataProc_RunLoop();
    return true;
}

bool
standby() {
    fixture::source.power = {false, false, true, hal::PowerCause::User};
    lv_tick_inc(20);
    DataProc_RunLoop();
    return true;
}

bool
wifi(bool on, bool pending) {
    fixture::source.wifi.on = on;
    fixture::source.wifi.pending = pending;
    lv_tick_inc(500);
    DataProc_RunLoop();
    return true;
}
} // namespace

int main() {
    fixture::source.board.capabilities = {false, false, true, false, false};
    fixture::source.wifi.available = true;
    fixture::source.request_on = request_on;
    fixture::source.execute_power = execute_power;
    fixture::source.request_wifi = request_wifi;
    fixture::source.execute_work = execute_work;
    fixture::source.execute_record = execute_record;
    fixture::source.poll = poll;
    lv_init();
    lv_display_t* display = lv_display_create(width, height);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    assert(display);
    {
        App app;
        active = &app;
        assert(app.init({.battery_warning = true}));
        publish_status();
        advance(400);
        assert(on_page("Pages/Startup"));
        assert(!hal_test::snapshot<dp::Version>().capabilities.simulated && !find_text(lv_screen_active(), "DEMO"));

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
        assert(on_page("Pages/Startup") && find_text(lv_screen_active(), "0%"));

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
                dp::InitStepInfo info;
                assert(dp::Init::info(*DataProc::Broker()->mainNode(), step, info) == DataNode::RES_OK);
                const char* title = i18n::text(info.title);
                const auto* font = resource_pool::get_font("oswaldBold_18");
                lv_point_t text_size;
                lv_text_get_size(&text_size, title, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
                assert(text_size.x <= 246);
            }
        }
        assert(i18n::set_language(i18n::Language::English));

        boot_to_dialplate();
        assert(!find_text(lv_screen_active(), "DEMO"));
        // SystemLoading built the power-off page ahead (home is the current page); other pages load on entry.
        assert(DataProc::Broker()->search("SaveConfigModel"));
        assert(!DataProc::Broker()->search("WorkSettingsModel"));

        // Before the platform publishes, device fields show N/A; afterwards pages show the real data.
        assert(find_text(lv_screen_active(), "N/A") || on_page("Pages/Dialplate"));
        auto device = real_device();
        assert(set_device(device));
        advance(100);

        // SystemInfos cards from device data (formats as on the former HC32 pages).
        assert(app.show_page("Pages/SystemInfos"));
        advance(500);
        assert(find_text(lv_screen_active(), "22.543096\n114.057865\n8.25 m"));
        assert(find_text(lv_screen_active(), "AP-X1PRO-TEST\nOFF\n10.0.0.7"));
        assert(find_text(lv_screen_active(), "64.5%\n7.71 V\n31.0 C"));
        assert(find_text(lv_screen_active(), "V2.4.4\n01:02:05\n0\n2026-10-19"));
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::WorkModeBase)));
        assert(app.back());
        advance(500);

        // 预设选择不发送；应用后留页，显示等待、超时和主控接收状态。
        device_requests = 0;
        work_requests.clear();
        record_requests.clear();
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        assert(focus_text(i18n::text(i18n::TextId::RadioBase)));
        app.on_input(App::InputAction::Confirm); // Base -> Network base
        assert(device_requests == 0 && find_text(lv_screen_active(), i18n::text(i18n::TextId::NetworkBase)));
        assert(lv_group_get_obj_count(lv_group_get_default()) == 3); // Preset, Apply, Back.
        choose(i18n::TextId::Apply);
        app.on_input(App::InputAction::Confirm);
        assert(on_page("Pages/WorkSettings") && device_requests == 1);
        assert(work_requests.back().target == hal::WorkTarget::Ntrip);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestWaiting)));
        auto* work_dot = activity(lv_screen_active());
        assert(work_dot);
        const auto initial_opacity = lv_obj_get_style_bg_opa(work_dot, LV_PART_MAIN);
        advance(350);
        assert(lv_obj_get_style_bg_opa(work_dot, LV_PART_MAIN) != initial_opacity);
        assert(lv_group_get_obj_count(lv_group_get_default()) == 1);
        app.on_input(App::InputAction::Commit);
        assert(device_requests == 1);
        advance(15500);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestTimeout)));
        device.work.pending = false;
        assert(set_device(device));
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestReceived)));
        assert(!lv_anim_get(work_dot, page::settings_style::pulse));
        assert(app.back());
        advance(500);
        device.work.channel = 0;
        assert(set_device(device));
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        choose(i18n::TextId::Apply);
        app.on_input(App::InputAction::Confirm);
        assert(on_page("Pages/WorkSettings") && device_requests == 1);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::SelectChannel)));
        assert(app.back());
        advance(500);
        device.work.channel = 5;
        assert(set_device(device));
        assert(app.show_page("Pages/WorkSettings"));
        advance(500);
        choose(i18n::TextId::Apply);
        reject_device = true;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests == 1 && find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestRejected)));
        reject_device = false;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests == 2 && work_requests.back().protocol == 3 && work_requests.back().channel == 5);
        assert(set_device(device));
        advance(100);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestReceived)));
        assert(app.back());
        advance(500);

        // 固定参数只读；单击即开始/停止（双击同样有效），等待期间禁止重复请求。
        assert(app.show_page("Pages/RecordConfig"));
        advance(500);
        assert(focus_text(i18n::text(i18n::TextId::StartupStart)));
        reject_device = true;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests == 2 && find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestRejected)));
        reject_device = false;
        app.on_input(App::InputAction::Confirm);
        assert(device_requests == 3 && record_requests.back().action == hal::RecordAction::Start);
        assert(record_requests.back().type == 1 && record_requests.back().interval == 2);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestWaiting)));
        auto* record_dot = activity(lv_screen_active());
        assert(record_dot);
        app.on_input(App::InputAction::Commit); // 等待期间按钮禁用、焦点在返回；双击也不会重复发送。
        assert(device_requests == 3);
        advance(15500);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestTimeout)));
        device.recorder.recording = device.recorder.reported_recording = true;
        assert(set_device(device));
        advance(100);
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::RequestReceived)));
        choose(i18n::TextId::Stop);
        app.on_input(App::InputAction::Confirm);
        assert(device_requests == 4 && record_requests.back().action == hal::RecordAction::Stop);
        device.recorder.recording = device.recorder.reported_recording = false;
        assert(set_device(device));
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
        assert(wifi_requests == 1 && requested_wifi && hal_test::snapshot<dp::WiFi>().pending);
        advance(50);
        assert(find_text(lv_screen_active(), "..."));
        auto* wifi_dot = activity(lv_screen_active());
        assert(wifi_dot);
        app.on_input(App::InputAction::Confirm); // Ignored while pending.
        assert(wifi_requests == 1);
        assert(wifi(true, false));
        assert(!lv_anim_get(wifi_dot, page::settings_style::pulse));
        advance(50);
        assert(!find_text(lv_screen_active(), "...") && !hal_test::snapshot<dp::WiFi>().pending);
        assert(hal_test::snapshot<dp::WiFi>().on);
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
        assert(!find_text(lv_screen_active(), i18n::text(i18n::TextId::RebootAction)));
        assert(lv_group_get_obj_count(lv_group_get_default()) == 1);
        app.on_input(App::InputAction::Commit); // Double-click no longer confirms anything.
        advance(100);
        assert(on_page("Pages/SystemDash") && !hal_test::snapshot<dp::Power>().pending);
        assert(find_text(app.focused(), i18n::text(i18n::TextId::ShutdownAction)));
        app.on_input(App::InputAction::Press); // Released before 100%: the dialog closes, nothing prepared.
        advance(1500);
        app.on_input(App::InputAction::Release);
        advance(100);
        assert(focus_text(i18n::text(i18n::TextId::PowerTitle)) && !hal_test::snapshot<dp::Power>().pending);
        assert(!find_text(lv_screen_active(), i18n::text(i18n::TextId::ShutdownAction)));
        app.on_input(App::InputAction::Confirm);
        advance(100);
        // Service-level guard: without platform reboot, even SystemDash's own Reboot request is rejected.
        auto* dash = DataProc::Broker()->search("SystemDashModel");
        assert(dash
               && dp::Power::notify(*dash, {dp::PowerOperation::Prepare, hal::PowerCommand::Reboot})
                      == DataNode::RES_PARAM_ERROR);
        assert(!hal_test::snapshot<dp::Power>().pending);
        assert(find_text(app.focused(), i18n::text(i18n::TextId::ShutdownAction)));

        // User shutdown: a completed hold prepares it, SaveConfig executes it, then the page waits for standby.
        app.on_input(App::InputAction::Press);
        advance(1500);
        assert(on_page("Pages/SystemDash") && !hal_test::snapshot<dp::Power>().pending);
        advance(600);
        assert(hal_test::snapshot<dp::Power>().pending);
        // The dialog stays unchanged under the sliding SaveConfig; it closes only after the transition.
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::ShutdownAction)));
        advance(400);
        assert(on_page("Pages/SaveConfig"));
        app.on_input(App::InputAction::Release);
        advance(10000);
        assert(off_calls == 1 && reboot_calls == 0 && on_page("Pages/SaveConfig"));
        assert(shutdown(hal::PowerCause::User)); // e.g. HAL ShutdownGoing.
        advance(100);
        assert(app.back() && on_page("Pages/SaveConfig")); // Back is consumed during a shutdown.
        assert(standby());
        advance(600);
        assert(on_page("Pages/Startup") && !app.back());

        // A platform step that never finishes cannot block startup: it is recorded as failed after the page's
        // limit and the working root is still reached (power on/off keep working meanwhile).
        stall_last_step = true;
        assert(power_on());
        advance(200);
        assert(on_page("Pages/SystemLoading"));
        advance(page::SystemLoadingModel::kStepLimitMs);
        assert(on_page("Pages/SystemLoading"));
        advance(5000);
        assert(on_page("Pages/Dialplate"));
        const auto stalled = hal_test::snapshot<dp::Init>().results[4]; // snapshot() returns a copy.
        assert(stalled.status == DataProc::InitializationStatus::Failed
               && std::strstr(stalled.detail.data(), "no result within"));
        stall_last_step = false;

        // Platform shutdown (host request): SaveConfig shows the cause, cannot be left, standby returns.
        assert(shutdown(hal::PowerCause::Host));
        advance(600);
        assert(on_page("Pages/SaveConfig"));
        assert(find_text(lv_screen_active(), i18n::text(i18n::TextId::SaveConfigPowerOffBoard)));
        assert(app.back() && on_page("Pages/SaveConfig"));
        advance(10000);
        assert(off_calls == 1 && on_page("Pages/SaveConfig")); // Nothing is executed for a platform shutdown.
        assert(standby());
        advance(600);
        assert(on_page("Pages/Startup"));
        assert(!shutdown(hal::PowerCause::Forced)); // Already in standby.
        active = nullptr;
    }
    lv_display_delete(display);
    std::puts("app_platform: OK");
    return 0;
}
