#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { DISABLE = 0, ENABLE = 1 } en_functional_state_t;

typedef enum { PIN_RESET = 0, PIN_SET = 1 } en_pin_state_t;

typedef struct {
    uint16_t u16PinState, u16PinDir, u16PinDrv, u16PinAttr, u16Latch;
    uint16_t u16PullUp, u16Invert, u16ExtInt, u16PinOutputType;
} stc_gpio_init_t;

enum {
    LL_OK = 0,
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_H,
    GPIO_FUNC_0 = 0,
    PIN_DIR_IN = 0,
    PIN_DIR_OUT = 0x0002,
    PIN_LOW_DRV = 0,
    PIN_HIGH_DRV = 0x0020,
    PIN_PU_OFF = 0,
    PIN_PU_ON = 0x0040,
    PIN_ATTR_DIGITAL = 0,
    PIN_ATTR_ANALOG = 0x8000,
    PIN_OUT_TYPE_CMOS = 0,
    PIN_OUT_TYPE_NMOS = 0x0004,
};

/** 提供 LL 默认 GPIO 配置。 */
int32_t GPIO_StructInit(stc_gpio_init_t* config);
/** 记录端口、位掩码与完整初始配置。 */
int32_t GPIO_Init(uint8_t port, uint16_t mask, const stc_gpio_init_t* config);
/** 读取虚拟输入电平。 */
en_pin_state_t GPIO_ReadInputPins(uint8_t port, uint16_t mask);
/** 记录置高操作。 */
void GPIO_SetPins(uint8_t port, uint16_t mask);
/** 记录置低操作。 */
void GPIO_ResetPins(uint8_t port, uint16_t mask);
/** 记录翻转操作。 */
void GPIO_TogglePins(uint8_t port, uint16_t mask);
/** 记录复用功能选择。 */
void GPIO_SetFunc(uint8_t port, uint16_t mask, uint16_t function);

#ifdef __cplusplus
}
#endif
