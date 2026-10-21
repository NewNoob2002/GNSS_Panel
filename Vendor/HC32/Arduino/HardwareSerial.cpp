#include "core_log.h"
#include "hc32_ll_usart.h"
#include "hc32f460.h"
#include <hc32_ll.h>
#include <gpio.h>
#include <irqn.h>
#include <core_debug.h>
#include <sysclock.h>

#include <HardwareSerial.h>

#if SERIAL_1_ENABLE
HardwareSerial Serial(&USART1_config, SERIAL_1_TX_PIN, SERIAL_1_RX_PIN);
uint8_t usart1_rx_buffer[SERIAL_RX_BUFFER_SIZE];
uint8_t usart1_tx_buffer[SERIAL_TX_BUFFER_SIZE];
#endif

#if SERIAL_2_ENABLE
HardwareSerial Serial2(&USART2_config, SERIAL_2_TX_PIN, SERIAL_2_RX_PIN);
#endif

#if SERIAL_3_ENABLE
HardwareSerial Serial3(&USART3_config, SERIAL_3_TX_PIN, SERIAL_3_RX_PIN);
#endif

//
// IRQ register / unregister helper
//
inline void usart_irq_register(usart_interrupt_config_t &irq, const char *name, uint32_t priority = DDL_IRQ_PRIO_05)
{
    // get auto-assigned irqn and set in irq struct
    IRQn_Type irqn;
    irqn_aa_get(irqn, name);
    irq.interrupt_number = irqn;

    // create irq registration struct
    stc_irq_signin_config_t irqConf = {
        .enIntSrc    = irq.interrupt_source,
        .enIRQn      = irq.interrupt_number,
        .pfnCallback = irq.interrupt_handler,
    };

    // register and enable irq
    INTC_IrqSignIn(&irqConf);
    NVIC_SetPriority(irqConf.enIRQn, priority);
    NVIC_ClearPendingIRQ(irqConf.enIRQn);
    NVIC_EnableIRQ(irqConf.enIRQn);
}

inline void usart_irq_resign(usart_interrupt_config_t &irq, const char *name)
{
    // disable interrupt and clear pending
    NVIC_DisableIRQ(irq.interrupt_number);
    NVIC_ClearPendingIRQ(irq.interrupt_number);
    INTC_IrqSignOut(irq.interrupt_number);

    // resign auto-assigned irqn
    irqn_aa_resign(irq.interrupt_number, name);
}

//
// debug print helpers
//
#define USART_REG_TO_X(reg) \
    reg == CM_USART1   ? 1  \
    : reg == CM_USART2 ? 2  \
    : reg == CM_USART3 ? 3  \
    : reg == CM_USART4 ? 4  \
                       : 0
