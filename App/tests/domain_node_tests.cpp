#include "DomainFixture.hpp"
#include "hal_device_checks.h"
#include "Def/DP_Battery.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Clock.h"
#include "Def/DP_Work.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Version.h"
#include "Def/DP_Power.h"
#include "Def/DP_Init.h"
#include "Def/DP_Settings.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>

namespace {
uint32_t tick = 0;
uint32_t clock_ms() { return tick; }
void advance(uint32_t ms) { tick += ms; DataProc_RunLoop(); }
std::map<std::string,unsigned> publishes;
int receive(DataNode*,DataNode::EventParam_t* event) {
    assert(event && event->event == DataNode::EVENT_PUBLISH && event->tran);
    ++publishes[event->tran->getID()]; return DataNode::RES_OK;
}
template<class Node> typename Node::State pull(DataNode& caller) {
    typename Node::State result;
    assert(Node::pull(caller,result) == DataNode::RES_OK); return result;
}
unsigned init_polls = 0;
hal::InitStatus probe(unsigned step, uint32_t elapsed, char* detail, size_t size) {
    ++init_polls;
    std::snprintf(detail, size, "step %u at %lu", step, static_cast<unsigned long>(elapsed));
    return elapsed < 400 ? hal::InitStatus::Pending : step == 3 ? hal::InitStatus::Failed : hal::InitStatus::Ok;
}
void init_queries(DataNode& caller) {
    const auto original = pull<dp::Init>(caller);
    fixture::source.poll = probe;
    constexpr i18n::TextId titles[] = {i18n::TextId::LoadingConfiguration, i18n::TextId::CheckingBatteryGauge,
        i18n::TextId::CheckingCharger, i18n::TextId::PoweringHostBoard, i18n::TextId::CheckingHostLink};
    for (unsigned step = 0; step < DataProc::kInitializationStepCount; ++step) {
        dp::InitStepInfo info;
        assert(dp::Init::info(caller, step, info) == DataNode::RES_OK);
        assert(info.title == titles[step] && std::strcmp(info.name, "test") == 0 && init_polls == step * 2);
        DataProc::InitializationResult result;
        assert(dp::Init::poll(caller, step, 399, result) == DataNode::RES_OK);
        assert(result.status == DataProc::InitializationStatus::Pending);
        assert(dp::Init::poll(caller, step, 400, result) == DataNode::RES_OK);
        assert(result.status == (step == 3 ? DataProc::InitializationStatus::Failed : DataProc::InitializationStatus::Ok));
        char expected[96];
        std::snprintf(expected, sizeof(expected), "step %u at 400", step);
        assert(std::strcmp(result.detail.data(), expected) == 0);
    }
    const auto calls_before = init_polls;
    dp::InitStepInfo info;
    DataProc::InitializationResult result;
    assert(dp::Init::info(caller, 5, info) == DataNode::RES_PARAM_ERROR && std::strcmp(info.name, "invalid") == 0);
    assert(dp::Init::poll(caller, 5, 0, result) == DataNode::RES_PARAM_ERROR
        && result.status == DataProc::InitializationStatus::NotRun);
    dp::InitQuery query;
    query.operation = static_cast<dp::InitQueryOperation>(99);
    assert(caller.pull(dp::names::Init, &query, sizeof(query)) == DataNode::RES_PARAM_ERROR);
    assert(caller.pull(dp::names::Init, &query, sizeof(query) - 1) == DataNode::RES_SIZE_MISMATCH);
    assert(caller.pull(dp::names::Init, &query, sizeof(query) + 1) == DataNode::RES_SIZE_MISMATCH);
    assert(caller.pull(dp::names::Init, nullptr, sizeof(query)) == DataNode::RES_PARAM_ERROR);
    assert(init_polls == calls_before && pull<dp::Init>(caller) == original && publishes.empty());
    fixture::source.poll = nullptr;
}
void dates(DataNode& caller) {
    assert(dp::days_in_month(2000, 2) == 29 && dp::days_in_month(2024, 2) == 29);
    assert(dp::days_in_month(2100, 2) == 28 && dp::days_in_month(2026, 4) == 30);
    assert(dp::days_in_month(2026, 0) == 0 && dp::days_in_month(2026, 13) == 0);
    const auto original = pull<dp::Clock>(caller);
    for (const DataProc::DateTime date : {DataProc::DateTime{1999,1,1,0,0}, {2100,1,1,0,0},
        {2026,0,1,0,0}, {2026,13,1,0,0}, {2026,1,0,0,0}, {2026,4,31,0,0},
        {2025,2,29,0,0}, {2026,1,1,24,0}, {2026,1,1,0,60}}) {
        assert(!dp::valid_date(date));
        assert(dp::Clock::notify(caller, {date}) == DataNode::RES_PARAM_ERROR);
        assert(pull<dp::Clock>(caller) == original && publishes.empty());
    }
    assert(dp::valid_date({2000,2,29,23,59}) && dp::valid_date({2099,12,31,23,59}));
}
unsigned calls = 0;
bool power(hal::PowerCommand) { ++calls; return true; }
}
DeviceManager& hal::manager() { return fixture::devices(); }
int main() {
    fixture::source = {};
    auto& s = fixture::source;
    s.board.capabilities = {false,true,true,true,true}; s.wifi.available = true;
    s.work.valid = s.recorder.valid = true; s.work.channel = 1;
    s.execute_power = power;
    assert(DataProc_Init(clock_ms));
    assert(DataProc::Broker()->getNodeNumber() == 12);
    assert(!DataProc::Broker()->search("Status") && !DataProc::Broker()->search("System")
        && !DataProc::Broker()->search("Device"));
    {
        DataNode arbitrary("AnyCaller",DataProc::Broker());
        arbitrary.setEventCallback(receive);
        assert(dp::Power::subscribe(arbitrary) && dp::Battery::subscribe(arbitrary) && dp::GNSS::subscribe(arbitrary)
            && dp::Clock::subscribe(arbitrary) && dp::Work::subscribe(arbitrary) && dp::Recorder::subscribe(arbitrary)
            && dp::WiFi::subscribe(arbitrary) && dp::Version::subscribe(arbitrary) && dp::Init::subscribe(arbitrary)
            && dp::Settings::subscribe(arbitrary));
        init_queries(arbitrary);
        dates(arbitrary);
        assert(!DataProc_Deinit());
        advance(1000); advance(1000);
        assert(publishes.empty()); // 十个领域节点静态值轮询不发布。
        assert(pull<dp::Battery>(arbitrary) == s.battery && pull<dp::GNSS>(arbitrary) == s.gnss);
        s.battery.level_percent = 42;
        s.gnss.satellites_used = 7;
        s.clock.hour = 4;
        s.work.protocol = 2;
        s.recorder.space_mb = 3;
        s.wifi.on = true;
        s.board.uptime_s = 8;
        s.power = {true,false,false,hal::PowerCause::User};
        advance(1000);
        for (const char* name : {dp::names::Power,dp::names::Battery,dp::names::GNSS,dp::names::Clock,dp::names::Work,
            dp::names::Recorder,dp::names::WiFi,dp::names::Version}) assert(publishes[name] == 1);
        const auto counts = publishes;
        advance(1000); assert(publishes == counts);
        // 每类载荷都检查精确尺寸；读取不发布，命令不依赖页面名字。
        hal::BatteryState b;
        assert(arbitrary.pull(dp::names::Battery,&b,sizeof(b)-1) == DataNode::RES_SIZE_MISMATCH);
        dp::SettingsCommand language{i18n::Language::Russian};
        assert(arbitrary.notify(dp::names::Settings,&language,sizeof(language)-1) == DataNode::RES_SIZE_MISMATCH);
        assert(dp::Settings::notify(arbitrary,{static_cast<i18n::Language>(99)}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Settings::notify(arbitrary,language) == DataNode::RES_OK);
        assert(dp::Settings::notify(arbitrary,language) == DataNode::RES_OK && publishes[dp::names::Settings] == 1);
        assert(dp::Clock::notify(arbitrary,{{2025,2,29,12,30}}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Clock::notify(arbitrary,{{2000,2,29,23,59}}) == DataNode::RES_OK);
        const auto clock_count = publishes[dp::names::Clock];
        assert(dp::Clock::notify(arbitrary,{{2000,2,29,23,59}}) == DataNode::RES_OK && publishes[dp::names::Clock] == clock_count);
        for (hal::WorkCommand c : {hal::WorkCommand{static_cast<hal::WorkTarget>(99),0,1}, {hal::WorkTarget::Rover,8,1},
            {hal::WorkTarget::Rover,0,10}, {hal::WorkTarget::Rover,0,0}}) assert(dp::Work::notify(arbitrary,c) == DataNode::RES_PARAM_ERROR);
        s.work.pending = true; advance(500);
        assert(dp::Work::notify(arbitrary,{hal::WorkTarget::Base,0,1}) == DataNode::RES_PARAM_ERROR);
        s.work.pending = false; advance(500);
        assert(dp::Work::notify(arbitrary,{hal::WorkTarget::Base,1,2}) == DataNode::RES_OK);
        const auto work_count = publishes[dp::names::Work];
        assert(dp::Work::notify(arbitrary,{hal::WorkTarget::Base,1,2}) == DataNode::RES_OK && publishes[dp::names::Work] == work_count);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Stop}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start,2,0}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start,0,6}) == DataNode::RES_PARAM_ERROR);
        s.recorder.pending = true; advance(500);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start}) == DataNode::RES_PARAM_ERROR);
        s.recorder.pending = false; s.recorder.valid = false; advance(500);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start}) == DataNode::RES_PARAM_ERROR);
        s.recorder.valid = true; advance(500);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start,1,2}) == DataNode::RES_OK);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Start,1,2}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Recorder::notify(arbitrary,{hal::RecordAction::Stop}) == DataNode::RES_OK);
        s.wifi.available = false; advance(500);
        assert(dp::WiFi::notify(arbitrary,{false}) == DataNode::RES_PARAM_ERROR);
        s.wifi.available = true; s.wifi.pending = true; advance(500);
        assert(dp::WiFi::notify(arbitrary,{false}) == DataNode::RES_PARAM_ERROR);
        s.wifi.pending = false; advance(500);
        assert(dp::WiFi::notify(arbitrary,{true}) == DataNode::RES_OK); // 相同值无需设备写入。
        assert(dp::WiFi::notify(arbitrary,{false}) == DataNode::RES_UNKNOWN); // 设备拒绝保留原值。
        assert(dp::Init::notify(arbitrary,{dp::InitOperation::Record,0,{DataProc::InitializationStatus::Ok,{}}}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Init::notify(arbitrary,{dp::InitOperation::Begin}) == DataNode::RES_OK);
        const auto init_count = publishes[dp::names::Init];
        assert(dp::Init::notify(arbitrary,{dp::InitOperation::Begin}) == DataNode::RES_OK && publishes[dp::names::Init] == init_count);
        dp::InitCommand record{dp::InitOperation::Record,0,{DataProc::InitializationStatus::Ok,{}}};
        record.step = 5; assert(dp::Init::notify(arbitrary,record) == DataNode::RES_PARAM_ERROR);
        record.step = 0; record.result.status = DataProc::InitializationStatus::Pending;
        assert(dp::Init::notify(arbitrary,record) == DataNode::RES_PARAM_ERROR);
        record.result.status = DataProc::InitializationStatus::Ok; record.result.detail.fill('x');
        assert(dp::Init::notify(arbitrary,record) == DataNode::RES_PARAM_ERROR);
        record.result.detail.back() = '\0'; assert(dp::Init::notify(arbitrary,record) == DataNode::RES_OK);
        const auto recorded = publishes[dp::names::Init];
        assert(dp::Init::notify(arbitrary,record) == DataNode::RES_OK && publishes[dp::names::Init] == recorded);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Execute,{},0}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Prepare,static_cast<hal::PowerCommand>(99)}) == DataNode::RES_PARAM_ERROR);
        s.board.capabilities.reboot_available = false;
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Prepare,hal::PowerCommand::Reboot}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Prepare,hal::PowerCommand::Off}) == DataNode::RES_OK);
        auto p = pull<dp::Power>(arbitrary);
        assert(p.pending && p.token == 1);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Prepare}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Cancel,{},p.token+1}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Execute,{},p.token+1}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Cancel,{},p.token}) == DataNode::RES_OK);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Execute,{},p.token}) == DataNode::RES_PARAM_ERROR);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Prepare}) == DataNode::RES_OK);
        p = pull<dp::Power>(arbitrary);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Execute,{},p.token}) == DataNode::RES_OK);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::Execute,{},p.token}) == DataNode::RES_OK && calls == 1);
        assert(pull<dp::Power>(arbitrary).calls == 1);
        s.power = {true,true,false,hal::PowerCause::Host}; advance(20);
        assert(pull<dp::Power>(arbitrary).platform.cause == hal::PowerCause::Host);
        s.power = {false,false,true,hal::PowerCause::Host}; advance(20);
        assert(pull<dp::Power>(arbitrary).platform.standby);
        assert(dp::Power::notify(arbitrary,{dp::PowerOperation::RequestOn}) == DataNode::RES_OK);
        assert(pull<dp::Power>(arbitrary).platform.powered_on);
        // 任意调用者直接发送领域命令；日期和协议选项仍由各领域校验。
        const dp::ClockCommand request{{2026,1,1,1,1}};
        assert(dp::Clock::notify(arbitrary,request) == DataNode::RES_OK);
        assert(pull<dp::Clock>(arbitrary).date == request.date);
        const hal::WorkCommand work{hal::WorkTarget::Base,1,2};
        assert(dp::Work::notify(arbitrary,work) == DataNode::RES_OK);
        assert(pull<dp::Work>(arbitrary).mode == hal::WorkMode::Base
            && pull<dp::Work>(arbitrary).protocol == 1 && pull<dp::Work>(arbitrary).channel == 2);
    }
    assert(DataProc_Deinit() && DataProc::Broker()->getNodeNumber() == 0 && DataProc_RunLoop() == UINT32_MAX);
    fixture::source = {};
    assert(DataProc_Init(clock_ms));
    assert(hal_test::snapshot<dp::Power>().token == 0 && hal_test::snapshot<dp::Power>().calls == 0);
    assert(DataProc_Deinit());
    std::puts("领域节点：变化发布、语义命令与重建通过；12 节点、8 定时器。");
}
