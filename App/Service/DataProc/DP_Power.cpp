#include "Node.h"
#include "Def/DP_Power.h"
#include "HAL/Def/HAL_Board.h"
#include <limits>
namespace {
class PowerNode final : public dp::ValueNode<dp::PowerState> {
public:
    explicit PowerNode(DataBroker* broker) : ValueNode(dp::names::Power, broker) {}
    bool init() override { startTimer(20); return isRegistered() && hasTimer(); }
    int onEvent(EventParam_t* event) override {
        if (!event) return RES_PARAM_ERROR;
        if (event->event == EVENT_TIMER) { refresh(); return RES_OK; }
        if (event->event != EVENT_NOTIFY) return ValueNode::onEvent(event);
        dp::PowerCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        auto next = state_;
        auto* device = system_backend::devices().getDevice(hal::names::Power);
        switch (command.operation) {
        case dp::PowerOperation::Prepare: {
            hal::BoardState board;
            if (next.pending || (command.action != hal::PowerCommand::Off && command.action != hal::PowerCommand::Reboot)
                || next.token == std::numeric_limits<uint64_t>::max()) return RES_PARAM_ERROR;
            if (command.action == hal::PowerCommand::Reboot
                && (hal::read(system_backend::devices().getDevice(hal::names::Board), board) != DeviceObject::RES_OK
                    || !board.capabilities.reboot_available)) return RES_PARAM_ERROR;
            ++next.token;
            next.action = command.action;
            next.pending = true;
            commit(next);
            return RES_OK;
        }
        case dp::PowerOperation::Cancel:
            if (command.token != next.token) return RES_PARAM_ERROR;
            next.pending = false;
            commit(next);
            return RES_OK;
        case dp::PowerOperation::RequestOn:
            if (next.platform.powered_on) return RES_OK;
            if (hal::ioctl(device, hal::kRequestPowerOn) != DeviceObject::RES_OK) return RES_OK;
            next.platform.powered_on = true;
            next.platform.standby = next.platform.shutting_down = false;
            commit(next);
            return RES_OK;
        case dp::PowerOperation::Execute:
            if (!command.token) return RES_PARAM_ERROR;
            if (command.token == executed_token_) return RES_OK;
            if (!next.pending || command.token != next.token || next.calls == std::numeric_limits<uint64_t>::max())
                return RES_PARAM_ERROR;
            next.pending = false;
            ++next.calls;
            executed_token_ = command.token;
            commit(next); // 副作用前消费 token，防止同步回调重入导致重复执行。
            if (hal::write(device, next.action) != DeviceObject::RES_OK) return RES_UNKNOWN;
            refresh();
            return RES_OK;
        default: return RES_PARAM_ERROR;
        }
    }
private:
    uint64_t executed_token_ = 0;
    void refresh() {
        hal::PowerState report;
        if (hal::read(system_backend::devices().getDevice(hal::names::Power), report) != DeviceObject::RES_OK) return;
        auto next = state_;
        if (report.powered_on && !last_.powered_on) {
            next.platform.powered_on = true;
            next.platform.standby = next.platform.shutting_down = false;
        }
        if (report.powered_on && report.shutting_down && !next.platform.standby) {
            next.platform.shutting_down = true;
            next.platform.cause = report.cause;
            next.pending = false;
        }
        if (report.standby) {
            next.platform.standby = true;
            next.platform.powered_on = next.platform.shutting_down = next.pending = false;
        }
        last_ = report;
        commit(next);
    }
    hal::PowerState last_{};
};
}
DP_CREATE(Power, PowerNode)
