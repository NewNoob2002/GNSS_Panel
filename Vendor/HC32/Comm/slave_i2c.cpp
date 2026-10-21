#include "slave_i2c.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "SystemState.h"
#include "delay.h"
#include "hc32_ll.h" // IWYU pragma: keep
#include "mcu_define.h"
#include "panel_protocol.h"
#include "platform_debug.h"

volatile SLAVE_I2C_STATE slave_state = SLAVE_RX;
static volatile uint16_t tx_head;
static volatile uint16_t tx_tail;
static uint8_t tx_buffer[SLAVE_TX_BUFFER_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static uint8_t rx_buffer[SLAVE_RX_BUFFER_SIZE];
static uint8_t response_buffer[SLAVE_TX_BUFFER_SIZE];
static SEMP_PARSE_STATE* parser;
static volatile bool transaction_active;
static volatile bool tx_has_response;
static volatile uint16_t tx_start_tail;
static volatile uint16_t tx_response_length;
static volatile uint32_t tx_started_bytes;
static volatile uint32_t tx_completed_bytes;
static volatile uint32_t progress_sequence;
static volatile uint32_t completed_reads;
static uint32_t consumed_reads;
static uint32_t last_progress_ms;
static uint32_t polled_progress;
static uint32_t last_rx_ms;
static bool initialized;
static uint16_t pending_response_length;
static bool response_reserved;
volatile SlaveI2cDiagnostics slave_i2c_diagnostics;

static uint32_t
enter_critical() {
    const uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

static void
exit_critical(uint32_t state) {
    __set_PRIMASK(state);
}

static void
reset_parser() {
    parser->state = sempFirstByte;
    parser->length = 0;
    parser->crc = 0;
    parser->computeCrc = nullptr;
}

#if defined(__CORE_DEBUG)
static void
print_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

static bool
bad_crc(SEMP_PARSE_STATE* parse) {
    CORE_DEBUG_PRINTF("I2C CRC error, len=%u residue=0x%08lx", parse->length, (unsigned long)parse->crc);
    return true;
}
#endif

int
txBufferAvailable() {
    return tx_head >= tx_tail ? tx_head - tx_tail : 0;
}

int
txBufferRead() {
    return txBufferAvailable() > 0 ? tx_buffer[tx_tail++] : SLAVE_I2C_TX_EMPTY_BYTE;
}

int
txBufferWrite(const uint8_t* buffer, uint16_t length) {
    if (!buffer || !length || length > sizeof(tx_buffer)) {
        response_reserved = false;
        return -1;
    }
    const uint32_t irq_state = enter_critical();
    if (txBufferAvailable() || slave_state == SLAVE_TX) {
        response_reserved = false;
        ++slave_i2c_diagnostics.response_busy;
        exit_critical(irq_state);
        return -1;
    }
    memcpy(tx_buffer, buffer, length);
    tx_tail = 0;
    tx_head = length;
    response_reserved = false;
    exit_critical(irq_state);
    return length;
}

static void
process_frame(SEMP_PARSE_STATE* parse, uint16_t type) {
    (void)type;
    const uint32_t irq_state = enter_critical();
    const bool available = !response_reserved && pending_response_length == 0U && tx_head == 0
                           && consumed_reads == completed_reads && slave_state != SLAVE_TX;
    if (available) {
        response_reserved = true;
    } else {
        ++slave_i2c_diagnostics.response_busy;
    }
    exit_critical(irq_state);
    if (!available) {
        return;
    }
    const int length = message_decode(parse, response_buffer, sizeof(response_buffer));
    if (length <= 0) {
        response_reserved = false;
        return;
    }
    pending_response_length = static_cast<uint16_t>(length);
    // 编码期间主控可能开始空读；等事务结束后发布，避免业务副作用发生却丢失响应。
    if (slave_state != SLAVE_TX && txBufferWrite(response_buffer, pending_response_length) > 0) {
        pending_response_length = 0;
    }
}

static void
queue_byte() {
    const bool has_byte = tx_has_response && txBufferAvailable() > 0;
    I2C_WriteData(I2C_UNIT, has_byte ? txBufferRead() : SLAVE_I2C_TX_EMPTY_BYTE);
    ++tx_started_bytes;
    if (has_byte) {
        ++slave_i2c_diagnostics.tx_bytes;
    }
    ++progress_sequence;
}

static void
I2C_EEI_Callback() {
    if (SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_MATCH_ADDR0)) {
        const bool read = SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_TRA);
        I2C_ClearStatus(I2C_UNIT, I2C_CLR_STOPFCLR | I2C_CLR_NACKFCLR);
        transaction_active = true;
        ++progress_sequence;
        if (read) {
            slave_state = SLAVE_TX;
            tx_has_response = tx_head != 0;
            tx_start_tail = tx_tail;
            tx_response_length = static_cast<uint16_t>(txBufferAvailable());
            tx_started_bytes = 0;
            tx_completed_bytes = 0;
            // 与 Boot 一致：装入首字节后才释放地址匹配，避免主控读到旧 DTR。
            queue_byte();
            I2C_IntCmd(I2C_UNIT, I2C_INT_TX_CPLT, ENABLE);
        } else {
            slave_state = SLAVE_RX;
        }
        I2C_IntCmd(I2C_UNIT, I2C_INT_STOP | I2C_INT_NACK | I2C_INT_ARBITRATE_FAIL, ENABLE);
        I2C_ClearStatus(I2C_UNIT, I2C_CLR_SLADDR0FCLR | I2C_CLR_STARTFCLR);
    }
    if (SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_ARBITRATE_FAIL)) {
        ++slave_i2c_diagnostics.arbitration_lost;
        ++progress_sequence;
        I2C_IntCmd(I2C_UNIT, I2C_INT_TX_CPLT, DISABLE);
        NVIC_ClearPendingIRQ(I2C_TEI_IRQN_DEF);
        if (slave_state == SLAVE_TX && tx_has_response) {
            tx_tail = tx_start_tail;
            tx_has_response = false;
        }
        I2C_ClearStatus(I2C_UNIT, I2C_FLAG_CLR_ARBITRATE_FAIL);
    }
    // NACK 与 STOP 可同时挂起，必须在同次中断内独立处理。
    if (SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_NACKF)) {
        ++progress_sequence;
        I2C_ClearStatus(I2C_UNIT, I2C_CLR_NACKFCLR);
        if (slave_state == SLAVE_TX) {
            tx_completed_bytes = tx_started_bytes;
            I2C_IntCmd(I2C_UNIT, I2C_INT_TX_CPLT, DISABLE);
            NVIC_ClearPendingIRQ(I2C_TEI_IRQN_DEF);
            I2C_ClearStatus(I2C_UNIT, I2C_FLAG_CLR_TX_CPLT);
            (void)I2C_ReadData(I2C_UNIT);
        }
    }
    if (SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_STOP)) {
        ++progress_sequence;
        I2C_IntCmd(I2C_UNIT, I2C_INT_TX_CPLT | I2C_INT_STOP | I2C_INT_NACK | I2C_INT_ARBITRATE_FAIL, DISABLE);
        NVIC_ClearPendingIRQ(I2C_TEI_IRQN_DEF);
        // 清除 START 锁存，避免一次正常事务结束后 BUSY 永久保持。
        I2C_ClearStatus(I2C_UNIT, I2C_CLR_STOPFCLR | I2C_CLR_STARTFCLR);
        transaction_active = false;
        if (slave_state == SLAVE_RX) {
            slave_state = SLAVE_RX_DONE;
        } else if (slave_state == SLAVE_TX) {
            if (tx_has_response && tx_completed_bytes >= tx_response_length) {
                ++completed_reads;
                ++slave_i2c_diagnostics.complete_reads;
                tx_head = 0;
                tx_tail = 0;
                slave_state = SLAVE_TX_DONE;
            } else {
                if (tx_has_response) {
                    tx_tail = tx_start_tail;
                    ++slave_i2c_diagnostics.partial_reads;
                }
                slave_state = SLAVE_RX;
            }
            tx_has_response = false;
        }
    }
}

