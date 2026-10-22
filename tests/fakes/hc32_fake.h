#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

using IRQn_Type = int;
using en_int_src_t = int;
using func_ptr_t = void (*)();
using en_flag_status_t = int;

struct CM_I2C_TypeDef {
    uint32_t SR = 0;
    uint32_t DR = 0;
};

struct FakeGpio {
    uint16_t PWPR = 0;
};

extern CM_I2C_TypeDef fake_i2c1, fake_i2c2;
extern FakeGpio fake_gpio;

#define CM_I2C1                     (&fake_i2c1)
#define CM_I2C2                     (&fake_i2c2)
#define CM_GPIO                     (&fake_gpio)
#define LL_OK                       0
#define LL_ERR                      (-1)
#define ENABLE                      1
#define DISABLE                     0
#define SET                         1
#define RESET                       0
#define FCG1_PERIPH_I2C1            1
#define PWC_FCG1_I2C2               2
#define GPIO_PORT_A                 0
#define GPIO_PIN_03                 3
#define GPIO_PIN_02                 2
#define GPIO_PIN_08                 (1U << 8)
#define GPIO_PIN_09                 (1U << 9)
#define GPIO_FUNC_49                49
#define GPIO_FUNC_48                48
#define GPIO_FUNC_51                51
#define GPIO_FUNC_50                50
#define INT005_IRQn                 5
#define INT006_IRQn                 6
#define INT004_IRQn                 4
#define INT_SRC_I2C1_EEI            5
#define INT_SRC_I2C1_RXI            6
#define INT_SRC_I2C1_TEI            4
#define DDL_IRQ_PRIO_09             9
#define DDL_IRQ_PRIO_10             10
#define I2C_CLK_DIV2                2
#define I2C_CLK_DIV8                8
#define I2C_ADDR0                   0
#define I2C_ADDR_7BIT               7
#define I2C_DIR_TX                  0
#define I2C_DIR_RX                  1
#define I2C_ACK                     0
#define I2C_NACK                    1
#define I2C_FLAG_MATCH_ADDR0        (1U << 0)
#define I2C_FLAG_TRA                (1U << 1)
#define I2C_FLAG_NACKF              (1U << 2)
#define I2C_FLAG_STOP               (1U << 3)
#define I2C_FLAG_ARBITRATE_FAIL     (1U << 4)
#define I2C_FLAG_TX_CPLT            (1U << 5)
#define I2C_FLAG_BUSY               (1U << 6)
#define I2C_FLAG_START              (1U << 7)
#define I2C_CLR_STOPFCLR            I2C_FLAG_STOP
#define I2C_CLR_NACKFCLR            I2C_FLAG_NACKF
#define I2C_CLR_SLADDR0FCLR         I2C_FLAG_MATCH_ADDR0
#define I2C_CLR_STARTFCLR           I2C_FLAG_START
#define I2C_FLAG_CLR_ARBITRATE_FAIL I2C_FLAG_ARBITRATE_FAIL
#define I2C_FLAG_CLR_TX_CPLT        I2C_FLAG_TX_CPLT
#define I2C_INT_TX_CPLT             I2C_FLAG_TX_CPLT
#define I2C_INT_STOP                I2C_FLAG_STOP
#define I2C_INT_NACK                I2C_FLAG_NACKF
#define I2C_INT_ARBITRATE_FAIL      I2C_FLAG_ARBITRATE_FAIL
#define I2C_INT_MATCH_ADDR0         I2C_FLAG_MATCH_ADDR0
#define I2C_INT_RX_FULL             (1U << 8)

struct stc_i2c_init_t {
    uint32_t u32ClockDiv, u32Baudrate, u32SclTime;
};

using float32_t = float;

struct stc_irq_signin_config_t {
    IRQn_Type enIRQn;
    en_int_src_t enIntSrc;
    func_ptr_t pfnCallback;
};

