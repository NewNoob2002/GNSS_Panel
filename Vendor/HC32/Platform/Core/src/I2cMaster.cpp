#include "I2cMaster.h"
#include "gpio.h"

/* 时序依据：原 Platform/Core/src/Wire.cpp，保留已验证的 LL 调用顺序与超时。 */
#define I2C_MASTER_TIMEOUT 0x40000UL

bool
I2cMaster::begin(uint32_t clockHz) {
    bus->initialized = false;
    if (clock_hz == 0U || clock_hz > 400000U) {
        return false;
    }
    GPIO_SetFunction(config.scl, config.sclFunc);
    GPIO_SetFunction(config.sda, config.sdaFunc);
    FCG_Fcg1PeriphClockCmd(config.fcg1Clock, ENABLE);
    (void)I2C_DeInit(bus->register_base);
    stc_i2c_init_t config;
    float32_t error;
    (void)I2C_StructInit(&config);
    config.u32ClockDiv = clock_hz <= 100000U ? I2C_CLK_DIV8 : I2C_CLK_DIV2;
    config.u32Baudrate = clock_hz;
    config.u32SclTime = clock_hz <= 100000U ? 3U : 5U;
    if (I2C_Init(bus->register_base, &config, &error) != LL_OK) {
        return false;
    }
    I2C_BusWaitCmd(bus->register_base, ENABLE);
    bus->initialized = true;
    return true;
}

void
I2cMaster::end() {
    (void)I2C_DeInit(bus->register_base);
    bus->initialized = false;
}

bool
I2cMaster::finish() {
    const int32_t stop_result = I2C_Stop(bus->register_base, I2C_MASTER_TIMEOUT);
    if (bus->transaction_status == LL_OK) {
        bus->transaction_status = stop_result;
    }
    I2C_AckConfig(bus->register_base, I2C_ACK);
    if (bus->transaction_status != LL_OK) {
        I2C_SWResetCmd(bus->register_base, ENABLE);
        I2C_SWResetCmd(bus->register_base, DISABLE);
    }
    I2C_Cmd(bus->register_base, DISABLE);
    return bus->transaction_status == LL_OK;
}

bool
I2cMaster::start(uint8_t addr) {
    if (!bus || !bus->initialized || addr > 0x7fU) {
        return false;
    }
    I2C_Cmd(bus->register_base, ENABLE);
    I2C_SWResetCmd(bus->register_base, ENABLE);
    I2C_SWResetCmd(bus->register_base, DISABLE);
    I2C_AckConfig(bus->register_base, I2C_ACK);
    bus->transaction_status = I2C_Start(bus->register_base, I2C_MASTER_TIMEOUT);
    if (bus->transaction_status == LL_OK) {
        bus->transaction_status = I2C_TransAddr(bus->register_base, addr, I2C_DIR_TX, I2C_MASTER_TIMEOUT);
    }
    if (bus->transaction_status != LL_OK) {
        (void)i2c_master_finish(bus);
        return false;
    }
    return true;
}

bool
I2cMaster::send(const uint8_t* data, size_t len) {
    bus->transaction_status = I2C_TransData(bus->register_base, data, len, I2C_MASTER_TIMEOUT);
    if (bus->transaction_status != LL_OK) {
        (void)i2c_master_finish(bus);
        return false;
    }
    return true;
}

bool
I2cMaster::write(uint8_t addr, const uint8_t* data, size_t len) {
    if (!data || !len || len > UINT32_MAX || !i2c_master_start(bus, addr)) {
        return false;
    }
    const bool written = i2c_master_send(bus, data, len);
    /* 原 MP2762A 调用在 write 失败自恢复后仍执行 endTransmission，保留第二次 STOP。 */
    const bool stopped = i2c_master_finish(bus);
    return written && stopped;
}

bool
I2cMaster::writeRead(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
    if (!buf || !len || len > UINT32_MAX || !i2c_master_start(bus, addr)) {
        return false;
    }
    if (!i2c_master_send(bus, &reg, 1)) {
        bus->transaction_status = LL_ERR;
        (void)i2c_master_finish(bus);
        return false;
    }
    bus->transaction_status = I2C_Restart(bus->register_base, I2C_MASTER_TIMEOUT);
    if (bus->transaction_status == LL_OK) {
        if (len == 1U) {
            I2C_AckConfig(bus->register_base, I2C_NACK);
        }
        bus->transaction_status = I2C_TransAddr(bus->register_base, addr, I2C_DIR_RX, I2C_MASTER_TIMEOUT);
    }
    if (bus->transaction_status == LL_OK) {
        bus->transaction_status = I2C_MasterReceiveDataAndStop(bus->register_base, buf, len, I2C_MASTER_TIMEOUT);
    }
    I2C_AckConfig(bus->register_base, I2C_ACK);
    if (bus->transaction_status != LL_OK) {
        (void)i2c_master_finish(bus);
        return false;
    }
    I2C_Cmd(bus->register_base, DISABLE);
    return true;
}

bool
I2cMaster::probe(uint8_t addr) {
    return i2c_master_start(bus, addr) && i2c_master_finish(bus);
}

int
I2cMaster::scan(ScanCallback callback) {
    int count = 0;
    for (uint8_t address = 1; address < 0x7f; ++address) {
        const bool online = i2c_master_probe(bus, address);
        if (online) {
            ++count;
        }
        if (callback) {
            callback(online ? &address : nullptr);
        }
    }
    return count;
}
