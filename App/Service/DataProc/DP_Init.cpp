#include "Node.h"
#include "Def/DP_Init.h"
#include "HAL/Def/HAL_Init.h"
#include <algorithm>
namespace dp {
int Init::info(DataNode& caller, unsigned step, InitStepInfo& value) {
    InitQuery query;
    query.step = step;
    const int result = caller.pull(names::Init, &query, sizeof(query));
    if (result == DataNode::RES_OK) value = query.info;
    return result;
}
int Init::poll(DataNode& caller, unsigned step, uint32_t elapsed, DataProc::InitializationResult& value) {
    InitQuery query;
    query.operation = InitQueryOperation::Poll;
    query.step = step;
    query.elapsed_ms = elapsed;
    const int result = caller.pull(names::Init, &query, sizeof(query));
    if (result == DataNode::RES_OK) value = query.result;
    return result;
}
}
namespace {
i18n::TextId title(hal::InitStage stage) {
    switch (stage) {
    case hal::InitStage::Configuration: return i18n::TextId::LoadingConfiguration;
    case hal::InitStage::BatteryGauge: return i18n::TextId::CheckingBatteryGauge;
    case hal::InitStage::Charger: return i18n::TextId::CheckingCharger;
    case hal::InitStage::HostPower: return i18n::TextId::PoweringHostBoard;
    case hal::InitStage::HostLink: return i18n::TextId::CheckingHostLink;
    case hal::InitStage::Services: return i18n::TextId::PreparingServices;
    case hal::InitStage::GNSS: return i18n::TextId::StartingGnssService;
    case hal::InitStage::Network: return i18n::TextId::PreparingNetwork;
    case hal::InitStage::Finalization: return i18n::TextId::FinalizingStartup;
    }
    return i18n::TextId::Initializing;
}
class InitNode final : public dp::ValueNode<dp::InitState> {
public:
    explicit InitNode(DataBroker* broker) : ValueNode(dp::names::Init, broker) {}
    int onEvent(EventParam_t* event) override {
        if (event && event->event == EVENT_PULL && event->size == sizeof(dp::InitQuery)) return query(event);
        if (!event || event->event != EVENT_NOTIFY) return ValueNode::onEvent(event);
        dp::InitCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        auto next = state_;
        switch (command.operation) {
        case dp::InitOperation::Begin: next = {}; next.started = true; break;
        case dp::InitOperation::Record:
            if (!state_.started || command.step >= next.results.size()
                || (command.result.status != DataProc::InitializationStatus::Ok && command.result.status != DataProc::InitializationStatus::Failed)
                || std::find(command.result.detail.begin(), command.result.detail.end(), '\0') == command.result.detail.end())
                return RES_PARAM_ERROR;
            next.results[command.step] = command.result;
            break;
        default: return RES_PARAM_ERROR;
        }
        commit(next);
        return RES_OK;
    }
private:
    /** @brief 查询只回写同步载荷，不更改历史结果或触发发布。 */
    int query(EventParam_t* event) {
        dp::InitQuery value;
        const int decoded = DataProc::ReadPayload(event, value);
        if (decoded != RES_OK) return decoded;
        if (value.step >= DataProc::kInitializationStepCount) return RES_PARAM_ERROR;
        auto* device = hal::manager().getDevice(hal::names::Init);
        int result;
        switch (value.operation) {
        case dp::InitQueryOperation::Info: {
            hal::InitInfo info;
            info.step = value.step;
            result = hal::ioctl(device, hal::kInitializationInfo, info);
            if (result == DeviceObject::RES_OK) value.info = {info.name, title(info.stage)};
            break;
        }
        case dp::InitQueryOperation::Poll: {
            hal::InitPoll poll;
            poll.step = value.step;
            poll.elapsed_ms = value.elapsed_ms;
            result = hal::ioctl(device, hal::kPollInitialization, poll);
            if (result == DeviceObject::RES_OK)
                value.result = {static_cast<DataProc::InitializationStatus>(poll.status), poll.detail};
            break;
        }
        default: return RES_PARAM_ERROR;
        }
        if (result != DeviceObject::RES_OK) return RES_UNKNOWN;
        std::memcpy(event->data_p, &value, sizeof(value));
        return RES_OK;
    }
};
}
DP_CREATE(Init, InitNode)
