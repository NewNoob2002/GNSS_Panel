#pragma once
#include <array>
#include "HAL_Def.h"

namespace hal {
inline constexpr unsigned kRecordTypeCount = 2, kRecordIntervalCount = 6;
/** @brief 当前有效录制状态；请求确认前可与主控实际状态不同。 */
struct RecorderState {
    bool valid = false, recording = false, pending = false;
    unsigned type = 0, interval = 0; ///< 类型/间隔选项下标 0..1/0..5。
    std::array<char, 17> name{}; ///< 调用方拥有的 NUL 结尾文件名，最多 16 字节。
    float space_mb = 0; ///< 剩余空间，MB，沿用主控上报单位。
    bool reported_recording = false; ///< 主控实际录制状态，状态栏在确认前使用它。
    bool operator==(const RecorderState& v) const {
        return valid == v.valid && recording == v.recording && pending == v.pending && type == v.type
            && interval == v.interval && name == v.name && space_mb == v.space_mb && reported_recording == v.reported_recording;
    }
    bool operator!=(const RecorderState& v) const { return !(*this == v); }
};
/** @brief 录制动作；开始时校验选项范围，停止时选项被忽略。 */
enum class RecordAction { Start, Stop };
struct RecorderCommand {
    RecordAction action{};
    unsigned type = 0, interval = 0;
};
} // namespace hal
