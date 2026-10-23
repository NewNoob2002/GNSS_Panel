#include "Aggregate.h"
#include "SystemService.h"
#include "Def/DP_Clock.h"
#include "Def/DP_Init.h"
#include "Def/DP_Power.h"
#include "Def/DP_Settings.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"
namespace {
class SystemNode final : public dp::Aggregate<DataProc::SystemState, dp::Clock, dp::Init, dp::Power,
    dp::Settings, dp::Version, dp::WiFi> {
public:
    explicit SystemNode(DataBroker* broker) : Aggregate(dp::names::System, broker) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return Aggregate::onEvent(event);
        DataProc::SystemRequest request;
        const int decoded = DataProc::ReadPayload(event, request);
        if (decoded != RES_OK) return decoded;
        switch (request.command) {
        case DataProc::SystemCommand::SetLanguage: return dp::Settings::notify(*this, {request.language});
        case DataProc::SystemCommand::SetTime: return dp::Clock::notify(*this, {request.date});
        case DataProc::SystemCommand::PreparePower: return dp::Power::notify(*this, {dp::PowerOperation::Prepare, request.action});
        case DataProc::SystemCommand::CancelPower: return dp::Power::notify(*this, {dp::PowerOperation::Cancel, {}, request.token});
        case DataProc::SystemCommand::ExecutePower: return dp::Power::notify(*this, {dp::PowerOperation::Execute, {}, request.token});
        case DataProc::SystemCommand::RequestPowerOn: return dp::Power::notify(*this, {dp::PowerOperation::RequestOn});
        case DataProc::SystemCommand::SetWifi: return dp::WiFi::notify(*this, {request.wifi_on});
        case DataProc::SystemCommand::BeginInitialization: return dp::Init::notify(*this, {dp::InitOperation::Begin});
        case DataProc::SystemCommand::RecordInitialization:
            return dp::Init::notify(*this, {dp::InitOperation::Record, request.initialization_step, request.initialization});
        default: return RES_UNSUPPORTED_REQUEST;
        }
    }
private:
    DataProc::SystemState compose() const override {
        DataProc::SystemState value;
        const auto& power = get<dp::Power>(); const auto& caps = get<dp::Version>().capabilities;
        const auto& wifi = get<dp::WiFi>(); const auto& init = get<dp::Init>();
        value.language = get<dp::Settings>().language; value.date = get<dp::Clock>().date;
        value.simulated = caps.simulated; value.reboot_available = caps.reboot_available;
        value.star_map = caps.star_map; value.record_settings = caps.record_settings;
        value.wifi_available = wifi.available; value.wifi_on = wifi.on; value.wifi_pending = wifi.pending;
        value.power_pending = power.pending; value.power_action = power.action; value.power_token = power.token; value.power_calls = power.calls;
        value.powered_on = power.platform.powered_on; value.shutting_down = power.platform.shutting_down;
        value.standby = power.platform.standby; value.power_cause = power.platform.cause;
        value.initialization_started = init.started; value.initialization = init.results;
        return value;
    }
};
}
DP_CREATE(System, SystemNode)
DataProc::SystemState SystemService::snapshot() {
    DataProc::SystemState result;
    auto* node = DataProc::Broker()->search(dp::names::System);
    if (node) {
        DataNode::EventParam_t event(nullptr, node, DataNode::EVENT_PULL, &result, sizeof(result));
        (void)node->onEvent(&event);
    }
    return result;
}
unsigned SystemService::days_in_month(unsigned year, unsigned month) { return dp::days_in_month(year, month); }
bool SystemService::valid_date(const DataProc::DateTime& date) { return dp::valid_date(date); }
