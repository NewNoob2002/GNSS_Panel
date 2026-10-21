#include "I2cMaster.h"
#include <gpio.h>

/* 时序依据：原 Wire.cpp，保留已验证的 LL 调用顺序与超时。 */
#define I2C_MASTER_TIMEOUT 0x40000UL

bool
I2cMaster::begin(uint32_t clockHz) {
    initialized = false;
    if (clockHz == 0U || clockHz > 400000U) {
        return false;
    }
    GPIO_SetFunction(config.scl, config.sclFunc);
    GPIO_SetFunction(config.sda, config.sdaFunc);
    FCG_Fcg1PeriphClockCmd(config.fcg1Clock, ENABLE);
    (void)I2C_DeInit(config.regs);
    stc_i2c_init_t initConfig;
    float32_t error;
    (void)I2C_StructInit(&initConfig);
    initConfig.u32ClockDiv = clockHz <= 100000U ? I2C_CLK_DIV8 : I2C_CLK_DIV2;
    initConfig.u32Baudrate = clockHz;
    initConfig.u32SclTime = clockHz <= 100000U ? 3U : 5U;
    if (I2C_Init(config.regs, &initConfig, &error) != LL_OK) {
        return false;
    }
    I2C_BusWaitCmd(config.regs, ENABLE);
    initialized = true;
    return true;
}

void
I2cMaster::end() {
    (void)I2C_DeInit(config.regs);
    initialized = false;
}

bool
I2cMaster::finish() {
    const int32_t stopResult = I2C_Stop(config.regs, I2C_MASTER_TIMEOUT);
    if (transactionStatus == LL_OK) {
        transactionStatus = stopResult;
    }
    I2C_AckConfig(config.regs, I2C_ACK);
    if (transactionStatus != LL_OK) {
        I2C_SWResetCmd(config.regs, ENABLE);
        I2C_SWResetCmd(config.regs, DISABLE);
    }
    I2C_Cmd(config.regs, DISABLE);
    return transactionStatus == LL_OK;
}

bool
I2cMaster::start(uint8_t addr) {
    if (!initialized || addr > 0x7fU) {
        return false;
    }
    I2C_Cmd(config.regs, ENABLE);
    I2C_SWResetCmd(config.regs, ENABLE);
    I2C_SWResetCmd(config.regs, DISABLE);
    I2C_AckConfig(config.regs, I2C_ACK);
    transactionStatus = I2C_Start(config.regs, I2C_MASTER_TIMEOUT);
    if (transactionStatus == LL_OK) {
        transactionStatus = I2C_TransAddr(config.regs, addr, I2C_DIR_TX, I2C_MASTER_TIMEOUT);
    }
    if (transactionStatus != LL_OK) {
        (void)finish();
        return false;
    }
    return true;
}

bool
I2cMaster::send(const uint8_t* data, size_t len) {
    transactionStatus = I2C_TransData(config.regs, data, len, I2C_MASTER_TIMEOUT);
    if (transactionStatus != LL_OK) {
        (void)finish();
        return false;
    }
    return true;
}

bool
I2cMaster::write(uint8_t addr, const uint8_t* data, size_t len) {
    if (!data || !len || len > UINT32_MAX || !start(addr)) {
        return false;
    }
    const bool written = send(data, len);
    /* 原 MP2762A 调用在 write 失败自恢复后仍执行 endTransmission，保留第二次 STOP。 */
    const bool stopped = finish();
    return written && stopped;
}

bool
I2cMaster::writeRead(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len) {
    if (!buf || !len || len > UINT32_MAX || !start(addr)) {
        return false;
    }
    if (!send(&reg, 1)) {
        transactionStatus = LL_ERR;
        (void)finish();
        return false;
    }
    transactionStatus = I2C_Restart(config.regs, I2C_MASTER_TIMEOUT);
    if (transactionStatus == LL_OK) {
        if (len == 1U) {
            I2C_AckConfig(config.regs, I2C_NACK);
        }
        transactionStatus = I2C_TransAddr(config.regs, addr, I2C_DIR_RX, I2C_MASTER_TIMEOUT);
    }
    if (transactionStatus == LL_OK) {
        transactionStatus = I2C_MasterReceiveDataAndStop(config.regs, buf, len, I2C_MASTER_TIMEOUT);
    }
    I2C_AckConfig(config.regs, I2C_ACK);
    if (transactionStatus != LL_OK) {
        (void)finish();
        return false;
    }
    I2C_Cmd(config.regs, DISABLE);
    return true;
}

bool
I2cMaster::probe(uint8_t addr) {
    return start(addr) && finish();
}

int
I2cMaster::scan(ScanCallback callback) {
    int count = 0;
    for (uint8_t address = 1; address < 0x7f; ++address) {
        const bool online = probe(address);
        if (online) {
            ++count;
        }
        if (callback) {
            callback(online ? &address : nullptr);
        }
    }
    return count;
}
