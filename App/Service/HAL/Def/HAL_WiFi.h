#pragma once
#include <array>
#include <cstdint>

namespace hal {
/** @brief Wi-Fi 同步状态；available 表示平台允许切换，valid 表示链路信息可读。 */
struct WifiState {
    bool valid = false, available = false, on = false, pending = false, station = false;
    std::array<char, 17> ssid{}; ///< 调用方拥有的 NUL 结尾 SSID，最多 16 字节。
    std::array<std::uint8_t, 4> ip{}; ///< IPv4 四个八位段，网络显示顺序。
    bool operator==(const WifiState& v) const {
        return valid == v.valid && available == v.available && on == v.on && pending == v.pending
            && station == v.station && ssid == v.ssid && ip == v.ip;
    }
    bool operator!=(const WifiState& v) const { return !(*this == v); }
};
/** @brief 应用线程同步提交 Wi-Fi 开关；未提供切换能力返回 RES_UNSUPPORT。 */
struct WifiCommand { bool on = false; };
} // namespace hal
