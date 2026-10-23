/** @file spring_tests.cpp @brief 无 LVGL 的弹簧解析解、连续性与输入边界回归。 */
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include "Spring.h"

using app::SpringParams;
using app::SpringValue;

namespace {
unsigned converge(SpringValue& value, unsigned limit_ms = 10000) {
    unsigned elapsed = 0;
    while (!value.settled() && elapsed < limit_ms) {
        value.step(5);
        elapsed += 5;
        assert(std::isfinite(value.position()) && std::isfinite(value.velocity()));
    }
    assert(value.settled() && value.position() == value.target() && value.velocity() == 0);
    return elapsed;
}
void damping_forms() {
    for (float damping : {6.0f, 20.0f, 40.0f}) {
        SpringValue value(0, {100, damping, 1});
        value.retarget(100);
        bool overshot = false;
        for (unsigned i = 0; i < 2000 && !value.settled(); ++i) {
            value.step(5);
            overshot |= value.position() > 100;
            if (damping >= 20)
                assert(value.velocity() >= 0 && value.position() <= 100);
        }
        assert(overshot == (damping < 20));
        assert(value.settled() && value.position() == 100 && value.velocity() == 0);
    }
    SpringValue critical(0, {100, 20, 1});
    critical.retarget(100);
    critical.step(64);
    const float expected = 100 * (1 - (1 + 0.64f) * std::exp(-0.64f));
    assert(std::fabs(critical.position() - expected) < 0.0001f);
    assert(std::fabs(critical.velocity() - 640 * std::exp(-0.64f)) < 0.0001f);
    /** 解析解应与分帧无关，且 k/c/m 同比缩放不改变轨迹。 */
    for (float damping : {6.0f, 20.0f, 40.0f}) {
        SpringValue whole(0, {100, damping, 1}), split(0, {300, damping * 3, 3});
        whole.retarget(100);
        split.retarget(100);
        whole.step(60);
        for (unsigned i = 0; i < 6; ++i)
            split.step(10);
        assert(std::fabs(whole.position() - split.position()) < 0.001f);
        assert(std::fabs(whole.velocity() - split.velocity()) < 0.001f);
    }
}
void duration_and_retarget() {
    for (float duration : {150.0f, 250.0f, 500.0f}) {
        const auto params = SpringParams::from_duration(duration, 0.15f);
        const float omega = 6000 / duration;
        assert(std::fabs(params.stiffness - omega * omega) < 0.001f);
        assert(std::fabs(params.damping - 1.7f * omega) < 0.001f);
        SpringValue value(0, params);
        value.retarget(100);
        const auto elapsed = converge(value);
        assert(elapsed >= duration * 0.5f && elapsed <= duration * 2);
        std::printf("Spring duration %.0f ms: 100-unit step settled at %u ms\n", static_cast<double>(duration), elapsed);
    }
    SpringValue value;
    value.retarget(100);
    value.step(60);
    assert(value.velocity() > 0);
    const float position = value.position(), velocity = value.velocity();
    value.retarget(-100);
    assert(value.position() == position && value.velocity() == velocity);
    value.step(5);
    /** 新目标在左，仍先延续向右的惯性。 */
    assert(value.velocity() > 0);
    converge(value);
    value.retarget(-99.95f);
    assert(!value.settled());
    converge(value);
    value.snap(42);
    assert(value.settled() && value.position() == 42 && value.target() == 42 && value.velocity() == 0);
}
void invalid_and_long_frames() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    for (const auto params : {SpringParams{0, -1, 0}, SpringParams{-10, 0, -2}, SpringParams{nan, infinity, nan}}) {
        SpringValue value(nan, params);
        value.retarget(100);
        converge(value);
        value.retarget(nan);
        value.snap(infinity);
        assert(value.position() == 100 && value.target() == 100);
    }
    for (const auto params : {SpringParams{1.0e30f, 1.0e30f, 1.0e-30f}, SpringParams{1, 1.0e6f, 1}}) {
        SpringValue extreme(0, params);
        extreme.retarget(1.0e30f);
        for (unsigned i = 0; i < 100; ++i) {
            extreme.step(1.0e30f);
            assert(std::isfinite(extreme.position()) && std::isfinite(extreme.velocity()));
        }
    }
    SpringValue long_frame, capped;
    long_frame.retarget(100);
    capped.retarget(100);
    long_frame.step(1000000);
    capped.step(SpringValue::kMaxStepMs);
    assert(long_frame.position() == capped.position() && long_frame.velocity() == capped.velocity());
    const auto position = long_frame.position(), velocity = long_frame.velocity();
    for (float dt : {0.0f, -1.0f, nan, infinity})
        long_frame.step(dt);
    assert(long_frame.position() == position && long_frame.velocity() == velocity);
    converge(long_frame);
    for (float duration : {-1.0f, 0.0f, infinity, nan})
        for (float bounce : {-1.0f, 2.0f, nan}) {
            const auto params = SpringParams::from_duration(duration, bounce);
            assert(std::isfinite(params.stiffness) && params.stiffness > 0);
            assert(std::isfinite(params.damping) && params.damping > 0 && params.mass == 1);
        }
}
} // namespace

int main() {
    damping_forms();
    duration_and_retarget();
    invalid_and_long_frames();
}
