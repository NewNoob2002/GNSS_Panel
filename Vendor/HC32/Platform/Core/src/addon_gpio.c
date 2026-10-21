#include "addon_gpio.h"

#if (LL_GPIO_ENABLE == DDL_ON)

// copied from 'hc32f46x_gpio.c'
#define IS_VALID_PORT(x)                                                                                               \
    (((x) == GPIO_PORT_A) || ((x) == GPIO_PORT_B) || ((x) == GPIO_PORT_C) || ((x) == GPIO_PORT_D)                      \
     || ((x) == GPIO_PORT_E) || ((x) == GPIO_PORT_H))

/**
 * get pin configuration, inverse to PORT_Init
 */
int32_t
PORT_GetConfig(uint8_t port, uint16_t pin, stc_gpio_init_t* portConf) {
    DDL_ASSERT(IS_VALID_PORT(port));
    // PORT_Unlock();
    __IO uint16_t* PCRx;
    for (uint8_t pinPos = 0u; pinPos < 16u; pinPos++) {
        if ((pin & 1U) != 0U) {
            PCRx = &(*(__IO uint16_t*)(&CM_GPIO->PCRA0 + 0x40ul * port + 0x4ul * pinPos));
            uint16_t reg_value = READ_REG16(*PCRx);
            // input latch setting
            portConf->u16PinOutputType = (reg_value >> GPIO_PCR_NOD_POS) & 0x01;
            portConf->u16PinDrv = (reg_value >> GPIO_PCR_DRV_POS) & 0x03;
            portConf->u16PullUp = (reg_value >> GPIO_PCR_PUU_POS) & 0x01;
            portConf->u16Latch = (reg_value >> GPIO_PCR_LTE_POS) & 0x01;
        }
        pin >>= 1U;
        if (0U == pin) {
            break;
        }
    }

    // PORT_Lock();
    return LL_OK;
}

int32_t
PORT_GetFunc(uint8_t port, uint16_t pin, uint16_t* funcSel, en_functional_state_t* subFuncEn) {
    DDL_ASSERT(IS_VALID_PORT(port));
    // PORT_Unlock();
    __IO uint16_t* PFSRx;
    for (uint8_t pinPos = 0u; pinPos < 16u; pinPos++) {
        if ((pin & 1U) != 0U) {
            PFSRx = &(*(__IO uint16_t*)(&CM_GPIO->PFSRA0 + 0x40ul * port + 0x4ul * pinPos));

            // main function
            uint16_t reg_value = READ_REG16(*PFSRx);
            *funcSel = reg_value & GPIO_PFSR_FSEL;

            // subfunction enable
            *subFuncEn = reg_value & GPIO_PFSR_BFE;
        }
        pin >>= 1U;
    }

    // PORT_Lock();
    return LL_OK;
}

void
disable_JTAG() {
    WRITE_REG16(CM_GPIO->PSPCR, 0x03);
}
#endif
