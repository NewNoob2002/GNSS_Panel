#include "DomainFixture.hpp"
#include "SystemService.h"
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
unsigned calls = 0;
bool power(DataProc::PowerAction) { ++calls; return true; }
}
DeviceManager& system_backend::devices() { return fixture::devices(); }
int main() {
    fixture::source = {};
    auto& s = fixture::source;
    s.board.capabilities = {false,true,true,true,true}; s.wifi.available = true;
    s.work.valid = s.recorder.valid = true; s.work.channel = 1;
    s.execute_power = power;
    assert(DataProc_Init(clock_ms));
    assert(DataProc::Broker()->getNodeNumber() == 13);
    {
        DataNode arbitrary("AnyCaller",DataProc::Broker());
        arbitrary.setEventCallback(receive);
        assert(dp::Power::subscribe(arbitrary) && dp::Battery::subscribe(arbitrary) && dp::GNSS::subscribe(arbitrary)
            && dp::Clock::subscribe(arbitrary) && dp::Work::subscribe(arbitrary) && dp::Recorder::subscribe(arbitrary)
            && dp::WiFi::subscribe(arbitrary) && dp::Version::subscribe(arbitrary) && dp::Init::subscribe(arbitrary)
            && dp::Settings::subscribe(arbitrary));
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
        // 旧 Device/System 聚合转发同样无名字授权；实际约束全部在领域节点。
        assert(arbitrary.subscribe(dp::names::Device) && arbitrary.subscribe(dp::names::System));
        DataProc::SystemRequest request;
        request.command = DataProc::SystemCommand::SetTime; request.date = {2026,1,1,1,1};
        assert(arbitrary.notify(dp::names::System,&request,sizeof(request)) == DataNode::RES_OK);
        assert(SystemService::snapshot().date == request.date);
        DataProc::DeviceRequest work;
        work.target = DataProc::WorkTarget::Base; work.radio_protocol = 1; work.radio_channel = 2;
        assert(arbitrary.notify(dp::names::Device,&work,sizeof(work)) == DataNode::RES_OK);
    }
    assert(DataProc_Deinit() && DataProc::Broker()->getNodeNumber() == 0 && DataProc_RunLoop() == UINT32_MAX);
    fixture::source = {};
    assert(DataProc_Init(clock_ms));
    assert(SystemService::snapshot().power_token == 0 && SystemService::snapshot().power_calls == 0);
    assert(DataProc_Deinit());
    std::puts("领域节点：变化发布、语义命令、聚合转发与重建通过；13 节点、9 定时器。");
}
