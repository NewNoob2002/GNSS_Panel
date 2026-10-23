#include "DataProc.h"
#include "Node.h"
#include <array>
#include <memory>
#include "Log.h"

namespace {
DataBroker center("CENTER");
constexpr size_t provider_count = 0
#define DP_DEF(NAME) +1
#include "DataProc_NodeList.inc"
#undef DP_DEF
;
std::array<std::unique_ptr<dp::Provider>, provider_count> providers;
bool initialized = false;
void destroy() {
    for (auto it = providers.rbegin(); it != providers.rend(); ++it) it->reset();
}
}
DataBroker* DataProc::Broker() { return &center; }

#define DP_DEF(NAME) dp::Provider* _DP_##NAME##_Create(DataBroker* broker);
#include "DataProc_NodeList.inc"
#undef DP_DEF

bool DataProc_Init(DataTimerManager::TickFunction_t tick_source) {
    if (initialized || center.getNodeNumber() != 0 || !center.initTimerManager(tick_source)) return false;
    size_t index = 0;
    bool ok = true;
#define DP_DEF(NAME) \
    if (ok) { \
        providers[index].reset(_DP_##NAME##_Create(&center)); \
        ok = providers[index] && providers[index]->isRegistered() && providers[index]->init(); \
    } \
    ++index;
#include "DataProc_NodeList.inc"
#undef DP_DEF
    if (!ok) { destroy(); APP_LOG_E("DataProc", "provider initialization failed and rolled back"); return false; }
    initialized = true;
    APP_LOG_I("DataProc", "providers initialized: %zu", providers.size());
    return true;
}
uint32_t DataProc_RunLoop() { return initialized ? center.handleTimer() : UINT32_MAX; }
bool DataProc_Deinit() {
    if (!initialized) return true;
    if (center.getNodeNumber() != providers.size()) {
        APP_LOG_E("DataProc", "destroy page/component DataNodes before providers");
        return false;
    }
    destroy();
    initialized = false;
    return true;
}
