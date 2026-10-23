#include "Node.h"
#include "Def/DP_Clock.h"
namespace dp {
unsigned days_in_month(unsigned year, unsigned month) {
    constexpr unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return days[month - 1] + (month == 2 && leap ? 1 : 0);
}
bool valid_date(const DataProc::DateTime& date) {
    return date.year >= 2000 && date.year <= 2099 && date.month >= 1 && date.month <= 12 && date.day >= 1
        && date.day <= days_in_month(date.year, date.month) && date.hour < 24 && date.minute < 60;
}
}
namespace {
class ClockNode final : public dp::ValueNode<dp::ClockState> {
public:
    explicit ClockNode(DataBroker* broker) : ValueNode(dp::names::Clock, broker) {}
    bool init() override { refresh(); startTimer(200); return isRegistered() && hasTimer(); }
    int onEvent(EventParam_t* event) override {
        if (!event) return RES_PARAM_ERROR;
        if (event->event == EVENT_TIMER) { refresh(); return RES_OK; }
        if (event->event != EVENT_NOTIFY) return ValueNode::onEvent(event);
        dp::ClockCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (!dp::valid_date(command.date)) return RES_PARAM_ERROR;
        auto next = state_;
        next.date = command.date;
        commit(next);
        return RES_OK;
    }
private:
    void refresh() {
        auto next = state_;
        if (hal::read(system_backend::devices().getDevice(hal::names::Clock), next.clock) == DeviceObject::RES_OK) commit(next);
    }
};
}
DP_CREATE(Clock, ClockNode)
