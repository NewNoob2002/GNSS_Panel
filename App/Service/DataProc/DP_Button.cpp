#include "Node.h"
#include "Def/DP_Button.h"

namespace {
class ButtonNode final : public dp::ValueNode<dp::ButtonState> {
  public:
    explicit ButtonNode(DataBroker* broker) : ValueNode(dp::names::Button, broker) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY)
            return ValueNode::onEvent(event);
        dp::ButtonCommand command;
        const int result = DataProc::ReadPayload(event, command);
        if (result != RES_OK)
            return result;
        switch (command.operation) {
        case dp::ButtonOperation::Level:
            if (command.key == dp::ButtonKey::Power)
                power_down_ = command.pressed;
            else if (command.key == dp::ButtonKey::Function)
                function_down_ = command.pressed;
            else
                return RES_PARAM_ERROR;
            return RES_OK;
        case dp::ButtonOperation::Cancel:
            if (command.key != dp::ButtonKey::Power && command.key != dp::ButtonKey::Function
                && command.key != dp::ButtonKey::Both)
                return RES_PARAM_ERROR;
            if (command.key != dp::ButtonKey::Function) {
                if (command.clear_levels)
                    power_down_ = false;
                power_.cancel(power_down_);
            }
            if (command.key != dp::ButtonKey::Power) {
                if (command.clear_levels)
                    function_down_ = false;
                function_.cancel(function_down_);
            }
            state_ = {{}, {}, power_.held(), function_.held()};
            return RES_OK;
        case dp::ButtonOperation::Poll: {
            const uint64_t now = command.time_ms < last_poll_ms_ ? last_poll_ms_ : command.time_ms;
            last_poll_ms_ = now;
            state_ = {power_.sample(power_down_, now), function_.sample(function_down_, now), power_.held(), function_.held()};
            /** @brief hold 需要每次采样的电平，不能只发布动作或变化。 */
            const auto sample = state_;
            return publish(&sample, sizeof(sample));
        }
        default:
            return RES_PARAM_ERROR;
        }
    }

  private:
    ButtonGesture power_, function_;
    bool power_down_ = false, function_down_ = false;
    uint64_t last_poll_ms_ = 0;
};
}
DP_CREATE(Button, ButtonNode)
