#include "Node.h"
#include "Def/DP_Work.h"
namespace {
class WorkNode final : public dp::PollingNode<hal::WorkState> {
public:
    explicit WorkNode(DataBroker* broker) : PollingNode(dp::names::Work, broker, hal::names::Work, 500) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return PollingNode::onEvent(event);
        hal::WorkCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (state_.pending || (command.target != hal::WorkTarget::Rover && command.target != hal::WorkTarget::Base
            && command.target != hal::WorkTarget::Ntrip) || command.protocol >= hal::kRadioProtocolCount
            || command.channel >= hal::kRadioChannelCount || (command.target != hal::WorkTarget::Ntrip && !command.channel))
            return RES_PARAM_ERROR;
        return execute(command);
    }
};
}
DP_CREATE(Work, WorkNode)
