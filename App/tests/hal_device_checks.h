#pragma once
#include <cassert>
#include <cstring>
#include "SystemBackend.h"
#include "SystemService.h"
#include "Helper/NodeHelper.h"
#include "Def/HAL_Battery.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_Work.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Init.h"

namespace hal_test {
inline DeviceObject* device(const char* name) {
    auto* d = system_backend::devices().getDevice(name);
    assert(d);
    return d;
}
template<class State> State read(const char* name) {
    State value;
    assert(hal::read(device(name), value) == DeviceObject::RES_OK);
    return value;
}
/** 验证精确尺寸、空指针、读写方向及 ioctl 完整编码。 */
template<class State, class Command = State> void shape(const char* name, bool writable = false) {
    auto* d = device(name);
    State value;
    assert(hal::read(d, value) == DeviceObject::RES_OK);
    assert(d->read(&value, sizeof(value) - 1) == DeviceObject::RES_PARAM_ERROR);
    assert(d->read(&value, sizeof(value) + 1) == DeviceObject::RES_PARAM_ERROR);
    assert(d->read(nullptr, sizeof(value)) == DeviceObject::RES_PARAM_ERROR);
    Command command{};
    const int bad = writable ? DeviceObject::RES_PARAM_ERROR : DeviceObject::RES_NO_IMPLEMENTED;
    assert(d->write(&command, sizeof(command) - 1) == bad);
    assert(d->write(&command, sizeof(command) + 1) == bad);
    assert(d->write(nullptr, sizeof(command)) == DeviceObject::RES_PARAM_ERROR);
    if (!writable) assert(hal::write(d, command) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(d->ioctl(0) == DeviceObject::RES_UNSUPPORT || d->ioctl(0) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(d->ioctl(hal::kRequestPowerOn ^ (1U << 30)) != DeviceObject::RES_OK);
    const State original = value;
    assert(hal::read(static_cast<DeviceObject*>(nullptr), value) == DeviceObject::RES_INIT_FAILED);
    assert(value == original);
}
inline void shapes() {
    shape<hal::PowerState, hal::PowerCommand>(hal::names::Power, true);
    shape<hal::BatteryState>(hal::names::Battery);
    shape<hal::GNSSState>(hal::names::GNSS);
    shape<hal::ClockState>(hal::names::Clock);
    shape<hal::WorkState, hal::WorkCommand>(hal::names::Work, true);
    shape<hal::RecorderState, hal::RecorderCommand>(hal::names::Recorder, true);
    shape<hal::WifiState, hal::WifiCommand>(hal::names::WiFi, true);
    shape<hal::BoardState>(hal::names::Board);
    auto* init = device(hal::names::Init);
    hal::InitInfo info;
    assert(init->read(&info, sizeof(info)) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(init->write(&info, sizeof(info)) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(init->ioctl(hal::kInitializationInfo, &info, sizeof(info) - 1) == DeviceObject::RES_PARAM_ERROR);
    assert(init->ioctl(hal::kInitializationInfo, &info, sizeof(info) + 1) == DeviceObject::RES_PARAM_ERROR);
    assert(init->ioctl(hal::kInitializationInfo, nullptr, sizeof(info)) == DeviceObject::RES_PARAM_ERROR);
    for (uint32_t mask : {1U << 30, 1U << 16, 1U << 8, 1U}) {
        const uint32_t cmd = hal::kInitializationInfo ^ mask;
        const size_t size = (cmd >> 16) & 0x3fffU;
        assert(init->ioctl(cmd, &info, size) == DeviceObject::RES_UNSUPPORT);
    }
    info.step = hal::kInitializationStepCount;
    assert(hal::ioctl(init, hal::kInitializationInfo, info) == DeviceObject::RES_PARAM_ERROR);
    assert(hal::ioctl(init, hal::kPollInitialization, info) == DeviceObject::RES_PARAM_ERROR);
    hal::InitPoll poll;
    poll.step = hal::kInitializationStepCount;
    assert(hal::ioctl(init, hal::kPollInitialization, poll) == DeviceObject::RES_PARAM_ERROR);
    for (uint32_t mask : {1U << 30, 1U << 16, 1U << 8, 1U}) {
        const uint32_t cmd = hal::kPollInitialization ^ mask;
        const size_t size = (cmd >> 16) & 0x3fffU;
        assert(init->ioctl(cmd, &poll, size) == DeviceObject::RES_UNSUPPORT);
    }
    assert(hal::write(device(hal::names::Power), static_cast<hal::PowerCommand>(99)) == DeviceObject::RES_PARAM_ERROR);
    const hal::WorkCommand invalid_work[] = {{static_cast<hal::WorkTarget>(99), 0, 1},
        {hal::WorkTarget::Base, 8, 1}, {hal::WorkTarget::Rover, 0, 10}, {hal::WorkTarget::Rover, 0, 0},
        {hal::WorkTarget::Ntrip, 8, 0}};
    for (const auto& command : invalid_work)
        assert(hal::write(device(hal::names::Work), command) == DeviceObject::RES_PARAM_ERROR);
    const hal::RecorderCommand invalid_record[] = {{static_cast<hal::RecordAction>(99), 0, 0},
        {hal::RecordAction::Start, 2, 0}, {hal::RecordAction::Start, 0, 6}};
    for (const auto& command : invalid_record)
        assert(hal::write(device(hal::names::Recorder), command) == DeviceObject::RES_PARAM_ERROR);
}
/** 所有可对应字段逐项比较，避免只检查少数代表字段。 */
inline void equivalent() {
    assert(DataProc_Init(+[]() -> uint32_t { return 0; }));
    const auto d = DeviceService::snapshot();
    DataProc::StatusSnapshot s;
    assert(DataProc::Broker()->mainNode()->pull(dp::names::Status, &s, sizeof(s)) == DataNode::RES_OK);
    const auto system = SystemService::snapshot();
    const auto battery = read<hal::BatteryState>(hal::names::Battery);
    assert(battery.valid == d.power_valid && battery.level_percent == s.battery_percent);
    assert(battery.gauge_percent == d.battery_percent && battery.voltage == d.battery_voltage);
    assert(battery.temperature_valid == s.battery_temperature_valid && battery.temperature == d.battery_temperature);
    assert(static_cast<int>(battery.charge) == static_cast<int>(d.charge) && battery.external_power == d.external_power);
    const auto gnss = read<hal::GNSSState>(hal::names::GNSS);
    assert(gnss.position_valid == d.position_valid && gnss.latitude == d.latitude && gnss.longitude == d.longitude);
    assert(gnss.altitude == d.altitude && gnss.satellites_valid == d.satellites_valid);
    assert(gnss.satellites_used == d.satellites_used && gnss.satellites_tracked == d.satellites_tracked);
    assert(gnss.constellations_valid == d.constellations_valid && gnss.constellations == d.constellations);
    const auto clock = read<hal::ClockState>(hal::names::Clock);
    assert(clock.time_valid == s.clock_valid && clock.hour == s.hour && clock.minute == s.minute && clock.second == s.second);
    assert(clock.source == hal::ClockSource::Uptime);
    const auto work = read<hal::WorkState>(hal::names::Work);
    assert(work.valid == d.work_valid && work.pending == d.work_pending && work.radio_on == d.radio_on);
    assert(static_cast<int>(work.mode) == static_cast<int>(d.work_mode));
    assert(work.protocol == d.radio_protocol && work.channel == d.radio_channel);
    assert(static_cast<int>(work.ntrip) == static_cast<int>(d.ntrip));
    const auto record = read<hal::RecorderState>(hal::names::Recorder);
    assert(record.valid == d.record_valid && record.recording == d.recording && record.pending == d.record_pending);
    assert(record.type == d.record_type && record.interval == d.record_interval && record.name == d.record_name);
    assert(record.space_mb == d.record_space_mb);
    const auto wifi = read<hal::WifiState>(hal::names::WiFi);
    assert(wifi.valid == d.wifi_valid && wifi.station == d.wifi_station && wifi.ssid == d.wifi_ssid && wifi.ip == d.wifi_ip);
    assert(wifi.on == system.wifi_on && wifi.pending == system.wifi_pending);
    const auto board = read<hal::BoardState>(hal::names::Board);
    assert(board.firmware == d.firmware && board.hardware == d.hardware && board.build == d.build && board.uptime_s == d.uptime_s);
    assert(board.capabilities.simulated == system.simulated && board.capabilities.reboot_available == system.reboot_available
        && board.capabilities.star_map == system.star_map && board.capabilities.record_settings == system.record_settings);
    assert(wifi.available == board.capabilities.wifi_available);
    assert(record.reported_recording == s.recording);
    assert(DataProc_Deinit());
}
/** 逐字段扰动：相等运算不能漏掉成员，也不能比较结构体填充字节。 */
template<class State, class Field> void field(const State& original, Field State::*member, const Field& changed) {
    State other = original;
    assert(other == original && !(other != original));
    other.*member = changed;
    assert(other != original && !(other == original));
}
inline void equality() {
    hal::PowerState p;
    field(p, &hal::PowerState::powered_on, true); field(p, &hal::PowerState::shutting_down, true);
    field(p, &hal::PowerState::standby, true); field(p, &hal::PowerState::cause, hal::PowerCause::Host);
    hal::BatteryState b;
    field(b, &hal::BatteryState::valid, true); field(b, &hal::BatteryState::level_percent, 1U);
    field(b, &hal::BatteryState::gauge_percent, 1.f); field(b, &hal::BatteryState::voltage, 1.f);
    field(b, &hal::BatteryState::temperature_valid, true); field(b, &hal::BatteryState::temperature, 1.f);
    field(b, &hal::BatteryState::charge, hal::ChargeState::Fast); field(b, &hal::BatteryState::external_power, true);
    hal::GNSSState g;
    field(g, &hal::GNSSState::position_valid, true); field(g, &hal::GNSSState::latitude, 1.);
    field(g, &hal::GNSSState::longitude, 1.); field(g, &hal::GNSSState::altitude, 1.);
    field(g, &hal::GNSSState::satellites_valid, true); field(g, &hal::GNSSState::satellites_used, 1U);
    field(g, &hal::GNSSState::satellites_tracked, 1U); field(g, &hal::GNSSState::constellations_valid, true);
    auto stars = g.constellations;
    for (size_t i = 0; i < stars.size(); ++i) { stars = {}; stars[i] = 1; field(g, &hal::GNSSState::constellations, stars); }
    field(g, &hal::GNSSState::fix, hal::FixStatus::Fixed);
    hal::ClockState c;
    field(c, &hal::ClockState::time_valid, true); field(c, &hal::ClockState::hour, 1U);
    field(c, &hal::ClockState::minute, 1U); field(c, &hal::ClockState::second, 1U);
    field(c, &hal::ClockState::source, hal::ClockSource::Host); field(c, &hal::ClockState::uptime_ms, uint64_t{1});
    hal::WorkState w;
    field(w, &hal::WorkState::valid, true); field(w, &hal::WorkState::pending, true);
    field(w, &hal::WorkState::mode, hal::WorkMode::Base); field(w, &hal::WorkState::radio_on, true);
    field(w, &hal::WorkState::protocol, 1U); field(w, &hal::WorkState::channel, 1U); field(w, &hal::WorkState::ntrip, hal::NtripRole::Client);
    hal::RecorderState r;
    field(r, &hal::RecorderState::valid, true); field(r, &hal::RecorderState::recording, true);
    field(r, &hal::RecorderState::reported_recording, true);
    field(r, &hal::RecorderState::pending, true); field(r, &hal::RecorderState::type, 1U);
    field(r, &hal::RecorderState::interval, 1U); auto name = r.name; name[15] = 'x';
    field(r, &hal::RecorderState::name, name); field(r, &hal::RecorderState::space_mb, 1.f);
    hal::WifiState f;
    field(f, &hal::WifiState::valid, true); field(f, &hal::WifiState::available, true); field(f, &hal::WifiState::on, true);
    field(f, &hal::WifiState::pending, true); field(f, &hal::WifiState::station, true);
    auto ssid = f.ssid; ssid[15] = 'x'; field(f, &hal::WifiState::ssid, ssid);
    auto ip = f.ip; ip[3] = 1; field(f, &hal::WifiState::ip, ip);
    hal::Capabilities caps;
    field(caps, &hal::Capabilities::simulated, true); field(caps, &hal::Capabilities::reboot_available, true);
    field(caps, &hal::Capabilities::wifi_available, true); field(caps, &hal::Capabilities::star_map, true);
    field(caps, &hal::Capabilities::record_settings, true);
    hal::BoardState board;
    auto fw = board.firmware; fw[14] = 'x'; field(board, &hal::BoardState::firmware, fw);
    field(board, &hal::BoardState::hardware, fw); auto build = board.build; build[22] = 'x';
    field(board, &hal::BoardState::build, build); field(board, &hal::BoardState::uptime_s, uint32_t{1});
    caps.star_map = true; field(board, &hal::BoardState::capabilities, caps);
}
} // namespace hal_test
