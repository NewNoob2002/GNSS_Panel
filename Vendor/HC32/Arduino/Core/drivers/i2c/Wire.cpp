#include "delay.h"
#include "hc32_ll_def.h"
#include "hc32_ll_fcg.h"
#include "hc32_ll_i2c.h"
#include <core_log.h>
#include <Wire.h>

uint8_t rxBuffer[WIRE_BUFF_SIZE];
uint8_t txBuffer[WIRE_BUFF_SIZE];

i2c_peripheral_config_t I2C1_config = {
    .register_base    = CM_I2C1,
    .clock_id         = PWC_FCG1_I2C1,
    .scl_pin_function = Func_I2c1_Scl,
    .sda_pin_function = Func_I2c1_Sda,
};

i2c_peripheral_config_t *I2Cx[1] = {
    &I2C1_config,
};

TwoWire::TwoWire(struct i2c_peripheral_config_t *config, gpio_pin_t scl_pin, gpio_pin_t sda_pin)
{
    this->_config  = config;
    this->_scl_pin = scl_pin;
    this->_sda_pin = sda_pin;

    // this->_rxBuffer = nullptr;
    // this->_txBuffer = nullptr;

    this->_max_trytimes = 3;
}

void TwoWire::begin()
{
    // this->_rxBuffer = new RingBuffer<uint8_t>(rxBuffer, WIRE_BUFF_SIZE);
    // this->_txBuffer = new RingBuffer<uint8_t>(txBuffer, WIRE_BUFF_SIZE);

    // if (this->_rxBuffer == nullptr || this->_txBuffer == nullptr) {
    //     LOG_ERROR("Failed to allocate memory for rxBuffer or txBuffer");
    // }

    GPIO_SetFunction(this->_scl_pin, this->_config->scl_pin_function);
    GPIO_SetFunction(this->_sda_pin, this->_config->sda_pin_function);

    FCG_Fcg1PeriphClockCmd(this->_config->clock_id, ENABLE);

    int32_t i32Ret;
    stc_i2c_init_t stcI2cInit;
    float32_t fErr;

    (void)I2C_DeInit(this->_config->register_base);

    (void)I2C_StructInit(&stcI2cInit);
    stcI2cInit.u32ClockDiv = I2C_CLK_DIV2;
    stcI2cInit.u32Baudrate = this->_clock_frequency;
    stcI2cInit.u32SclTime  = 3UL;
    i32Ret                 = I2C_Init(this->_config->register_base, &stcI2cInit, &fErr);

    if (i32Ret != LL_OK) {
        LOG_ERROR("Failed to initialize I2C");
    }

    I2C_BusWaitCmd(this->_config->register_base, ENABLE);
}

void TwoWire::end()
{
    I2C_DeInit(this->_config->register_base);
}

void TwoWire::setClock(uint32_t clockFreq)
{
    this->_clock_frequency = clockFreq;

    int32_t i32Ret;
    stc_i2c_init_t stcI2cInit;
    float32_t fErr;

    (void)I2C_DeInit(this->_config->register_base);
}

bool TwoWire::beginTransmission(uint8_t address)
{
    uint32_t i32Ret  = LL_ERR;
    uint8_t trytimes = 0;
    while (trytimes < this->_max_trytimes) {
        I2C_Cmd(this->_config->register_base, ENABLE);

        if (I2C_Start(this->_config->register_base, 1000) == LL_OK) {
#if (I2C_ADDR_MD == I2C_ADDR_MD_10BIT)
            i32Ret = I2C_Trans10BitAddr(this->_config->register_base, address, I2C_DIR_TX, 1000);
#else
            i32Ret = I2C_TransAddr(this->_config->register_base, address, I2C_DIR_TX, 1000);
#endif
        }
        if (i32Ret == LL_OK) {
            break;
        }
        if (i32Ret != LL_OK && trytimes + 1 < this->_max_trytimes) {
            delay_ms(5);
            continue;
        }
    }

    return i32Ret == LL_OK;
}

uint8_t TwoWire::endTransmission(bool stopBit)
{
    if (stopBit) {
        // Stop by software
        I2C_Stop(this->_config->register_base, 1000);
        // Disable I2C
        I2C_Cmd(this->_config->register_base, DISABLE);
        return 1;
    }

    return 0;
}

size_t TwoWire::write(uint8_t data)
{
    if (I2C_TransData(this->_config->register_base, &data, 1, 1000) == LL_OK) {
        return 1;
    }
    return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    if (I2C_TransData(this->_config->register_base, data, quantity, 1000) == LL_OK) {
        return quantity;
    }
    return 0;
}
