#pragma once
#include "HAL_Def.h"

namespace hal {
inline constexpr unsigned kRadioProtocolCount = 8, kRadioChannelCount = 10;
/** @brief 当前工作模式与 NTRIP 角色。 */
enum class WorkMode { Rover, Base };
enum class NtripRole { None, Server, Client };
/** @brief 应用所请求的工作目标；协议码转换仅属于平台实现。 */
enum class WorkTarget { Rover, Base, Ntrip };
/** @brief 当前有效工作配置，pending 表示请求尚未被主控完整读走。 */
struct WorkState {
    bool valid = false, pending = false;
    WorkMode mode{};
    bool radio_on = false;
    unsigned protocol = 0, channel = 0; ///< 选项下标 0..7/0..9，信道 0 为 CUSTOM。
    NtripRole ntrip{};
    bool operator==(const WorkState& v) const {
        return valid == v.valid && pending == v.pending && mode == v.mode && radio_on == v.radio_on
            && protocol == v.protocol && channel == v.channel && ntrip == v.ntrip;
    }
    bool operator!=(const WorkState& v) const { return !(*this == v); }
};
/** @brief 同步提交工作请求；电台信道为 1..9，Ntrip 不使用电台选项但仍要求下标合法。 */
struct WorkCommand {
    WorkTarget target{};
    unsigned protocol = 0, channel = 0;
};
} // namespace hal
