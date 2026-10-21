#include "support.h"

#include <inttypes.h>
#include <stdio.h>

const char*
MakeTimeString(uint32_t ms, char* buf, uint16_t len) {
    uint64_t ss = ms / 1000;
    uint64_t mm = ss / 60;
    uint32_t hh = (uint32_t)(mm / 60);

    snprintf(buf, len, "%" PRIu32 ":%02" PRIu32 ":%02" PRIu32, hh, (uint32_t)(mm % 60), (uint32_t)(ss % 60));

    return buf;
}

void
MakeTime(uint32_t ms, makeTime_t* time) {
    uint64_t ss = ms / 1000;
    uint64_t mm = ss / 60;
    uint32_t hh = (uint32_t)(mm / 60);

    time->second = ss % 60;
    time->minute = mm % 60;
    time->hour = hh;
}