static void
I2C_TEI_Callback() {
    if (SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_TX_CPLT) && RESET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_NACKF)) {
        tx_completed_bytes = tx_started_bytes;
        queue_byte();
    }
}

static void
I2C_RXI_Callback() {
    const uint8_t byte = I2C_ReadData(I2C_UNIT);
    const uint16_t next = (rx_head + 1U) % SLAVE_RX_BUFFER_SIZE;
    if (next != rx_tail) {
        rx_buffer[rx_head] = byte;
        rx_head = next;
    } else {
        ++slave_i2c_diagnostics.rx_overflow;
        ++State::Diagnostics().i2c_errors;
    }
    ++slave_i2c_diagnostics.rx_bytes;
    ++progress_sequence;
}

static void
deinit_hardware() {
    const IRQn_Type irqs[] = {I2C_EEI_IRQN_DEF, I2C_RXI_IRQN_DEF, I2C_TEI_IRQN_DEF};
    for (IRQn_Type irq : irqs) {
        NVIC_DisableIRQ(irq);
        NVIC_ClearPendingIRQ(irq);
        (void)INTC_IrqSignOut(irq);
    }
    (void)I2C_DeInit(I2C_UNIT);
}

static int32_t
register_irq(IRQn_Type irq, en_int_src_t source, uint32_t priority, func_ptr_t callback) {
    stc_irq_signin_config_t config = {};
    config.enIRQn = irq;
    config.enIntSrc = source;
    config.pfnCallback = callback;
    const int32_t result = INTC_IrqSignIn(&config);
    if (result == LL_OK) {
        NVIC_ClearPendingIRQ(irq);
        NVIC_SetPriority(irq, priority);
        NVIC_EnableIRQ(irq);
    }
    return result;
}

