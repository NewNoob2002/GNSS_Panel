#include "Node.h"
#include "Def/DP_GNSS.h"
namespace {
class GNSSNode final : public dp::PollingNode<hal::GNSSState> {
public:
    explicit GNSSNode(DataBroker* broker) : PollingNode(dp::names::GNSS, broker, hal::names::GNSS, 200) {}
};
}
DP_CREATE(GNSS, GNSSNode)
