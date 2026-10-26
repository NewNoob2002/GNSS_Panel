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
    uint8_t brightness = 100; ///< 屏幕亮度百分比，20..100。
    bool operator==(const BoardState& v) const {
        return firmware == v.firmware && hardware == v.hardware && build == v.build && uptime_s == v.uptime_s
            && capabilities == v.capabilities && brightness == v.brightness;
    }
    bool operator!=(const BoardState& v) const { return !(*this == v); }
};
struct BrightnessCommand { uint8_t percent = 100; };

constexpr bool
is_brightness_level(uint8_t percent) {
    return percent >= 20 && percent <= 100 && percent % 20 == 0;
}

// 实机最低档 0x0D 不可读，下限取原线性 20% 的 0x33（Y=51/255=0.2）。
constexpr uint8_t kBrightnessFloor = 0x33;
// CIE 1976 L*min=116*cbrt(0.2)-16≈51.8372，到 100 均分四段：51.84/63.88/75.92/87.96/100。
// Y=((L*+16)/116)^3，round(Y*255)=51/83/127/184/255；DCS 0x51 与 SDL 共用此表。
constexpr uint8_t
brightness_panel_value(uint8_t percent) {
    constexpr uint8_t values[] = {kBrightnessFloor, 0x53, 0x7F, 0xB8, 0xFF};
    // 命令端拒绝非五档值；显示端兜底取最近档，边界钳位，同距取较亮档。
    const unsigned index = percent <= 20 ? 0 : percent >= 100 ? 4 : (percent + 10U) / 20U - 1U;
    return values[index];
}
} // namespace hal
