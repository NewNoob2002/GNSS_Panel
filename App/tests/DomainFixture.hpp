#pragma once
#include "SystemBackend.h"
#include "Def/HAL_Battery.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_Work.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Init.h"

namespace fixture {
/** @brief 可变细设备测试源；测试先修改状态，再由真实 DataBroker 定时器拉取。 */
struct State {
    hal::PowerState power{};
    hal::BatteryState battery{};
    hal::GNSSState gnss{};
    hal::ClockState clock{};
    hal::WorkState work{};
    hal::RecorderState recorder{};
    hal::WifiState wifi{};
    hal::BoardState board{};
    bool (*request_on)() = nullptr;
    bool (*execute_power)(DataProc::PowerAction) = nullptr;
    bool (*execute_device)(const DataProc::DeviceRequest&) = nullptr;
    bool (*request_wifi)(bool) = nullptr;
    DataProc::InitializationStatus (*poll)(unsigned,uint32_t,char*,size_t) = nullptr;
    const char* (*name)(unsigned) = nullptr;
};
inline State source;
template<class S> class Read final : public hal::ReadObject<S> {
    S& value_;
public:
    Read(const char* name, S& value) : hal::ReadObject<S>(name), value_(value) {}
    S state() override { return value_; }
};
class Power final : public system_backend::PowerObject {
public:
    system_backend::PowerReport power_report() override { return source.power; }
    bool request_power_on() override {
        if (source.request_on) return source.request_on();
        source.power = {true,false,false,hal::PowerCause::User}; return true;
    }
    bool execute_power(DataProc::PowerAction c) override { return !source.execute_power || source.execute_power(c); }
};
class Work final : public hal::WriteObject<hal::WorkState,hal::WorkCommand> {
public:
    Work() : WriteObject(hal::names::Work) {}
    hal::WorkState state() override { return source.work; }
    int execute(const hal::WorkCommand& c) override {
        if ((c.target != hal::WorkTarget::Rover && c.target != hal::WorkTarget::Base && c.target != hal::WorkTarget::Ntrip)
            || c.protocol >= hal::kRadioProtocolCount || c.channel >= hal::kRadioChannelCount
            || (c.target != hal::WorkTarget::Ntrip && !c.channel)) return RES_PARAM_ERROR;
        DataProc::DeviceRequest r;
        r.target = static_cast<DataProc::WorkTarget>(c.target); r.radio_protocol = c.protocol; r.radio_channel = c.channel;
        if (source.execute_device) return source.execute_device(r) ? RES_OK : RES_UNKNOWN;
        source.work.mode = c.target == hal::WorkTarget::Rover ? hal::WorkMode::Rover : hal::WorkMode::Base;
        source.work.protocol = c.protocol; source.work.channel = c.channel;
        return RES_OK;
    }
};
class Recorder final : public hal::WriteObject<hal::RecorderState,hal::RecorderCommand> {
public:
    Recorder() : WriteObject(hal::names::Recorder) {}
    hal::RecorderState state() override { return source.recorder; }
    int execute(const hal::RecorderCommand& c) override {
        if (c.action != hal::RecordAction::Start && c.action != hal::RecordAction::Stop) return RES_PARAM_ERROR;
        if (c.action == hal::RecordAction::Start && (c.type >= hal::kRecordTypeCount || c.interval >= hal::kRecordIntervalCount)) return RES_PARAM_ERROR;
        DataProc::DeviceRequest r;
        r.command = c.action == hal::RecordAction::Start ? DataProc::DeviceCommand::StartRecord : DataProc::DeviceCommand::StopRecord;
        r.record_type = c.type; r.record_interval = c.interval;
        if (source.execute_device) return source.execute_device(r) ? RES_OK : RES_UNKNOWN;
        source.recorder.recording = source.recorder.reported_recording = c.action == hal::RecordAction::Start;
        source.recorder.type = c.type; source.recorder.interval = c.interval;
        return RES_OK;
    }
};
class Wifi final : public hal::WriteObject<hal::WifiState,hal::WifiCommand> {
public:
    Wifi() : WriteObject(hal::names::WiFi) {}
    hal::WifiState state() override { return source.wifi; }
    int execute(const hal::WifiCommand& c) override {
        return source.request_wifi && source.request_wifi(c.on) ? RES_OK : RES_UNSUPPORT;
    }
};
class Init final : public hal::InitObject {
public:
    void info(hal::InitInfo& v) override {
        constexpr hal::InitStage stages[] = {hal::InitStage::Configuration,hal::InitStage::BatteryGauge,
            hal::InitStage::Charger,hal::InitStage::HostPower,hal::InitStage::HostLink};
        v.stage = stages[v.step]; v.name = source.name ? source.name(v.step) : "test";
    }
    void poll(hal::InitPoll& v) override {
        v.status = source.poll ? static_cast<hal::InitStatus>(source.poll(v.step,v.elapsed_ms,v.detail.data(),v.detail.size())) : hal::InitStatus::Ok;
    }
};
inline DeviceManager& devices() {
    static Power power;
    static Read<hal::BatteryState> battery(hal::names::Battery, source.battery);
    static Read<hal::GNSSState> gnss(hal::names::GNSS, source.gnss);
    static Read<hal::ClockState> clock(hal::names::Clock, source.clock);
    static Work work; static Recorder recorder; static Wifi wifi;
    static Read<hal::BoardState> board(hal::names::Board, source.board); static Init init;
    static DeviceObject* list[] = {&power,&battery,&gnss,&clock,&work,&recorder,&wifi,&board,&init};
    static DeviceManager manager(list,9);
    static const bool initialized = (manager.init(),true); (void)initialized;
    return manager;
}
/** @brief 将既有测试样例映射为细设备状态，不向 DataProc 发布。 */
inline bool set_device(const DataProc::DeviceSnapshot& d) {
    if (d.record_name.back() || d.wifi_ssid.back() || d.firmware.back() || d.hardware.back() || d.build.back()) return false;
    source.battery = {d.power_valid, static_cast<unsigned>(d.battery_percent),d.battery_percent,d.battery_voltage,
        d.power_valid,d.battery_temperature,static_cast<hal::ChargeState>(d.charge),d.external_power};
    source.gnss = {d.position_valid,d.latitude,d.longitude,d.altitude,d.satellites_valid,d.satellites_used,
        d.satellites_tracked,d.constellations_valid,d.constellations,d.position_valid ? hal::FixStatus::Fixed : hal::FixStatus::None};
    source.work = {d.work_valid,d.work_pending,static_cast<hal::WorkMode>(d.work_mode),d.radio_on,d.radio_protocol,d.radio_channel,static_cast<hal::NtripRole>(d.ntrip)};
    source.recorder = {d.record_valid,d.recording,d.record_pending,d.record_type,d.record_interval,d.record_name,d.record_space_mb,d.recording};
    source.wifi.valid = d.wifi_valid; source.wifi.station = d.wifi_station; source.wifi.ssid = d.wifi_ssid; source.wifi.ip = d.wifi_ip;
    source.board.firmware = d.firmware; source.board.hardware = d.hardware; source.board.build = d.build; source.board.uptime_s = d.uptime_s;
    return true;
}
}
