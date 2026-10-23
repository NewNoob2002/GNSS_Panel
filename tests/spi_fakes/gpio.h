#pragma once
#include "pinmap.h"

enum gpio_mode_t { GPIO_OUTPUT, GPIO_OUTPUT_AF_PP };

constexpr uint8_t GPIO_LOW = 0;
constexpr uint8_t GPIO_HIGH = 1;
void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t level);
void gpio_write(gpio_pin_t pin, uint32_t level);
int gpio_read(gpio_pin_t pin);
void gpio_toggle(gpio_pin_t pin);
void GPIO_SetFunction(gpio_pin_t pin, uint16_t function);
