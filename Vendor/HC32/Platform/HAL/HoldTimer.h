#pragma once
#include <stdint.h>

/** 按住计时：连续按住达到 hold_ms 时触发一次，松开后重新计时。时间差用无符号运算，支持 tick 回绕。 */
class HoldTimer {
  public:
    explicit constexpr HoldTimer(uint32_t hold_ms) : hold_ms_(hold_ms) {}

    /** 每次采样调用；本次按住首次达到时长时返回 true。 */
    bool
    update(bool pressed, uint32_t now) {
        if (!pressed) {
            held_ = fired_ = false;
            return false;
        }
        if (!held_) {
            held_ = true;
            started_ms_ = now;
        }
        if (fired_ || now - started_ms_ < hold_ms_) {
            return false;
        }
        fired_ = true;
        return true;
    }

  private:
    uint32_t hold_ms_;
    uint32_t started_ms_ = 0;
    bool held_ = false;
    bool fired_ = false;
};