extern func_ptr_t fake_irqs[16];
extern uint32_t fake_ms, fake_primask, fake_slave_initializations, fake_master_resets;
extern int fake_io_error, fake_receive_error, fake_slave_init_error;
extern int fake_master_init_error;
extern char fake_master_failure;
extern std::string fake_master_trace;
extern stc_i2c_init_t fake_master_config;
extern uint8_t fake_address, fake_register;
extern uint16_t fake_registers[128][256];
extern uint32_t fake_master_writes;
extern int fake_pins[32];
inline uint32_t fake_fcg1_clock;
inline int fake_mux_pin, fake_mux_function;
inline CM_I2C_TypeDef* fake_master_regs;

inline uint32_t
__get_PRIMASK() {
    return fake_primask;
}

inline void
__disable_irq() {
    fake_primask = 1;
}

inline void
__set_PRIMASK(uint32_t state) {
    fake_primask = state;
}

inline void
__WFI() {}

extern uint32_t fake_system_resets;

inline void
__DSB() {}

inline void
NVIC_SystemReset() {
    ++fake_system_resets;
}

inline uint32_t
millis() {
    return fake_ms;
}

inline void
GPIO_REG_Unlock() {
    fake_gpio.PWPR = 1;
}

inline void
GPIO_REG_Lock() {
    fake_gpio.PWPR = 0;
}

inline void
GPIO_SetFunc(int port, int pin, int function) {
    if (pin == GPIO_PIN_09 || pin == GPIO_PIN_08) {
        assert(port == GPIO_PORT_A);
        assert(function == (pin == GPIO_PIN_09 ? GPIO_FUNC_51 : GPIO_FUNC_50));
    }
}

/** 核对主板 I2C 复用配置；其他引脚由独立配置回归覆盖。 */
inline void
GPIO_SetFunction(int pin, int function) {
    fake_mux_pin = pin;
    fake_mux_function = function;
    if (pin == 9 || pin == 8) {
        assert(function == (pin == 9 ? GPIO_FUNC_51 : GPIO_FUNC_50));
    }
}

/** 记录使能的外设时钟，验证驱动使用调用方配置。 */
inline void
FCG_Fcg1PeriphClockCmd(int clock, int) {
    fake_fcg1_clock = clock;
}

inline void
NVIC_DisableIRQ(int) {}

inline void
NVIC_ClearPendingIRQ(int) {}

inline void
NVIC_SetPriority(int, uint32_t) {}

inline void
NVIC_EnableIRQ(int) {}

inline int32_t
INTC_IrqSignOut(int irq) {
    fake_irqs[irq] = nullptr;
    return LL_OK;
}

inline int32_t
INTC_IrqSignIn(const stc_irq_signin_config_t* config) {
    fake_irqs[config->enIRQn] = config->pfnCallback;
    return LL_OK;
}

inline int32_t
I2C_DeInit(CM_I2C_TypeDef* unit) {
    fake_master_regs = unit;
    unit->SR = 0;
    return LL_OK;
}

inline int32_t
I2C_StructInit(stc_i2c_init_t* config) {
    *config = {};
    return LL_OK;
}

inline int32_t
I2C_Init(CM_I2C_TypeDef* unit, const stc_i2c_init_t* config, float*) {
    if (unit == CM_I2C1) {
        ++fake_slave_initializations;
        return fake_slave_init_error;
    }
    fake_master_config = *config;
    return fake_master_init_error;
}

inline void
I2C_SlaveAddrConfig(CM_I2C_TypeDef*, int, int, int) {}

inline void
I2C_IntCmd(CM_I2C_TypeDef*, uint32_t, int) {}

inline void
I2C_Cmd(CM_I2C_TypeDef* unit, int enabled) {
    if (unit == CM_I2C2) {
        fake_master_trace += enabled ? 'E' : 'e';
    }
}

inline void
I2C_BusWaitCmd(CM_I2C_TypeDef* unit, int enabled) {
    if (unit == CM_I2C2) {
        assert(enabled == ENABLE);
        fake_master_trace += 'B';
    }
}

inline void
I2C_AckConfig(CM_I2C_TypeDef* unit, int ack) {
    if (unit == CM_I2C2) {
        fake_master_trace += ack == I2C_ACK ? 'A' : 'N';
    }
}

inline void
I2C_SWResetCmd(CM_I2C_TypeDef* unit, int enabled) {
    if (unit == CM_I2C2) {
        fake_master_trace += enabled ? 'R' : 'r';
    }
    if (enabled) {
        unit->SR = 0;
        ++fake_master_resets;
    }
}

