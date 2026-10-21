#pragma once
#include "hc32_ll.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief inverse of PORT_Init
    */
    int32_t PORT_GetConfig(uint8_t port, uint16_t pin, stc_gpio_init_t *portConf);

    /**
     * @brief inverse of PORT_SetFunc
    */
    int32_t PORT_GetFunc(uint8_t port, uint16_t pin, uint16_t *funcSel, en_functional_state_t *subFuncEn);
		void disable_JTAG();
#ifdef __cplusplus
}
#endif