int32_t
slave_i2c_init() {
    if (!parser) {
        static const SEMP_PARSE_ROUTINE routines[] = {sempCustomPreamble};
        static const char* const names[] = {"CUSTOM"};
#if defined(__CORE_DEBUG)
        parser = sempBeginParser(routines, 1, names, 1, 0, SLAVE_RX_BUFFER_SIZE, process_frame, "Panel", print_message,
                                 nullptr, bad_crc);
#else
        parser = sempBeginParser(routines, 1, names, 1, 0, SLAVE_RX_BUFFER_SIZE, process_frame, "Panel");
#endif
        if (!parser) {
            return LL_ERR;
        }
    }
    const uint32_t irq_state = enter_critical();
    // 恢复时重新配置 GPIO，并保持调用前的写保护状态。
    const uint16_t gpio_protection = CM_GPIO->PWPR;
    GPIO_REG_Unlock();
    FCG_Fcg1PeriphClockCmd(I2C_FCG_USE, ENABLE);
    deinit_hardware();
    GPIO_SetFunc(I2C_SCL_PORT, I2C_SCL_PIN, I2C_GPIO_SCL_FUNC);
    GPIO_SetFunc(I2C_SDA_PORT, I2C_SDA_PIN, I2C_GPIO_SDA_FUNC);
    stc_i2c_init_t config;
    float32_t baud_error;
    (void)I2C_StructInit(&config);
    config.u32ClockDiv = I2C_CLK_DIV2;
    config.u32Baudrate = 400000;
    config.u32SclTime = 5;
    int32_t result = I2C_Init(I2C_UNIT, &config, &baud_error);
    if (result == LL_OK) {
        I2C_SlaveAddrConfig(I2C_UNIT, I2C_ADDR0, I2C_ADDR_7BIT, DEVICE_ADDR);
        result = register_irq(I2C_EEI_IRQN_DEF, I2C_INT_EEI_DEF, DDL_IRQ_PRIO_09, I2C_EEI_Callback);
    }
    if (result == LL_OK) {
        result = register_irq(I2C_RXI_IRQN_DEF, I2C_INT_RXI_DEF, DDL_IRQ_PRIO_10, I2C_RXI_Callback);
    }
    if (result == LL_OK) {
        result = register_irq(I2C_TEI_IRQN_DEF, I2C_INT_TEI_DEF, DDL_IRQ_PRIO_10, I2C_TEI_Callback);
    }
    initialized = result == LL_OK;
    if (initialized) {
        rx_head = 0;
        rx_tail = 0;
        slave_state = SLAVE_RX;
        transaction_active = false;
        tx_has_response = false;
        response_reserved = false;
        reset_parser();
        last_progress_ms = millis();
        last_rx_ms = last_progress_ms;
        polled_progress = progress_sequence;
        I2C_Cmd(I2C_UNIT, ENABLE);
        I2C_IntCmd(I2C_UNIT, I2C_INT_MATCH_ADDR0 | I2C_INT_RX_FULL, ENABLE);
    } else {
        deinit_hardware();
        ++slave_i2c_diagnostics.init_failures;
    }
    if ((gpio_protection & 1U) == 0U) {
        GPIO_REG_Lock();
    }
    exit_critical(irq_state);
    return result;
}

void
slave_i2c_update() {
    const uint32_t now = millis();
    if (consumed_reads != completed_reads) {
        consumed_reads = completed_reads;
        message_response_complete(response_buffer, sizeof(response_buffer));
    }
    if (pending_response_length && slave_state != SLAVE_TX
        && txBufferWrite(response_buffer, pending_response_length) > 0) {
        pending_response_length = 0;
    }
    if (!initialized) {
        if (now - last_progress_ms >= 2000U) {
            last_progress_ms = now;
            (void)slave_i2c_init();
        }
        return;
    }
    // 每次只取少量字节，缩短关中断时间；解析和业务操作全部在主循环执行。
    uint8_t bytes[32];
    uint16_t count = 0;
    const uint32_t irq_state = enter_critical();
    if (!transaction_active) {
        while (rx_head != rx_tail && count < sizeof(bytes)) {
            bytes[count++] = rx_buffer[rx_tail];
            rx_tail = (rx_tail + 1U) % SLAVE_RX_BUFFER_SIZE;
        }
    }
    exit_critical(irq_state);
    if (count) {
        last_rx_ms = now;
        for (uint16_t i = 0; i < count; ++i) {
            sempParseNextByte(parser, bytes[i]);
        }
    } else if (parser->state != sempFirstByte && now - last_rx_ms >= 250U) {
        reset_parser();
        ++slave_i2c_diagnostics.parser_timeouts;
    }
    if (polled_progress != progress_sequence) {
        polled_progress = progress_sequence;
        last_progress_ms = now;
    }
    const bool busy = SET == I2C_GetStatus(I2C_UNIT, I2C_FLAG_BUSY);
    if (!transaction_active && !busy) {
        last_progress_ms = now;
    } else if (now - last_progress_ms >= 2000U) {
        slave_i2c_diagnostics.last_recovery_sr = I2C_UNIT->SR;
        ++slave_i2c_diagnostics.recoveries;
        ++State::Diagnostics().i2c_errors;
        const uint32_t state = enter_critical();
        if (tx_has_response) {
            tx_tail = tx_start_tail;
        }
        (void)slave_i2c_init();
        exit_critical(state);
    }
}
