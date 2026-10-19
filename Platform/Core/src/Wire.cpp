#include "Wire.h"

i2c_peripheral_config_t I2C2_config = {
    .register_base = CM_I2C2,
    .clock_id = PWC_FCG1_I2C2,
    .scl_pin_function = GPIO_FUNC_51,
    .sda_pin_function = GPIO_FUNC_50,
};
TwoWire Wire(&I2C2_config, PA9, PA8);

TwoWire::TwoWire(i2c_peripheral_config_t* config, gpio_pin_t scl_pin, gpio_pin_t sda_pin)
    : _config(config), _scl_pin(scl_pin), _sda_pin(sda_pin) {}

int
TwoWire::begin(uint32_t clockFreq) {
    isInitliased = false;
    if (!_config || clockFreq == 0U || clockFreq > 400000U) {
        return 0;
    }
    _clock_frequency = clockFreq;
    GPIO_SetFunction(_scl_pin, _config->scl_pin_function);
    GPIO_SetFunction(_sda_pin, _config->sda_pin_function);
    FCG_Fcg1PeriphClockCmd(_config->clock_id, ENABLE);
    (void)I2C_DeInit(_config->register_base);
    stc_i2c_init_t config;
    float32_t error;
    (void)I2C_StructInit(&config);
    config.u32ClockDiv = clockFreq <= 100000U ? I2C_CLK_DIV8 : I2C_CLK_DIV2;
    config.u32Baudrate = clockFreq;
    config.u32SclTime = clockFreq <= 100000U ? 3U : 5U;
    if (I2C_Init(_config->register_base, &config, &error) != LL_OK) {
        return 0;
    }
    I2C_BusWaitCmd(_config->register_base, ENABLE);
    isInitliased = true;
    return 1;
}

void
TwoWire::end() {
    (void)I2C_DeInit(_config->register_base);
    isInitliased = false;
}

bool
TwoWire::beginTransmission(uint8_t address) {
    if (!isInitliased || address > 0x7fU) {
        return false;
    }
    I2C_Cmd(_config->register_base, ENABLE);
    I2C_SWResetCmd(_config->register_base, ENABLE);
    I2C_SWResetCmd(_config->register_base, DISABLE);
    I2C_AckConfig(_config->register_base, I2C_ACK);
    _transaction_status = I2C_Start(_config->register_base, WIRE_TIMEOUT);
    if (_transaction_status == LL_OK) {
        _transaction_status = I2C_TransAddr(_config->register_base, address, I2C_DIR_TX, WIRE_TIMEOUT);
    }
    if (_transaction_status != LL_OK) {
        (void)endTransmission();
        return false;
    }
    return true;
}

uint8_t
TwoWire::endTransmission(bool stopBit) {
    if (!isInitliased) {
        return 4;
    }
    if (stopBit) {
        const int32_t stop_result = I2C_Stop(_config->register_base, WIRE_TIMEOUT);
        if (_transaction_status == LL_OK) {
            _transaction_status = stop_result;
        }
        I2C_AckConfig(_config->register_base, I2C_ACK);
        if (_transaction_status != LL_OK) {
            I2C_SWResetCmd(_config->register_base, ENABLE);
            I2C_SWResetCmd(_config->register_base, DISABLE);
        }
        I2C_Cmd(_config->register_base, DISABLE);
    }
    return _transaction_status == LL_OK ? 0 : 4;
}

size_t
TwoWire::write(uint8_t data) {
    return write(&data, 1);
}

size_t
TwoWire::write(const uint8_t* data, size_t quantity) {
    if (!data || !quantity || !isInitliased || _transaction_status != LL_OK) {
        _transaction_status = LL_ERR;
        return 0;
    }
    _transaction_status = I2C_TransData(_config->register_base, data, quantity, WIRE_TIMEOUT);
    if (_transaction_status != LL_OK) {
        (void)endTransmission();
        return 0;
    }
    return quantity;
}

size_t
TwoWire::requestFrom(uint8_t address, uint8_t register_address, uint8_t* buffer, uint8_t quantity, bool sendStop) {
    if (!buffer || !quantity || !sendStop || address > 0x7fU || write(register_address) != 1U) {
        _transaction_status = LL_ERR;
        (void)endTransmission();
        return 0;
    }
    _transaction_status = I2C_Restart(_config->register_base, WIRE_TIMEOUT);
    if (_transaction_status == LL_OK) {
        if (quantity == 1U) {
            I2C_AckConfig(_config->register_base, I2C_NACK);
        }
        _transaction_status = I2C_TransAddr(_config->register_base, address, I2C_DIR_RX, WIRE_TIMEOUT);
    }
    if (_transaction_status == LL_OK) {
        _transaction_status = I2C_MasterReceiveDataAndStop(_config->register_base, buffer, quantity, WIRE_TIMEOUT);
    }
    I2C_AckConfig(_config->register_base, I2C_ACK);
    if (_transaction_status != LL_OK) {
        (void)endTransmission();
        return 0;
    }
    I2C_Cmd(_config->register_base, DISABLE);
    return quantity;
}

bool
TwoWire::isDeviceOnline(uint8_t address) {
    return beginTransmission(address) && endTransmission() == 0;
}

int
TwoWire::scanDeivces(voidFuncPtrWithArg callback) {
    int count = 0;
    for (uint8_t address = 1; address < 0x7f; ++address) {
        const bool online = isDeviceOnline(address);
        if (online) {
            ++count;
        }
        if (callback) {
            callback(online ? &address : nullptr);
        }
    }
    return count;
}
