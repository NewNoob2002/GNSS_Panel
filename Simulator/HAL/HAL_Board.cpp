#include <array>

#include "../../Platform/HAL/Charger.h"
#include "Arduino.h"
#include "HAL.h"
#include "lvgl/lvgl.h"

namespace {

std::array<uint8_t, SIM_PIN_COUNT> pins{};

bool
valid(gpio_pin_t pin) {
    return pin >= 0 && pin < SIM_PIN_COUNT;
}

// 模拟器不区分主板版本的充电电路：待机/开机切换无动作，名称随 V 键切换的版本显示。
class SimCharger final : public HAL::Charger {
  public:
    const char*
    name() const override {
        return systemInfo.board.online_device.mp2762 ? "MP2762A (simulated)" : "GPIO (simulated)";
    }

    void
    begin(BatteryInfo_t&) override {}

    void
    update(BatteryInfo_t&) override {}

  protected:
    bool
    apply_enabled(bool, BatteryInfo_t&) override {
        return true;
    }
};

SimCharger charger;

} // namespace

void
pinMode(gpio_pin_t pin, PinMode_TypeDef mode, uint8_t state) {
    if (valid(pin) && mode == OUTPUT) {
        pins[pin] = state;
    }
}

int
digitalRead(gpio_pin_t pin) {
    return valid(pin) ? pins[pin] : LOW;
}

void
digitalWrite(gpio_pin_t pin, uint32_t value) {
    if (valid(pin)) {
        pins[pin] = value ? HIGH : LOW;
    }
}

void
digitalToggle(gpio_pin_t pin) {
    if (valid(pin)) {
        pins[pin] ^= 1U;
    }
}

uint32_t
millis() {
    return lv_tick_get();
}

HAL::Charger&
HAL::Charger_Get() {
    return charger;
}

void
HAL::Sim_SetPowerKey(bool pressed) {
    // Power 键按下为高电平（与固件 HAL::Key_PowerPressed 一致）。
    digitalWrite(POWER_KEY_PIN, pressed ? HIGH : LOW);
}
