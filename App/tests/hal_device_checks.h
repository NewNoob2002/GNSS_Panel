#pragma once
#include <array>
#include <cassert>
#include <cstring>
#include "DataProc.h"
#include "HAL_Manager.h"
#include "Def/DP_Power.h"
#include "Def/DP_Battery.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Clock.h"
#include "Def/DP_Work.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Version.h"
#include "Def/DP_Init.h"
#include "Def/DP_Settings.h"
#include "Helper/NodeHelper.h"
#include "Def/HAL_Battery.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_Work.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Init.h"

namespace hal_test {
/** @brief 从真实领域节点同步读取，避免测试重新组成聚合快照。 */
template<class Domain> typename Domain::State snapshot() {
    typename Domain::State value;
    assert(Domain::pull(*DataProc::Broker()->mainNode(), value) == DataNode::RES_OK);
    return value;
}
inline DeviceObject* device(const char* name) {
    auto* d = hal::manager().getDevice(name);
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
    shape<hal::BoardState, hal::BrightnessCommand>(hal::names::Board, true);
    for (uint8_t percent : {0, 19, 101, 255})
        assert(hal::write(device(hal::names::Board), hal::BrightnessCommand{percent}) == DeviceObject::RES_PARAM_ERROR);
    auto* init = device(hal::names::Init);
    hal::InitInfo info;
    assert(init->read(&info, sizeof(info)) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(init->write(&info, sizeof(info)) == DeviceObject::RES_NO_IMPLEMENTED);
    assert(init->ioctl(hal::kInitializationInfo, &info, sizeof(info) - 1) == DeviceObject::RES_PARAM_ERROR);
    assert(init->ioctl(hal::kInitializationInfo, &info, sizeof(info) + 1) == DeviceObject::RES_PARAM_ERROR);
    assert(init->ioctl(hal::kInitializationInfo, nullptr, sizeof(info)) == DeviceObject::RES_PARAM_ERROR);
    // 畸形编码仍携带其声明的完整缓冲，避免优化器把拒绝路径误判为越界复制。
    std::array<char, sizeof(hal::InitPoll) + 1> invalid_payload{};
    for (uint32_t mask : {1U << 30, 1U << 16, 1U << 8, 1U}) {
        const uint32_t cmd = hal::kInitializationInfo ^ mask;
        const size_t size = (cmd >> 16) & 0x3fffU;
        assert(init->ioctl(cmd, invalid_payload.data(), size) == DeviceObject::RES_UNSUPPORT);
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
        assert(init->ioctl(cmd, invalid_payload.data(), size) == DeviceObject::RES_UNSUPPORT);
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
/** @brief HAL 与领域逐字段比较（含全部数组、有效位与 pending），不经过聚合转换。 */
inline uint32_t frozen_tick() { return 0; }
inline void equivalent() {
    assert(DataProc_Init(frozen_tick));
    assert(!snapshot<dp::Power>().pending && snapshot<dp::Power>().calls == 0);
    assert(!snapshot<dp::Init>().started);
    assert(snapshot<dp::Settings>().language == i18n::get_language());
    assert(snapshot<dp::Settings>().brightness == read<hal::BoardState>(hal::names::Board).brightness);
    assert(snapshot<dp::Battery>() == read<hal::BatteryState>(hal::names::Battery));
    assert(snapshot<dp::GNSS>() == read<hal::GNSSState>(hal::names::GNSS));
    assert(snapshot<dp::Clock>().clock == read<hal::ClockState>(hal::names::Clock));
    assert(snapshot<dp::Clock>().clock.source == hal::ClockSource::Uptime);
    assert(snapshot<dp::Work>() == read<hal::WorkState>(hal::names::Work));
    assert(snapshot<dp::Recorder>() == read<hal::RecorderState>(hal::names::Recorder));
    assert(snapshot<dp::WiFi>() == read<hal::WifiState>(hal::names::WiFi));
    assert(snapshot<dp::Version>() == read<hal::BoardState>(hal::names::Board));
    assert(snapshot<dp::WiFi>().available == snapshot<dp::Version>().capabilities.wifi_available);
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
    field(b, &hal::BatteryState::current_ma, int16_t{-350}); field(b, &hal::BatteryState::current_valid, true);
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
    field(board, &hal::BoardState::brightness, uint8_t{20});
    caps.star_map = true; field(board, &hal::BoardState::capabilities, caps);
}
} // namespace hal_test
