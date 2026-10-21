/**
  ******************************************************************************
  * @file    dwt.c
  * @author  Frederic Pillon
  * @brief   Provide Data Watchpoint and Trace services
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019, STMicroelectronics
  * All rights reserved.
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

#include "dwt.h"

#ifdef DWT_BASE
#ifdef __cplusplus
extern "C" {
#endif

static bool dwt_enabled = false;

uint32_t
dwt_init(void) {

    /* Enable use of DWT */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }

    /* Reset the clock cycle counter value */
    DWT->CYCCNT = 0;

    /* Enable  clock cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 3 NO OPERATION instructions */
    __asm volatile(" nop      \n\t"
                   " nop      \n\t"
                   " nop      \n\t");

    /* Check if clock cycle counter has started */
    if (DWT->CYCCNT > 0) {
        dwt_enabled = true;
    } else {
        dwt_enabled = false;
    }
    return 0;
}

bool
dwt_getStatus() {
    return dwt_enabled;
}

#ifdef __cplusplus
}
#endif

#endif