#ifndef __HAL_CONFIG_DEF
#define __HAL_CONFIG_DEF

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_CORTEX_MODULE_ENABLED

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "HAL_CORTEX.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

/* unlock/lock peripheral */
#define EXAMPLE_PERIPH_WE (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define EXAMPLE_PERIPH_WP (LL_PERIPH_FCG | LL_PERIPH_SRAM)

#define XTAL_PORT         (GPIO_PORT_H)
#define XTAL_PIN          (GPIO_PIN_00 | GPIO_PIN_01)

#ifdef __cplusplus
}
#endif

#endif