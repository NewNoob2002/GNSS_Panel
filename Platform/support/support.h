#ifndef __SYSTEM_H__
#define __SYSTEM_H__
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct makeTime_t {
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} makeTime_t;

const char* MakeTimeString(uint32_t ms, char* buf, uint16_t len);
void MakeTime(uint32_t ms, makeTime_t* time);
#ifdef __cplusplus
}
#endif

#endif