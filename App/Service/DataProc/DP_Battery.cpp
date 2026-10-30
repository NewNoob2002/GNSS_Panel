#include "Node.h"
#include "Def/DP_Battery.h"
#include "Log.h"
namespace {
class BatteryNode final : public dp::PollingNode<hal::BatteryState> {
public:
    explicit BatteryNode(DataBroker* broker) : PollingNode(dp::names::Battery, broker, hal::names::Battery, 200) {}
#if APP_LOG_COMPILE_LEVEL <= 1
private:
    void refresh() override {
        hal::BatteryState next;
        if (hal::read(hal::manager().getDevice(device_), next) != DeviceObject::RES_OK)
            return;
        if (next.valid != state_.valid || next.charge != state_.charge || next.external_power != state_.external_power) {
            APP_LOG_I("Battery", "valid=%d external_power=%d charge=%u", next.valid, next.external_power,
                      static_cast<unsigned>(next.charge));
        }
        commit(next);
    }
#endif
};
}
DP_CREATE(Battery, BatteryNode)
