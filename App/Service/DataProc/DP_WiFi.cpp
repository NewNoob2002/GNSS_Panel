#include "Node.h"
#include "Def/DP_WiFi.h"
namespace {
class WifiNode final : public dp::PollingNode<hal::WifiState> {
public:
    explicit WifiNode(DataBroker* broker) : PollingNode(dp::names::WiFi, broker, hal::names::WiFi, 500) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return PollingNode::onEvent(event);
        hal::WifiCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (!state_.available || state_.pending) return RES_PARAM_ERROR;
        if (command.on == state_.on) return RES_OK;
        return execute(command);
    }
private:
    bool valid(const hal::WifiState& value) const override { return value.ssid.back() == '\0'; }
};
}
DP_CREATE(WiFi, WifiNode)
