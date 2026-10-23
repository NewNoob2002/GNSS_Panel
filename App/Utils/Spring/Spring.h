/**
 * @file Spring.h
 * @brief 无堆分配的物理弹簧，公式参考 smooth_ui_toolkit（MIT，Forairaaaaa）。
 * @see https://github.com/Forairaaaaa/smooth_ui_toolkit/blob/3cfe6a3/src/core/animation/generators/spring/spring.cpp
 */
#pragma once

namespace app {
/** @brief 刚度、阻尼及质量；非有限/非正值回退默认值，极端值由 SpringValue 钳制。 */
struct SpringParams {
    float stiffness = 576.0f;
    float damping = 40.8f;
    float mass = 1.0f;

    /** @brief duration_ms 钳制到 16..60000 ms，bounce 到 0..0.95；时长为视觉参考而非硬截止。 */
    static SpringParams from_duration(float duration_ms, float bounce = 0.15f);
};

/** @brief 单值弹簧；速度单位为值/秒，所有调用限同一线程，无异常与动态分配。 */
class SpringValue {
  public:
    static constexpr float kPositionThreshold = 0.1f;
    static constexpr float kVelocityThreshold = 1.0f;
    static constexpr float kMaxStepMs = 64.0f;

    explicit SpringValue(float position = 0, SpringParams params = {});
    /** @brief 切换有限目标（范围 ±1e6），保留当前位置与瞬时速度；非法目标忽略。 */
    void retarget(float target);
    /** @brief 直接落位并清零速度；非法位置忽略。 */
    void snap(float position);
    /** @brief 按解析解推进；dt_ms 非正/非有限时忽略，长帧最多推进 64 ms。 */
    void step(float dt_ms);
    /** @brief step 按误差 ≤0.1、速度 ≤1 判定并吸附；仅在已精确吸附且速度为零时返回 true。 */
    bool settled() const;
    float position() const { return position_; }
    float velocity() const { return velocity_; }
    float target() const { return target_; }

  private:
    float position_ = 0, velocity_ = 0, target_ = 0;
    float alpha_ = 0, omega_squared_ = 0;
};
} // namespace app