#define USART_DEBUG_PRINTF(fmt, ...) \
    CORE_DEBUG_PRINTF("[USART%d] " fmt, USART_REG_TO_X(this->usart_config->peripheral.register_base), ##__VA_ARGS__)

// void HardwareSerial::USART_rx_data_available_irq(void)
// {
//     uint8_t ch = USART_ReadData(this->usart_config->peripheral.register_base);
//     bool rxOverrun;
//     this->usart_config->state.rx_buffer->push(ch, true, rxOverrun);
// }

HardwareSerial::HardwareSerial(struct usart_config_t *usart_config,
                               gpio_pin_t tx_pin,
                               gpio_pin_t rx_pin)
{
    CORE_ASSERT(usart_config != NULL, "usart_config is NULL");
    ASSERT_GPIO_PIN_VALID(tx_pin, "tx_pin is invalid");
    ASSERT_GPIO_PIN_VALID(rx_pin, "rx_pin is invalid");

    this->usart_config = usart_config;
    this->tx_pin       = tx_pin;
    this->rx_pin       = rx_pin;

    // // 关联RingBuffer到USART状态结构体
    this->_rx_buffer = nullptr;
    this->_tx_buffer = nullptr;
    this->usart_config->state.rx_buffer = nullptr;
    this->usart_config->state.tx_buffer = nullptr;

    this->_tx_timeout = 1000;

    this->_is_initialized = false;
}

HardwareSerial::~HardwareSerial()
{
    // 释放RingBuffer
    delete this->_rx_buffer;
    delete this->_tx_buffer;

    this->_rx_buffer = nullptr;
    this->_tx_buffer = nullptr;

    this->usart_config->state.rx_buffer = nullptr;
    this->usart_config->state.tx_buffer = nullptr;
}

void HardwareSerial::begin(uint32_t baud)
{
    begin(baud, SERIAL_8N1);
}

void HardwareSerial::begin(uint32_t baud, uint16_t config)
{
    stc_usart_uart_init_t uart_init;
    (void)USART_UART_StructInit(&uart_init);
    uart_init.u32ClockDiv      = USART_CLK_DIV4;
    uart_init.u32Baudrate      = baud;
    uart_init.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    // stop bits
    switch (config & HARDSER_STOP_BIT_MASK) {
        default:
            CORE_ASSERT_FAIL("USART: invalid stop bit configuration");
        case HARDSER_STOP_BIT_1:
            uart_init.u32StopBit = USART_STOPBIT_1BIT;
            break;
        case HARDSER_STOP_BIT_2:
            uart_init.u32StopBit = USART_STOPBIT_2BIT;
            break;
    }

    // parity
    switch (config & HARDSER_PARITY_MASK) {
        default:
            CORE_ASSERT_FAIL("USART: invalid parity configuration");
        case HARDSER_PARITY_NONE:
            uart_init.u32Parity = USART_PARITY_NONE;
            break;
        case HARDSER_PARITY_EVEN:
            uart_init.u32Parity = USART_PARITY_EVEN;
            break;
        case HARDSER_PARITY_ODD:
            uart_init.u32Parity = USART_PARITY_ODD;
            break;
    }

    // data bits
    switch (config & HARDSER_DATA_MASK) {
        default:
            CORE_ASSERT_FAIL("USART: invalid data bits configuration");
        case HARDSER_DATA_8:
            uart_init.u32DataWidth = USART_DATA_WIDTH_8BIT;
            break;
    }

#ifdef USART_AUTO_CLKDIV_OS_CONFIG
    // auto-calculate best clock divider and oversampling mode for the given baudrate
    setCalculatedClockDivAndOversampling(&usartConfig, baud);
#endif

    // call begin with full config
    begin(&uart_init);
}

void HardwareSerial::begin(const stc_usart_uart_init_t *config, const bool rxNoiseFilter)
{
    // this->_rx_buffer->clear();
    // this->_tx_buffer->clear();
    CM_USART_TypeDef *USARTx = this->usart_config->peripheral.register_base;
    // set io
    GPIO_SetFunction(this->tx_pin, this->usart_config->peripheral.tx_pin_function);
    GPIO_SetFunction(this->rx_pin, this->usart_config->peripheral.rx_pin_function);

    FCG_Fcg1PeriphClockCmd(this->usart_config->peripheral.clock_id, ENABLE);
    // set config
    int32_t ret = USART_UART_Init(USARTx, config, NULL);
    if (ret != LL_OK) {
        CORE_ASSERT_FAIL("USART: failed to initialize");
    }
    if (USARTx == CM_USART1) {
        this->_rx_buffer = new RingBuffer<uint8_t>(usart1_rx_buffer, SERIAL_RX_BUFFER_SIZE);
        this->_rx_buffer->clear();
        this->_tx_buffer = new RingBuffer<uint8_t>(usart1_tx_buffer, SERIAL_TX_BUFFER_SIZE);
        this->_tx_buffer->clear();
    }
    // else if(USARTx == CM_USART2)
    // {
    //     this->usart_config->state.rx_buffer = new RingBuffer<uint8_t>(usart2_rx_buffer, SERIAL_RX_BUFFER_SIZE);
    // }
    // else if(USARTx == CM_USART3)
    // {
    //     this->usart_config->state.rx_buffer = new RingBuffer<uint8_t>(usart3_rx_buffer, SERIAL_RX_BUFFER_SIZE);
    // }
    this->usart_config->state.rx_buffer = this->_rx_buffer;
    this->usart_config->state.tx_buffer = this->_tx_buffer;

    usart_irq_register(this->usart_config->interrupts.rx_error, "USART_RX_ERROR");
    // usart_irq_register(this->usart_config->interrupts.tx_buffer_empty, "USART_TX_BUFFER_EMPTY");
    // usart_irq_register(this->usart_config->interrupts.tx_complete, "USART_TX_COMPLETE");

#ifdef USART_RX_DMA_SUPPORT
// dma config
#else
    // irq config
    usart_irq_register(this->usart_config->interrupts.rx_data_full, "USART_RX_DATA_FULL");
#endif

    // enable usart RX + interrupts
    // (tx is enabled on-demand when data is available to send)
    USART_FuncCmd(this->usart_config->peripheral.register_base, USART_TX, ENABLE);
    USART_FuncCmd(this->usart_config->peripheral.register_base, USART_RX, ENABLE);
    USART_FuncCmd(this->usart_config->peripheral.register_base, USART_INT_RX, ENABLE);

    // write debug message AFTER init (this UART may be used for the debug message)
    USART_DEBUG_PRINTF("begin completed\n");
    this->_is_initialized = true;
}

void HardwareSerial::end()
{
    USART_DEBUG_PRINTF("end");

    // flush buffers
    flush();

    this->_is_initialized = false;
    // disable usart RX + interrupts
    USART_FuncCmd(this->usart_config->peripheral.register_base, USART_RX, DISABLE);
    USART_FuncCmd(this->usart_config->peripheral.register_base, USART_TX, DISABLE);

    // disable interrupts
    usart_irq_resign(this->usart_config->interrupts.rx_error, "USART_RX_ERROR");
    // usart_irq_resign(this->usart_config->interrupts.tx_buffer_empty, "USART_TX_BUFFER_EMPTY");
    // usart_irq_resign(this->usart_config->interrupts.tx_complete, "USART_TX_COMPLETE");

#ifdef USART_RX_DMA_SUPPORT
// dma config
#else
    // irq config
    usart_irq_resign(this->usart_config->interrupts.rx_data_full, "USART_RX_DATA_FULL");
#endif

    USART_DeInit(this->usart_config->peripheral.register_base);

    FCG_Fcg1PeriphClockCmd(this->usart_config->peripheral.clock_id, DISABLE);

    // this->_rx_buffer->clear();
    // this->_tx_buffer->clear();

    USART_DEBUG_PRINTF("end completed\n");
}

int HardwareSerial::available()
{
    // return ((unsigned int)(SERIAL_RX_BUFFER_SIZE + _rxBufferHead - _rxBufferTail)) % SERIAL_RX_BUFFER_SIZE;
    return this->_rx_buffer->count();
}

int HardwareSerial::availableForWrite()
{
    // return this->_tx_buffer->capacity() - this->_tx_buffer->count();
    return USART_GetStatus(this->usart_config->peripheral.register_base, USART_FLAG_TX_EMPTY);
}

int HardwareSerial::peek()
{
    // if (_rxBufferHead == _rxBufferTail) {
    //     return -1;
    // } else {
    //     return _rxBuffer[_rxBufferTail];
    // }
    uint8_t ch;
    if (this->_rx_buffer->peek()) {
        return ch;
    }
    return -1;
}

int HardwareSerial::read()
{
    // if the head isn't ahead of the tail, we don't have any characters
    // if (_rxBufferHead == _rxBufferTail) {
    //     return -1;
    // } else {
    //     uint8_t c     = _rxBuffer[_rxBufferTail];
    //     _rxBufferTail = (uint16_t)(_rxBufferTail + 1) % SERIAL_RX_BUFFER_SIZE;
    //     return c;
    // }
    if (this->_rx_buffer == nullptr) {
        LOG_ERROR("this->_rx_buffer is nullptr");
        return -1;
    }
    uint8_t ch;
    if (this->_rx_buffer->pop(ch)) {
        return ch;
    }
    return -1;
}

void HardwareSerial::flush()
{
    _rxBufferHead = _rxBufferTail;
    // if (!this->_is_initialized) {
    //     return;
    // }
    // while (!this->_tx_buffer->isEmpty()) {
    //     uint8_t ch;
    //     this->_tx_buffer->pop(ch);
    // }
}

size_t HardwareSerial::write(uint8_t ch)
{
    if (!this->_is_initialized) {
        return 0;
    }

    while (RESET == USART_GetStatus(this->usart_config->peripheral.register_base, USART_FLAG_TX_EMPTY)) {
    }
    USART_WriteData(this->usart_config->peripheral.register_base, ch);
    // wrote one byte
    return 1;
}
