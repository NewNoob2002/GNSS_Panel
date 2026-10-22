#include <array>

#include "Charger.h"
#include <HAL.h>
#include <delay.h>
#include <gpio.h>
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

/** 配置虚拟引脚的初始输出电平。 */
void
gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t state) {
    if (valid(pin) && mode == GPIO_OUTPUT) {
        pins[pin] = state;
    }
}

/** 读取虚拟 GPIO 电平，无效引脚读低。 */
int
gpio_read(gpio_pin_t pin) {
    return valid(pin) ? pins[pin] : static_cast<int>(GPIO_LOW);
}

/** 写入虚拟 GPIO 电平。 */
void
gpio_write(gpio_pin_t pin, uint32_t value) {
    if (valid(pin)) {
        pins[pin] = value == GPIO_HIGH;
    }
}

/** 翻转虚拟 GPIO 输出电平。 */
void
gpio_toggle(gpio_pin_t pin) {
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

bool
HAL::I2C_ProbeDevice(uint8_t address) {
    // 虚拟主板：BQ40Z50 始终在线，MP2762A 随 V 键切换的主板版本。
    return (address == 0x0B && systemInfo.board.online_device.bq40z50)
           || (address == 0x5C && systemInfo.board.online_device.mp2762);
}

void
HAL::Sim_SetPowerKey(bool pressed) {
    // Power 键按下为高电平（与固件 HAL::Key_PowerPressed 一致）。
    gpio_write(POWER_KEY_PIN, pressed);
}
