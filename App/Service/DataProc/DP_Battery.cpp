#include "Node.h"
#include "Def/DP_Battery.h"
namespace {
class BatteryNode final : public dp::PollingNode<hal::BatteryState> {
public:
    explicit BatteryNode(DataBroker* broker) : PollingNode(dp::names::Battery, broker, hal::names::Battery, 200) {}
};
}
DP_CREATE(Battery, BatteryNode)
