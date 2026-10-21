/*
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Arduino_h
#define Arduino_h

/* c lib header*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "HAL.h"
#include "core_types.h"
#include "core_debug.h"
#include "yield.h"
#include "delay.h"
#include "dwt.h"
#include "gpio.h"
#include "mcu_config.h"
// some libraries and sketches depend on this AVR stuff,
// assuming Arduino.h or WProgram.h automatically includes it...
//
#include "pgmspace.h"
#include "dtostrf.h"

#include "binary.h"
#include "itoa.h"


// The following headers are for C++ only compilation
#ifdef __cplusplus
#include "WCharacter.h"
#include "WString.h"
//#include "Tone.h"
#include "WMath.h"
//#include "HardwareSerial.h"
// #include "RingBuf.h"
#include "Stream.h"
//#include "pulse.h"
#endif

#include "wiring_constants.h"
#include "wiring_digital.h"
//#include "wiring_analog.h"
//#include "wiring_shift.h"
//#include "WInterrupts.h"

// undefine stdlib's abs if encountered
#ifdef abs
#undef abs
#endif // abs

#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define interrupts() __enable_irq()
#define noInterrupts() __disable_irq()

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

#define delay(ms)                    delay_ms(ms)
#define delayMicroseconds(us)        delay_us(us)

#define NOT_A_PIN                    0xFF
#define NOT_A_PORT                   0xFF
#define NOT_AN_INTERRUPT             -1
// dynamic F_CPU
#ifndef F_CPU
  #define F_CPU (SystemCoreClock)
#endif

#endif // Arduino_h
