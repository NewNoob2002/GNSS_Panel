#include "DemoStatus.h"
#include <cstdio>
#include <random>

namespace {
std::uint64_t sample_time = 0;
std::uint32_t sample_seed = 0;
bool random_sample = false;

/** 原确定性采样逻辑保持不变，由 Status 设备读取。 */

page::StatusBarState
generate(const std::uint64_t elapsed_ms) {
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

/** 原随机采样逻辑，相同种子和时间产生相同数据。 */
page::StatusBarState
generate_random(const std::uint64_t elapsed_ms, const std::uint32_t seed) {
    auto state = generate(elapsed_ms);
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

/** 将设备值快照转换回既有预览接口，不改变调用者和页面表现。 */
page::StatusBarState
page_state(const DataProc::StatusSnapshot& value) {
    page::StatusBarState state;
    state.position = "DEMO";
    state.position_color = lv_color_hex(value.position_color_rgb);
    state.satellites = value.satellites;
    state.satellites_valid = value.satellites_valid;
    state.battery_percent = value.battery_percent;
    state.battery_valid = value.battery_valid;
    state.charging = value.charging;
    state.battery_voltage = value.battery_voltage;
    state.battery_temperature_valid = value.battery_temperature_valid;
    state.battery_temperature = value.battery_temperature;
    state.hour = value.hour;
    state.minute = value.minute;
    state.second = value.second;
    state.clock_valid = value.clock_valid;
    state.recording = value.recording;
    state.wifi = value.wifi;
    return state;
}
} // namespace

/** 采样配置仅在应用线程使用，读接口返回调用方拥有的值快照。 */
DataProc::StatusSnapshot
DemoStatus::status_snapshot() {
    const auto state = random_sample ? generate_random(sample_time, sample_seed) : generate(sample_time);
    DataProc::StatusSnapshot value;
    std::snprintf(value.position.data(), value.position.size(), "%s", state.position);
    value.position_color_rgb = lv_color_to_u32(state.position_color) & 0xffffffu;
    value.satellites = state.satellites;
    value.satellites_valid = state.satellites_valid;
    value.battery_percent = state.battery_percent;
    value.battery_valid = state.battery_valid;
    value.charging = state.charging;
    value.battery_voltage = state.battery_voltage;
    value.battery_temperature_valid = state.battery_temperature_valid;
    value.battery_temperature = state.battery_temperature;
    value.hour = state.hour;
    value.minute = state.minute;
    value.second = state.second;
    value.clock_valid = state.clock_valid;
    value.recording = state.recording;
    value.wifi = state.wifi;
    return value;
}

/** 保留现有采样入口，状态数据经过 DeviceManager 的 Status read。 */
page::StatusBarState
DemoStatus::sample(std::uint64_t elapsed_ms) {
    sample_time = elapsed_ms;
    random_sample = false;
    return page_state(system_backend::read_status());
}

/** 随机预览同样通过 Status 设备返回，不产生额外服务发布。 */
page::StatusBarState
DemoStatus::sample_random(std::uint64_t elapsed_ms, std::uint32_t seed) {
    sample_time = elapsed_ms;
    sample_seed = seed;
    random_sample = true;
    return page_state(system_backend::read_status());
}
