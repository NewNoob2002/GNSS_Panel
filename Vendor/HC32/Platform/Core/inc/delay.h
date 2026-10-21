#ifndef DELAY_h
#define DELAY_h
#include <stdint.h>
#include "../../Drivers/inc/hc32_ll.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Returns the number of milliseconds since the Arduino board began running the current program.
 *
 * This number will overflow (go back to zero), after approximately 50 days.
 *
 * \return Number of milliseconds since the program started (uint32_t)
 */
extern uint32_t millis(void);

/**
 * \brief Returns the number of microseconds since the Arduino board began running the current program.
 *
 * This number will overflow (go back to zero), after approximately 70 minutes. On 16 MHz Arduino boards
 * (e.g. Duemilanove and Nano), this function has a resolution of four microseconds (i.e. the value returned is
 * always a multiple of four). On 8 MHz Arduino boards (e.g. the LilyPad), this function has a resolution
 * of eight microseconds.
 *
 * \note There are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second.
 */
extern uint32_t micros(void);

extern void delay_ms(uint32_t ms);

extern void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif