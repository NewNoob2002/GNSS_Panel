#ifndef OTS_CORE_H
#define OTS_CORE_H
#include "hc32_ll.h"

#ifdef __cplusplus
extern "C" {
#endif
void OtsInitConfig(const func_ptr_t callback);
void OtsStart(void);
#ifdef __cplusplus
}
#endif

#endif