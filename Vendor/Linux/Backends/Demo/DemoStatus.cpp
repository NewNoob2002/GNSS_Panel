#include "DemoStatus.h"
#include <random>

page::StatusBarState DemoStatus::sample(const std::uint64_t elapsed_ms) {
    const std::uint64_t seconds = elapsed_ms / 1000;
    page::StatusBarState state;
    state.position = "DEMO";
    state.position_color = lv_color_hex(0xf44336);
    state.satellites_valid = true;
    state.satellites = 8U + static_cast<unsigned>(seconds % 5U);
    state.clock_valid = true;
    state.hour = static_cast<unsigned>((12U + (seconds / 3600U)) % 24U);
    state.minute = static_cast<unsigned>((seconds / 60U) % 60U);
    state.second = static_cast<unsigned>(seconds % 60U);
    state.battery_valid = true;
    state.battery_percent = 85U - static_cast<unsigned>((seconds / 30U) % 16U);
    state.charging = ((seconds / 15U) % 2U) != 0U;
    state.battery_voltage = 7.40f + (static_cast<float>(state.battery_percent) / 100.0f) * 0.80f;
    state.wifi = (seconds / 10U) % 2U != 0U;
    state.recording = (seconds / 16U) % 2U != 0U;
    return state;
}

page::StatusBarState DemoStatus::sample_random(const std::uint64_t elapsed_ms, const std::uint32_t seed) {
    auto state = sample(elapsed_ms);
    const auto event = elapsed_ms / 3000;
    std::seed_seq seeds{seed, static_cast<std::uint32_t>(event), static_cast<std::uint32_t>(event >> 32)};
    std::mt19937 random(seeds);
    state.battery_percent = std::uniform_int_distribution<unsigned>(0, 100)(random);
    state.charging = std::bernoulli_distribution(0.5)(random);
    // 仅用于视觉验证的双节电池范围，不代表真实放电曲线。
    state.battery_voltage = static_cast<float>(6800 + 16 * state.battery_percent) / 1000.0f;
    state.satellites = std::uniform_int_distribution<unsigned>(0, 20)(random);
    state.wifi = std::bernoulli_distribution(0.5)(random);
    state.recording = std::bernoulli_distribution(0.5)(random);
    return state;
}
