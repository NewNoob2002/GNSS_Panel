#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Power.h"

namespace dp {
/** @brief 电源领域状态；token 在本次 DataProc 生命周期内单调递增。 */
struct PowerState {
    hal::PowerState platform{};
    bool pending = false;
    hal::PowerCommand action{};
    uint64_t token = 0, calls = 0;
    bool operator==(const PowerState& v) const {
        return platform == v.platform && pending == v.pending && action == v.action && token == v.token && calls == v.calls;
    }
};
/** @brief 准备、取消、幂等执行或请求开机；执行/取消使用准备时返回的 token。 */
enum class PowerOperation { Prepare, Cancel, Execute, RequestOn };
struct PowerCommand { PowerOperation operation{}; hal::PowerCommand action{}; uint64_t token = 0; };
using Power = Writer<names::Power, PowerState, PowerCommand>;
}
