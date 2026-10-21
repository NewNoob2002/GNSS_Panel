#pragma once
#include <stdint.h>

#include "delay.h"

void systick_init();
uint32_t systick_millis();
uint32_t systick_micros();

uint32_t millis();
uint32_t micros();
