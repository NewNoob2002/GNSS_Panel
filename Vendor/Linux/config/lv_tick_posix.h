#ifndef RK3506_LV_TICK_POSIX_H
#define RK3506_LV_TICK_POSIX_H

#include <stdint.h>
#include <time.h>

static inline uint32_t lv_tick_posix_ms(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;
    return (uint32_t)((uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u);
}

#endif
