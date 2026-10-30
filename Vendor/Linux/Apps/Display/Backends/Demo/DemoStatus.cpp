#include "DemoStatus.h"
#include <random>

namespace {
DemoStatus::Clock source_clock = nullptr;
uint32_t origin = 0, sample_seed = 0;
uint64_t sample_time = 0;
bool random_sample = false, have_sample = false;
bool previous_recording = false;

page::StatusBarState
generate(uint64_t elapsed_ms) {
    const auto seconds = elapsed_ms / 1000;
    page::StatusBarState s;
    s.position = "DEMO";
    s.position_color = lv_color_hex(0xf44336);
    s.satellites_valid = true;
    s.satellites = 8U + static_cast<unsigned>(seconds % 5U);
    s.clock_valid = true;
    s.hour = static_cast<unsigned>((12U + seconds / 3600U) % 24U);
    s.minute = static_cast<unsigned>((seconds / 60U) % 60U);
    s.second = static_cast<unsigned>(seconds % 60U);
    s.battery_valid = true;
    s.battery_percent = 85U - static_cast<unsigned>((seconds / 30U) % 16U);
    s.charging = (seconds / 15U) % 2U != 0U;
    s.battery_voltage = 7.40f + static_cast<float>(s.battery_percent) / 100.0f * 0.80f;
    s.wifi = (seconds / 10U) % 2U != 0U;
    s.recording = (seconds / 16U) % 2U != 0U;
    if (random_sample) {
        const auto event = elapsed_ms / 3000;
        std::seed_seq seeds{sample_seed, static_cast<uint32_t>(event), static_cast<uint32_t>(event >> 32)};
        std::mt19937 random(seeds);
        s.battery_percent = std::uniform_int_distribution<unsigned>(0, 100)(random);
        s.charging = std::bernoulli_distribution(0.5)(random);
        s.battery_voltage = static_cast<float>(6800 + 16 * s.battery_percent) / 1000.0f;
        s.satellites = std::uniform_int_distribution<unsigned>(0, 20)(random);
        s.wifi = std::bernoulli_distribution(0.5)(random);
        s.recording = std::bernoulli_distribution(0.5)(random);
    }
    s.battery_temperature_valid = true;
    s.battery_temperature = 28.5f;
    return s;
}

void
apply(uint64_t time) {
    auto s = generate(time);
    auto& m = demo::model();
    m.battery = {s.battery_valid,
                 s.battery_percent,
                 static_cast<float>(s.battery_percent),
                 s.battery_voltage,
                 s.battery_temperature_valid,
                 s.battery_temperature,
                 s.charging ? hal::ChargeState::Normal : hal::ChargeState::NotCharging,
                 false};
    m.gnss.satellites_used = s.satellites;
    m.clock = {s.clock_valid, s.hour, s.minute, s.second, hal::ClockSource::Uptime, time};
    m.wifi.on = s.wifi;
    // 命令的录制结果持续到下一模拟边沿，读取本身不撤销用户操作。
    if (!have_sample || previous_recording != s.recording) {
        m.recorder.recording = m.recorder.reported_recording = s.recording;
    }
    previous_recording = s.recording;
    have_sample = true;
    m.board.uptime_s = static_cast<uint32_t>(time / 1000);
    sample_time = time;
}

page::StatusBarState
current() {
    auto s = generate(sample_time);
    const auto& m = demo::model();
    s.recording = m.recorder.reported_recording;
    return s;
}
} // namespace

void
DemoStatus::configure(Clock clock, bool random, uint32_t seed) {
    source_clock = clock;
    origin = clock ? clock() : 0;
    random_sample = random;
    sample_seed = seed;
    have_sample = false;
    apply(0);
}

void
DemoStatus::refresh() {
    if (source_clock) {
        const uint32_t elapsed = source_clock() - origin;
        if (elapsed != sample_time) {
            apply(elapsed);
        }
    }
}

page::StatusBarState
DemoStatus::sample(uint64_t time) {
    source_clock = nullptr;
    random_sample = false;
    apply(time);
    return current();
}

page::StatusBarState
DemoStatus::sample_random(uint64_t time, uint32_t seed) {
    source_clock = nullptr;
    random_sample = true;
    sample_seed = seed;
    apply(time);
    return current();
}

uint64_t
DemoStatus::uptime_ms() {
    return sample_time;
}