inline int
I2C_GetStatus(CM_I2C_TypeDef* unit, uint32_t flag) {
    return (unit->SR & flag) != 0;
}

inline void
I2C_ClearStatus(CM_I2C_TypeDef* unit, uint32_t flags) {
    unit->SR &= ~flags;
    if (flags & I2C_CLR_STARTFCLR) {
        unit->SR &= ~I2C_FLAG_BUSY;
    }
}

inline void
I2C_WriteData(CM_I2C_TypeDef* unit, uint8_t data) {
    unit->DR = data;
}

inline uint8_t
I2C_ReadData(CM_I2C_TypeDef* unit) {
    return static_cast<uint8_t>(unit->DR);
}

inline int32_t
fake_master_step(char step, uint32_t timeout) {
    assert(timeout == 0x40000UL);
    fake_master_trace += step;
    return fake_master_failure == step ? LL_ERR : fake_io_error;
}

/** 记录事务接收到的寄存器实例，验证无固定外设依赖。 */
inline int32_t
I2C_Start(CM_I2C_TypeDef* unit, uint32_t timeout) {
    fake_master_regs = unit;
    return fake_master_step('S', timeout);
}

inline int32_t
I2C_Stop(CM_I2C_TypeDef*, uint32_t timeout) {
    return fake_master_step('P', timeout);
}

inline int32_t
I2C_Restart(CM_I2C_TypeDef*, uint32_t timeout) {
    return fake_master_step('T', timeout);
}

inline int32_t
I2C_TransAddr(CM_I2C_TypeDef*, uint8_t address, int direction, uint32_t timeout) {
    fake_address = address;
    return fake_master_step(direction == I2C_DIR_TX ? 'W' : 'V', timeout);
}

inline int32_t
I2C_TransData(CM_I2C_TypeDef*, const uint8_t* data, size_t length, uint32_t timeout) {
    const int32_t result = fake_master_step('D', timeout);
    if (result != LL_OK) {
        return result;
    }
    fake_register = data[0];
    if (length == 2U) {
        fake_registers[fake_address][fake_register] = data[1];
        ++fake_master_writes;
    }
    return LL_OK;
}

inline int32_t
I2C_MasterReceiveDataAndStop(CM_I2C_TypeDef*, uint8_t* data, uint32_t length, uint32_t timeout) {
    const int32_t result = fake_master_step('G', timeout);
    if (result != LL_OK) {
        return result;
    }
    const uint16_t value = fake_registers[fake_address][fake_register];
    data[0] = value & 255U;
    if (fake_receive_error) {
        return fake_receive_error;
    }
    if (length == 2U) {
        data[1] = value >> 8;
    }
    return LL_OK;
}

#define HIGH                                    1
#define LOW                                     0
#define POWER_CONTROL_PIN                       0
#define WATCHDOG_FEED_PIN                       1
#define POWER_LED_PIN                           2
#define FUNCTION_LED_PIN                        3
#define CHARGE_LED_PIN                          4
#define POWER_KEY_PIN                           5
#define CHARGER_CTRL_PIN                        6
#define CHARGER_ENABLE_PIN                      7
#define CHARGER_SWITCH_PIN                      8
#define CHARGER_CTRL_FAST_PIN                   9
#define CHARGER_ADC_DETECT_PIN                  10
#define USB_SWITCH_PIN                          11
#define WATCHDOG_FEED_TIME                      5000
#define BATTERY_TERMINATE_TEMP                  45.0f
#define RMU_FLAG_SW                             1
#define CM_VALUE_IN_RANGE(value, low, high)     ((value) >= (low) && (value) <= (high))
#define CM_SET_VALUE_IN_RANGE(value, low, high) ((value) < (low) ? (low) : (value) > (high) ? (high) : (value))

inline int fake_software_reset = RESET;

inline int
RMU_GetStatus(int) {
    return fake_software_reset;
}

/** Consume the seeded reset reason after Power_Init reads it. */
inline void
RMU_ClearStatus() {
    fake_software_reset = RESET;
}
