#include "usart.h"
#include <stdbool.h>
#include <string.h>

static bool usart_initialized = false;

void
usart_init() {
    stc_usart_uart_init_t stcUartInit;

    STRUCT_ZERO(stcUartInit);
    /* Configure USART RX/TX pin. */
    GPIO_SetFunc(USART_RX_PORT, USART_RX_PIN, USART_RX_GPIO_FUNC);
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);
    /* Enable peripheral clock */
    USART_FCG_ENABLE();

    /* Initialize UART. */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32ClockDiv = USART_CLK_DIV4;
    stcUartInit.u32Baudrate = 921600UL;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    USART_UART_Init(USART_UNIT, &stcUartInit, NULL);
    USART_FuncCmd(USART_UNIT, (USART_TX), ENABLE);

    usart_initialized = true;
}

void
usart_deinit() {
    USART_DeInit(USART_UNIT);
    usart_initialized = false;
}

int
usart_write(int ch) {
    if (!usart_initialized) {
        return 0;
    }
    while (RESET == USART_GetStatus(USART_UNIT, USART_FLAG_TX_EMPTY)) {}
    USART_WriteData(USART_UNIT, ch);
    return 1;
}

size_t
usart_write_buffer(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        if (usart_write(*buffer++)) {
            n++;
        } else {
            break;
        }
    }
    return n;
}