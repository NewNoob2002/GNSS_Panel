#include "delay.h"
#include "HAL.h"
#include "dwt.h"
/** 延续现有可覆盖的等待钩子，声明与 Arduino/yield.c 的弱符号保持一致。 */
void yield(void);

uint32_t HAL_GetTick(void);

uint32_t
millis(void) {
    // ToDo: ensure no interrupts
    return HAL_GetTick();
}

// Interrupt-compatible version of micros
uint32_t
micros(void) {
    // pi-lens-ignore: identifier
    const uint32_t irq_state = __get_PRIMASK();
    // pi-lens-ignore: identifier
    __disable_irq();
    uint32_t tick = HAL_GetTick();
    uint32_t remaining = SysTick->VAL;
    const uint32_t period = SysTick->LOAD + 1U;
    if (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) {
        tick += HAL_GetTickFreq();
        remaining = SysTick->VAL;
    }
    // pi-lens-ignore: identifier
    __set_PRIMASK(irq_state);
    const uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    return tick * 1000U + (cycles_per_us ? (period - remaining) / cycles_per_us : 0U);
}

void
delay_ms(uint32_t ms) {
    if (ms != 0) {
        uint32_t start = HAL_GetTick();
        do {
            yield();
        } while (HAL_GetTick() - start < ms);
    }
}

void
delay_us(uint32_t us) {
#if defined(DWT_BASE)
    if (dwt_getStatus()) {
        const uint32_t cycles_per_us = SystemCoreClock / 1000000U;
        while (us != 0U) {
            const uint32_t chunk = us > 1000000U ? 1000000U : us;
            const uint32_t start = dwt_getCycles();
            const uint32_t cycles = chunk * cycles_per_us;
            while ((uint32_t)(dwt_getCycles() - start) < cycles) {}
            us -= chunk;
        }
    } else {
        /* 2 NO OPERATION instructions */
        __asm volatile(" nop      \n\t"
                       " nop      \n\t");
    }
#else
    __IO uint32_t currentTicks = SysTick->VAL;
    /* Number of ticks per millisecond */
    const uint32_t tickPerMs = SysTick->LOAD + 1;
    /* Number of ticks to count */
    const uint32_t nbTicks = ((us - ((us > 0) ? 1 : 0)) * tickPerMs) / 1000;
    /* Number of elapsed ticks */
    uint32_t elapsedTicks = 0;
    __IO uint32_t oldTicks = currentTicks;
    do {
        currentTicks = SysTick->VAL;
        elapsedTicks += (oldTicks < currentTicks) ? tickPerMs + oldTicks - currentTicks : oldTicks - currentTicks;
        oldTicks = currentTicks;
    } while (nbTicks > elapsedTicks);
#endif
}