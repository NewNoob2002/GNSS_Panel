#include <cstdio>
#include "Def/HAL_Init.h"
#include "DemoModel.h"
#include "HAL_Manager.h"
#include "PowerObject.h"

namespace {
class DemoPower final : public hal::PowerObject {
  public:
    hal::PowerState
    power_report() override {
        return demo::model().power;
    }

    bool
    request_power_on() override {
        demo::model().power = {true, false, false, hal::PowerCause::User};
        return true;
    }

    bool
    execute_power(hal::PowerCommand action) override {
        if (action == hal::PowerCommand::Off) {
            demo::model().power = {false, false, true, hal::PowerCause::User};
        }
        return true;
    }
};

template <class State, State demo::Model::* Field>
class DemoRead final : public hal::ReadObject<State> {
  public:
    explicit DemoRead(const char* name) : hal::ReadObject<State>(name) {}

    State
    state() override {
        demo::refresh();
        return demo::model().*Field;
    }
};

class DemoWork final : public hal::WriteObject<hal::WorkState, hal::WorkCommand> {
  public:
    DemoWork() : WriteObject(hal::names::Work) {}

    hal::WorkState
    state() override {
        demo::refresh();
        return demo::model().work;
    }

    int
    execute(const hal::WorkCommand& c) override {
        if ((c.target != hal::WorkTarget::Rover && c.target != hal::WorkTarget::Base
             && c.target != hal::WorkTarget::Ntrip)
            || c.protocol >= hal::kRadioProtocolCount || c.channel >= hal::kRadioChannelCount
            || (c.target != hal::WorkTarget::Ntrip && !c.channel)) {
            return RES_PARAM_ERROR;
        }
        auto& work = demo::model().work;
        work.ntrip = c.target == hal::WorkTarget::Ntrip ? hal::NtripRole::Server : hal::NtripRole::None;
        work.mode = c.target == hal::WorkTarget::Rover ? hal::WorkMode::Rover : hal::WorkMode::Base;
        work.radio_on = c.target != hal::WorkTarget::Ntrip;
        if (work.radio_on) {
            work.protocol = c.protocol;
            work.channel = c.channel;
        }
        return RES_OK;
    }
};

class DemoRecorder final : public hal::WriteObject<hal::RecorderState, hal::RecorderCommand> {
  public:
    DemoRecorder() : WriteObject(hal::names::Recorder) {}

    hal::RecorderState
    state() override {
        demo::refresh();
        return demo::model().recorder;
    }

    int
    execute(const hal::RecorderCommand& c) override {
        if (c.action != hal::RecordAction::Start && c.action != hal::RecordAction::Stop) {
            return RES_PARAM_ERROR;
        }
        if (c.action == hal::RecordAction::Start
            && (c.type >= hal::kRecordTypeCount || c.interval >= hal::kRecordIntervalCount)) {
            return RES_PARAM_ERROR;
        }
        auto& record = demo::model().recorder;
        record.recording = record.reported_recording = c.action == hal::RecordAction::Start;
        if (record.recording) {
            record.type = c.type;
            record.interval = c.interval;
        }
        return RES_OK;
    }
};

class DemoWifi final : public hal::WriteObject<hal::WifiState, hal::WifiCommand> {
  public:
    DemoWifi() : WriteObject(hal::names::WiFi) {}

    hal::WifiState
    state() override {
        demo::refresh();
        return demo::model().wifi;
    }

    int
    execute(const hal::WifiCommand&) override {
        return RES_UNSUPPORT;
    }
};

class DemoInit final : public hal::InitObject {
  public:
    void
    info(hal::InitInfo& v) override {
        constexpr const char* names[] = {"configuration", "services", "gnss", "network", "finalization"};
        constexpr hal::InitStage stages[] = {hal::InitStage::Configuration, hal::InitStage::Services,
                                             hal::InitStage::GNSS, hal::InitStage::Network,
                                             hal::InitStage::Finalization};
        v.name = names[v.step];
        v.stage = stages[v.step];
    }

    void
    poll(hal::InitPoll& v) override {
        constexpr uint32_t durations[] = {0, 400, 1200, 700, 200};
        constexpr const char* details[] = {"Rover, TRIMTALK, 455.05 MHz defaults loaded",
                                           "status and logging services prepared", "GNSS receiver placeholder prepared",
                                           "Wi-Fi backend reserved; continue offline", "working UI prepared"};
        if (v.elapsed_ms < durations[v.step]) {
            v.status = hal::InitStatus::Pending;
            return;
        }
        v.status = v.step == 3 ? hal::InitStatus::Failed : hal::InitStatus::Ok;
        std::snprintf(v.detail.data(), v.detail.size(), "%s", details[v.step]);
    }
};
} // namespace

DeviceManager&
hal::manager() {
    static DemoPower power;
    static DemoRead<hal::BatteryState, &demo::Model::battery> battery(hal::names::Battery);
    static DemoRead<hal::GNSSState, &demo::Model::gnss> gnss(hal::names::GNSS);
    static DemoRead<hal::ClockState, &demo::Model::clock> clock(hal::names::Clock);
    static DemoWork work;
    static DemoRecorder recorder;
    static DemoWifi wifi;
    static DemoRead<hal::BoardState, &demo::Model::board> board(hal::names::Board);
    static DemoInit init;
    static DeviceObject* list[] = {&power, &battery, &gnss, &clock, &work, &recorder, &wifi, &board, &init};
    static DeviceManager manager(list, sizeof(list) / sizeof(list[0]));
    static const bool initialized = (manager.init(), true);
    (void)initialized;
    return manager;
}
