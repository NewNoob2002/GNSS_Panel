#pragma once
#include "Arduino.h"

/** 按固定周期翻转的指示灯；周期为 0 时不翻转，引脚电平由 write() 决定。 */
class BlinkLed {
  public:
    explicit constexpr BlinkLed(gpio_pin_t pin) : pin_(pin) {}

    void
    set_period(uint16_t period_ms) {
        period_ms_ = period_ms;
    }

    void
    write(bool on) const {
        digitalWrite(pin_, on ? HIGH : LOW);
    }

    /** 到达周期时翻转一次。 */
    void
    update(uint32_t now) {
        if (period_ms_ > 0 && now - last_toggle_ms_ >= period_ms_) {
            last_toggle_ms_ = now;
            digitalToggle(pin_);
        }
    }

  private:
    gpio_pin_t pin_;
    uint16_t period_ms_ = 0;
    uint32_t last_toggle_ms_ = 0;
};
