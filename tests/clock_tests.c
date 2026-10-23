#include <assert.h>
#include <stdio.h>
#include <delay.h>
#include "time_fakes/dwt.h"
#include "time_fakes/yield.h"
uint32_t SystemCoreClock = 200000000U;
uint32_t clock_primask;
struct FakeSysTick fake_systick;
struct FakeScb fake_scb;
static uint32_t tick, frequency = 1;

uint32_t
HAL_GetTick(void) {
    return tick;
}

uint32_t
HAL_GetTickFreq(void) {
    return frequency;
}

void
yield(void) {}



int
main(void) {
    tick = 12;
    fake_systick.LOAD = 199999;
    fake_systick.VAL = 100000;
    assert(micros() == 12500U && clock_primask == 0);
    fake_scb.ICSR = SCB_ICSR_PENDSTSET_Msk;
    assert(micros() == 13500U);
    clock_primask = 1;
    assert(micros() == 13500U && clock_primask == 1);
    fake_scb.ICSR = 0;
    tick = UINT32_MAX;
    assert(micros() == (uint32_t)(UINT32_MAX * 1000U + 500U));
    tick = 20;
    frequency = 10;
    fake_systick.LOAD = 1999999;
    fake_systick.VAL = 1000000;
    assert(micros() == 25000U);
    fake_scb.ICSR = SCB_ICSR_PENDSTSET_Msk;
    assert(micros() == 35000U);
    puts("microsecond clock regression passed");
}
