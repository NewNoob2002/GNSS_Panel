#pragma once
#include <stdbool.h>
#include <stdint.h>
#define DWT_BASE 1
extern uint32_t SystemCoreClock;

struct FakeSysTick {
    uint32_t VAL, LOAD;
};

struct FakeScb {
    uint32_t ICSR;
};
extern struct FakeSysTick fake_systick;
extern struct FakeScb fake_scb;
extern uint32_t clock_primask;
#define SysTick                (&fake_systick)
#define SCB                    (&fake_scb)
#define SCB_ICSR_PENDSTSET_Msk (1U << 26)

static inline uint32_t
__get_PRIMASK(void) {
    return clock_primask;
}

static inline void
__disable_irq(void) {
    clock_primask = 1;
}

static inline void
__set_PRIMASK(uint32_t state) {
    clock_primask = state;
}

static inline bool
dwt_getStatus(void) {
    return false;
}

static inline uint32_t
dwt_getCycles(void) {
    return 0;
}
