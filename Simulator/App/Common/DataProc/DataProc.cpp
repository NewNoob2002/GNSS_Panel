#include "DataProc.h"
#include <array>
#include <memory>
#include "Utils/Log/Log.h"

namespace {
// Explicit initialization follows X-TRACK; Center is not a page/service implementation.
DataCenter center("CENTER");
constexpr size_t provider_count = 0
#define DP_DEF(NAME, SIZE) +1
#include "DP_LIST.inc"
#undef DP_DEF
    ;
std::array<std::unique_ptr<Account>, provider_count> providers;
bool initialized = false;
} // namespace
DataCenter* DataProc::Center() {
    return &center;
}

bool DataProc_Init() {
    if (initialized || center.GetAccountLen() != 0) {
        APP_LOG_E("DataProc", "initialization rejected: existing Accounts");
        return false;
    }
    size_t index = 0;
    bool ok = true;
#define DP_DEF(NAME, SIZE)                                                                                             \
    providers[index] = std::make_unique<Account>(#NAME, &center, SIZE);                                                \
    ok = ok && providers[index]->IsRegistered();                                                                       \
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
bool DataProc_Deinit() {
    if (!initialized)
        return true;
    if (center.GetAccountLen() != providers.size()) {
        APP_LOG_E("DataProc", "destroy page/component Accounts before providers");
        return false;
    }
    for (auto& provider : providers)
        provider.reset();
    initialized = false;
    APP_LOG_I("DataProc", "providers deinitialized");
    return true;
}
