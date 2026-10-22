#include "DataProc.h"
#include <array>
#include <memory>
#include <new>
#include "Log.h"

namespace {
// Explicit initialization follows X-TRACK; Center is not a page/service implementation.
DataBroker center("CENTER");
constexpr size_t provider_count = 0
#define DP_DEF(NAME, SIZE) +1
#include "DP_LIST.inc"
#undef DP_DEF
    ;
std::array<std::unique_ptr<DataNode>, provider_count> providers;
bool initialized = false;
} // namespace
DataBroker* DataProc::Broker() {
    return &center;
}

bool DataProc_Init(DataTimerManager::TickFunction_t clock) {
    if (initialized || center.getNodeNumber() != 0 || !center.initTimerManager(clock)) {
        APP_LOG_E("DataProc", "initialization rejected: existing DataNodes");
        return false;
    }
    size_t index = 0;
    bool ok = true;
#define DP_DEF(NAME, SIZE)                                                                                             \
    providers[index] = std::unique_ptr<DataNode>(new (std::nothrow) DataNode(#NAME, &center));                                                \
    ok = ok && providers[index] && providers[index]->isRegistered();                                                                       \
    ++index;
#include "DP_LIST.inc"
#undef DP_DEF
    index = 0;
#define DP_DEF(NAME, SIZE)                                                                                             \
    {                                                                                                                  \
        DATA_PROC_INIT_DEF(NAME);                                                                                      \
        if (ok)                                                                                                        \
            ok = _DP_##NAME##_Init(providers[index].get());                                                            \
        ++index;                                                                                                       \
    }
#include "DP_LIST.inc"
#undef DP_DEF
    if (!ok) {
        for (auto& provider : providers)
            provider.reset();
        APP_LOG_E("DataProc", "provider initialization failed and rolled back");
        return false;
    }
    initialized = true;
    APP_LOG_I("DataProc", "providers initialized");
    return true;
}
uint32_t DataProc_RunLoop() {
    return initialized ? center.handleTimer() : UINT32_MAX;
}
bool DataProc_Deinit() {
    if (!initialized)
        return true;
    if (center.getNodeNumber() != providers.size()) {
        APP_LOG_E("DataProc", "destroy page/component DataNodes before providers");
        return false;
    }
    for (auto& provider : providers)
        provider.reset();
    initialized = false;
    APP_LOG_I("DataProc", "providers deinitialized");
    return true;
}
