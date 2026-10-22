#pragma once
#include "HAL_Def.h"

namespace hal {
/** @brief 时钟来源；当前平台均为 Uptime，Host 预留给主控提供的时间。 */
enum class ClockSource { Uptime, Host };
/** @brief 平台时间同步快照；日期归应用所有，不在 HAL 中模拟。 */
struct ClockState {
    bool time_valid = false;
    unsigned hour = 0, minute = 0, second = 0; ///< 时/分/秒，范围 0..23/0..59/0..59。
    ClockSource source{};
    uint64_t uptime_ms = 0; ///< 运行时长，ms；HC32 tick 每 2^32 ms 回绕。
    bool operator==(const ClockState& v) const {
        return time_valid == v.time_valid && hour == v.hour && minute == v.minute && second == v.second
            && source == v.source && uptime_ms == v.uptime_ms;
    }
    bool operator!=(const ClockState& v) const { return !(*this == v); }
};
} // namespace hal
