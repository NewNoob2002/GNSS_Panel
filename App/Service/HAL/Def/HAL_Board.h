#pragma once
#include <array>
#include <cstdint>

namespace hal {
/** @brief 产品能力；均为应用线程读取的固定平台属性。 */
struct Capabilities {
    bool simulated = false, reboot_available = false, wifi_available = false, star_map = false, record_settings = false;
    bool operator==(const Capabilities& v) const {
        return simulated == v.simulated && reboot_available == v.reboot_available && wifi_available == v.wifi_available
            && star_map == v.star_map && record_settings == v.record_settings;
    }
    bool operator!=(const Capabilities& v) const { return !(*this == v); }
};
/** @brief 板卡版本与运行时长，字符串按值归调用方所有且以 NUL 结尾。 */
struct BoardState {
    std::array<char, 16> firmware{}, hardware{};
    std::array<char, 24> build{};
    std::uint32_t uptime_s = 0; ///< s，沿用平台 tick 的回绕规则。
    Capabilities capabilities{};
    bool operator==(const BoardState& v) const {
        return firmware == v.firmware && hardware == v.hardware && build == v.build && uptime_s == v.uptime_s
            && capabilities == v.capabilities;
    }
    bool operator!=(const BoardState& v) const { return !(*this == v); }
};
} // namespace hal
