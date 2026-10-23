#include "Node.h"
#include "Def/DP_Init.h"
#include <algorithm>
namespace dp {
int initialization_info(hal::InitInfo& value) {
    return hal::ioctl(system_backend::devices().getDevice(hal::names::Init), hal::kInitializationInfo, value);
}
int initialization_poll(hal::InitPoll& value) {
    return hal::ioctl(system_backend::devices().getDevice(hal::names::Init), hal::kPollInitialization, value);
}
}
namespace {
class InitNode final : public dp::ValueNode<dp::InitState> {
public:
    explicit InitNode(DataBroker* broker) : ValueNode(dp::names::Init, broker) {}
    int onEvent(EventParam_t* event) override {
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
};
}
DP_CREATE(Init, InitNode)
