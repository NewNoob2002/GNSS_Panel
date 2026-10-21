#pragma once
#include "hc32_ll.h"
#include <stdint.h>

#define STRUCT_ZERO(x)									(memset(&x, 0, sizeof(x)))

#define USART_RX_PORT                   (GPIO_PORT_A)   /* PB9: USART1_RX */
#define USART_RX_PIN                    (GPIO_PIN_11)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_33)

#define USART_TX_PORT                   (GPIO_PORT_A)   /* PE6: USART1_TX */
#define USART_TX_PIN                    (GPIO_PIN_12)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_32)

/* USART unit definition */
#define USART_UNIT                      (CM_USART1)
#define USART_FCG_ENABLE()              (FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART1, ENABLE))

#ifdef __cplusplus
extern "C"
{
#endif

void usart_init();
void usart_deinit();
size_t usart_write_buffer(const uint8_t *buffer, size_t size);
int usart_write(int ch);
#ifdef __cplusplus
}
#endif