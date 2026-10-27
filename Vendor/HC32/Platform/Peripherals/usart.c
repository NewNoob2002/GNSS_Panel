#include "usart.h"
#include <hc32_ll.h>
#include "board_pins.h"

static bool usart_initialized = false;

bool
usart_init(void) {
    stc_usart_uart_init_t config;
    if (__get_IPSR() != 0U) {
        return false;
    }
    GPIO_SetFunc(CONFIG_DEBUG_UART_TX_PORT, CONFIG_DEBUG_UART_TX_PIN, CONFIG_DEBUG_UART_TX_FUNC);
    FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART1, ENABLE);
    if (USART_UART_StructInit(&config) != LL_OK) {
        return false;
    }
    config.u32ClockDiv = USART_CLK_DIV4;
    config.u32Baudrate = CONFIG_DEBUG_UART_BAUD;
    config.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    config.u32DataWidth = USART_DATA_WIDTH_8BIT;
    config.u32StopBit = USART_STOPBIT_1BIT;
    config.u32Parity = USART_PARITY_NONE;
    if (USART_UART_Init(CM_USART1, &config, NULL) != LL_OK) {
        return false;
    }
    USART_FuncCmd(CM_USART1, USART_TX, ENABLE);
    usart_initialized = true;
    return true;
}

int
usart_write(int ch) {
    if (!usart_initialized || __get_IPSR() != 0U) {
        return 0;
    }
    // 不依赖 SysTick/中断推进，外设异常时也能退出。
    uint32_t timeout = 100000U;
    while (RESET == USART_GetStatus(CM_USART1, USART_FLAG_TX_EMPTY)) {
        if (--timeout == 0U) {
            return 0;
        }
    }
    USART_WriteData(CM_USART1, (uint16_t)ch);
    return 1;
}

size_t
usart_write_buffer(const uint8_t* buffer, size_t size) {
    size_t written = 0;
    if (!buffer) {
        return 0;
    }
    while (written < size && usart_write(buffer[written])) {
        ++written;
    }
    return written;
}
