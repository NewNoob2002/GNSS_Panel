#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Power.h"

namespace dp {
/** @brief UI 确认消费前保留电源边沿；待机到来时清除旧的工作导航。 */
struct PowerNavigation {
    bool standby = false, shutdown = false, power_on = false;
    bool operator==(const PowerNavigation& v) const {
        return standby == v.standby && shutdown == v.shutdown && power_on == v.power_on;
    }
};
/** @brief 电源领域状态；token 在本次 DataProc 生命周期内单调递增。 */
struct PowerState {
    hal::PowerState platform{};
    PowerNavigation navigation{};
    bool pending = false;
    hal::PowerCommand action{};
    uint64_t token = 0, calls = 0;
    bool operator==(const PowerState& v) const {
        return platform == v.platform && navigation == v.navigation && pending == v.pending && action == v.action && token == v.token && calls == v.calls;
    }
};
/** @brief 准备、取消、幂等执行或请求开机；执行/取消使用准备时返回的 token。 */
enum class PowerOperation { Prepare, Cancel, Execute, RequestOn, AcknowledgeNavigation };
struct PowerCommand {
    PowerOperation operation{};
    hal::PowerCommand action{};
    uint64_t token = 0;
    PowerNavigation navigation{};
};
using Power = Writer<names::Power, PowerState, PowerCommand>;
}
