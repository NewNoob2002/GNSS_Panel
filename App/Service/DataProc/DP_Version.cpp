#include "Node.h"
#include "Def/DP_Version.h"
namespace {
class VersionNode final : public dp::PollingNode<dp::Version::State> {
public:
    explicit VersionNode(DataBroker* broker) : PollingNode(dp::names::Version, broker, hal::names::Board, 1000) {}
private:
    bool valid(const hal::BoardState& value) const override {
        return !value.firmware.back() && !value.hardware.back() && !value.build.back();
    }
};
}
DP_CREATE(Version, VersionNode)
