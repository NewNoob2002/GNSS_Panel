#pragma once

#include <gpio.h>

/** 轻量引脚句柄；不持有硬件资源，构造只保存编号，允许多个句柄引用同一引脚。 */
class GpioPin {
  public:
    /** 保存有效的板级引脚编号；不访问硬件。 */
    explicit constexpr GpioPin(gpio_pin_t pin) : pin(pin) {}

    /** 沿用底层 GPIO 配置语义，初始电平与方向一起交给 GPIO_Init。 */
    void
    setMode(gpio_mode_t mode, bool initialLevel = false) const {
        gpio_set_mode(pin, mode, initialLevel);
    }

    /** 写入输出电平，不改变模式。 */
    void
    write(bool high) const {
        gpio_write(pin, high);
    }

    /** 读取输入寄存器并返回当前逻辑电平。 */
    bool
    read() const {
        return gpio_read(pin) == GPIO_HIGH;
    }

    /** 翻转输出电平，不改变模式。 */
    void
    toggle() const {
        gpio_toggle(pin);
    }

  private:
    gpio_pin_t pin;
};
