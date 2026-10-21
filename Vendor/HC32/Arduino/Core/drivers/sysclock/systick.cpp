#include <hc32_ll.h>

#include "systick.h"
/**
 * @brief system uptime counter, incremented by systick interrupt every 1ms
 */
#ifndef SYSTICK_TICK_FREQ
#define SYSTICK_TICK_FREQ     1000 // Hz
#endif

#define SYSTICK_TICK_STEP   (1000 / SYSTICK_TICK_FREQ)
#define SYSTICK_LOAD_VALUE      (SystemCoreClock / SYSTICK_TICK_FREQ)
#define SYSTICK_MILLIS          (SystemTickCount * SYSTICK_TICK_INTERVAL)
#define CYCLES_PER_MICROSECOND   (SystemCoreClock / 1000000U)

static volatile uint32_t SystemTickCount = 0;


extern "C" void SysTick_Handler(void)
{
  SystemTickCount++;
	__DSB();
}

void systick_init()
{
  stc_clock_freq_t clkFreq;
  CLK_GetClockFreq(&clkFreq);
  SysTick_Config(clkFreq.u32SysclkFreq / SYSTICK_TICK_FREQ); // tick every 1 ms
}

uint32_t systick_millis()
{
  return SystemTickCount;
}

uint32_t systick_micros()
{
  // based on implementation by STM32duino
  // https://github.com/stm32duino/Arduino_Core_STM32/blob/586319c6c2cee268747c8826d93e84b26d1549fd/libraries/SrcWrapper/src/stm32/clock.c#L29

  // read systick counter value and millis counter value
  uint32_t ms = SystemTickCount;
  volatile uint32_t ticks = SysTick->VAL;

  // and again
  uint32_t ms2 = SystemTickCount;
  volatile uint32_t ticks2 = SysTick->VAL;

  // if ms != ms2, then a systick occurred between the two reads
  // and we must use ms2 and ticks2
  if (ms != ms2)
  {
    ms = ms2;
    ticks = ticks2;
  }

  // get ticks per ms
  const uint32_t ticks_per_ms = SysTick->LOAD + 1;

  // calculate microseconds
  return (ms * 1000) + (((ticks_per_ms - ticks) * 1000) / ticks_per_ms);
}

uint32_t millis()
{
	return systick_millis();
}

uint32_t micros()
{
	return systick_micros();
}

void delay_ms(uint32_t ms)
{
    uint32_t tickstart = SystemTickCount;
    uint32_t wait = ms / SYSTICK_TICK_STEP;

    while((SystemTickCount - tickstart) < wait)
    {
    }
}

void delay_us(uint32_t us)
{
    uint32_t total = 0;
    uint32_t target = CYCLES_PER_MICROSECOND * us;
    int last = SysTick->VAL;
    int now = last;
    int diff = 0;
start:
    now = SysTick->VAL;
    diff = last - now;
    if(diff > 0)
    {
        total += diff;
    }
    else
    {
        total += diff + SYSTICK_LOAD_VALUE;
    }
    if(total > target)
    {
        return;
    }
    last = now;
    goto start;
}
