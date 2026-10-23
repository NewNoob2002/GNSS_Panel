/**
 * @file Spring.cpp
 * @brief 自行实现三种阻尼的解析解；duration/bounce 换算参考 smooth_ui_toolkit。
 * @see https://github.com/Forairaaaaa/smooth_ui_toolkit/blob/3cfe6a3/src/core/animation/generators/spring/spring.cpp
 * @copyright 公式参考来源 Copyright (c) 2023 Forairaaaaa，MIT License。
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "Spring.h"
#include <algorithm>
#include <cmath>

namespace app {
namespace {
float parameter(float value, float fallback, float maximum) {
    return std::isfinite(value) && value > 0 ? std::clamp(value, 0.001f, maximum) : fallback;
}
float position_limit(float value) {
    return std::clamp(value, -1.0e6f, 1.0e6f);
}
} // namespace

SpringParams SpringParams::from_duration(float duration_ms, float bounce) {
    duration_ms = std::isfinite(duration_ms) ? std::clamp(duration_ms, 16.0f, 60000.0f) : 250.0f;
    bounce = std::isfinite(bounce) ? std::clamp(bounce, 0.0f, 0.95f) : 0.15f;
    const float omega = 6000.0f / duration_ms;
    return {omega * omega, 2.0f * (1.0f - bounce) * omega, 1.0f};
}

SpringValue::SpringValue(float position, SpringParams params) {
    /** 限定物理参数量级，避免非法质量及极端平方运算溢出。 */
    params.stiffness = parameter(params.stiffness, 576.0f, 1.0e6f);
    params.damping = parameter(params.damping, 40.8f, 1.0e6f);
    params.mass = parameter(params.mass, 1.0f, 1000.0f);
    alpha_ = params.damping / (2.0f * params.mass);
    omega_squared_ = params.stiffness / params.mass;
    snap(position);
}

void SpringValue::retarget(float target) {
    if (std::isfinite(target))
        target_ = position_limit(target);
}

void SpringValue::snap(float position) {
    if (!std::isfinite(position))
        return;
    position_ = target_ = position_limit(position);
    velocity_ = 0;
}

bool SpringValue::settled() const {
    return position_ == target_ && velocity_ == 0;
}

void SpringValue::step(float dt_ms) {
    if (!std::isfinite(dt_ms) || dt_ms <= 0)
        return;
    const float t = std::min(dt_ms, kMaxStepMs) * 0.001f;
    const float offset = position_ - target_;
    const float discriminant = alpha_ * alpha_ - omega_squared_;
    float next_offset, next_velocity;
    if (std::fabs(discriminant) <= 1.0e-4f * omega_squared_) {
        const float c = velocity_ + alpha_ * offset;
        const float envelope = std::exp(-alpha_ * t);
        next_offset = envelope * (offset + c * t);
        next_velocity = envelope * (velocity_ - alpha_ * c * t);
    } else if (discriminant < 0) {
        const float omega = std::sqrt(-discriminant);
        const float sine = std::sin(omega * t), cosine = std::cos(omega * t);
        const float envelope = std::exp(-alpha_ * t);
        next_offset = envelope * (offset * cosine + (velocity_ + alpha_ * offset) / omega * sine);
        next_velocity = envelope * (velocity_ * cosine - (alpha_ * velocity_ + omega_squared_ * offset) / omega * sine);
    } else {
        const float beta = std::sqrt(discriminant);
        /** 用两个负实根避免 exp(-alpha*t)*cosh(beta*t) 的溢出及慢根相消。 */
        const float fast = -alpha_ - beta;
        const float slow = -omega_squared_ / (alpha_ + beta);
        const float a = (velocity_ - fast * offset) / (slow - fast);
        const float b = offset - a;
        const float slow_term = a * std::exp(slow * t), fast_term = b * std::exp(fast * t);
        next_offset = slow_term + fast_term;
        next_velocity = slow * slow_term + fast * fast_term;
    }
    position_ = target_ + next_offset;
    velocity_ = next_velocity;
    if (std::fabs(position_ - target_) <= kPositionThreshold && std::fabs(velocity_) <= kVelocityThreshold)
        snap(target_);
}
} // namespace app
