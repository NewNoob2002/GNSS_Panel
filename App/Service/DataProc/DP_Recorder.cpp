#include "Node.h"
#include "Def/DP_Recorder.h"
namespace {
class RecorderNode final : public dp::PollingNode<hal::RecorderState> {
public:
    explicit RecorderNode(DataBroker* broker) : PollingNode(dp::names::Recorder, broker, hal::names::Recorder, 500) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return PollingNode::onEvent(event);
        hal::RecorderCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (!state_.valid || state_.pending) return RES_PARAM_ERROR;
        switch (command.action) {
        case hal::RecordAction::Start:
            if (state_.recording || command.type >= hal::kRecordTypeCount || command.interval >= hal::kRecordIntervalCount)
                return RES_PARAM_ERROR;
            break;
        case hal::RecordAction::Stop: if (!state_.recording) return RES_PARAM_ERROR; break;
        default: return RES_PARAM_ERROR;
        }
        return execute(command);
    }
private:
    bool valid(const hal::RecorderState& value) const override { return value.name.back() == '\0'; }
};
}
DP_CREATE(Recorder, RecorderNode)
