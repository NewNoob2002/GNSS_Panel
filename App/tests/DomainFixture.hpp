#pragma once
#include "HAL_Manager.h"
#include "PowerObject.h"
#include "Def/HAL_Board.h"
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
    bool (*execute_power)(hal::PowerCommand) = nullptr;
    bool (*execute_work)(const hal::WorkCommand&) = nullptr;
    bool (*execute_record)(const hal::RecorderCommand&) = nullptr;
    bool (*request_wifi)(bool) = nullptr;
    hal::InitStatus (*poll)(unsigned,uint32_t,char*,size_t) = nullptr;
    const char* (*name)(unsigned) = nullptr;
};
inline State source;
template<class S> class Read final : public hal::ReadObject<S> {
    S& value_;
public:
    Read(const char* name, S& value) : hal::ReadObject<S>(name), value_(value) {}
    S state() override { return value_; }
};
class Power final : public hal::PowerObject {
public:
    hal::PowerState power_report() override { return source.power; }
    bool request_power_on() override {
        if (source.request_on) return source.request_on();
        source.power = {true,false,false,hal::PowerCause::User}; return true;
    }
    bool execute_power(hal::PowerCommand c) override { return !source.execute_power || source.execute_power(c); }
};
class Work final : public hal::WriteObject<hal::WorkState,hal::WorkCommand> {
public:
    Work() : WriteObject(hal::names::Work) {}
    hal::WorkState state() override { return source.work; }
    int execute(const hal::WorkCommand& c) override {
        if ((c.target != hal::WorkTarget::Rover && c.target != hal::WorkTarget::Base && c.target != hal::WorkTarget::Ntrip)
            || c.protocol >= hal::kRadioProtocolCount || c.channel >= hal::kRadioChannelCount
            || (c.target != hal::WorkTarget::Ntrip && !c.channel)) return RES_PARAM_ERROR;
        if (source.execute_work) return source.execute_work(c) ? RES_OK : RES_UNKNOWN;
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
        if (source.execute_record) return source.execute_record(c) ? RES_OK : RES_UNKNOWN;
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
/** @brief 替换细设备值而保留平台能力、回调与异步 Wi-Fi 状态。 */
inline bool set_domains(const State& d) {
    if (d.recorder.name.back() || d.wifi.ssid.back() || d.board.firmware.back()
        || d.board.hardware.back() || d.board.build.back()) return false;
    source.battery = d.battery;
    source.gnss = d.gnss;
    source.work = d.work;
    source.recorder = d.recorder;
    source.wifi.valid = d.wifi.valid; source.wifi.station = d.wifi.station;
    source.wifi.ssid = d.wifi.ssid; source.wifi.ip = d.wifi.ip;
    source.board.firmware = d.board.firmware; source.board.hardware = d.board.hardware;
    source.board.build = d.board.build; source.board.uptime_s = d.board.uptime_s;
    return true;
}
}
