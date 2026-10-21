#include "mp2762a.h"

I2cMaster* mp2762aI2c = nullptr;

bool
mp2762aBegin(I2cMaster* i2cBus) {
    if (i2cBus == nullptr) {
        return false;
    }
    mp2762aI2c = i2cBus;
    return true;
}

bool
mp2762aReadRegister8Checked(uint8_t reg, uint8_t* value) {
    uint8_t byte = 0;
    if (!value || !mp2762aI2c || !mp2762aI2c->writeRead(MP2762A_DEVICE_ADDRESS, reg, &byte, 1)) {
        return false;
    }
    *value = byte;
    return true;
}

uint8_t
mp2762aReadRegister8(uint8_t reg) {
    uint8_t value = UINT8_MAX;
    (void)mp2762aReadRegister8Checked(reg, &value);
    return value;
}

uint8_t
mp2762aWriteRegister8(uint8_t reg, uint8_t data) {
    const uint8_t bytes[2] = {reg, data};
    return mp2762aI2c && mp2762aI2c->write(MP2762A_DEVICE_ADDRESS, bytes, 2);
}

// Set the Precharge threshold
// 5.8V, 6.0, 6.2, 6.4, 6.6, 6.8, 7.4, 7.2 (oddly out of order)
void
mp2762setFastChargeVoltageMv(uint16_t mVoltLevel) {
    // Default to 6.8V (requires option '2')
    uint8_t option = 1;         // This is option 2 confusingly
    uint8_t newVbattPre = 0x01; // Default to 6.8V

    if (mVoltLevel <= 5800) {
        option = 0;
        newVbattPre = 0x00; // 5.8V
    } else if (mVoltLevel <= 6000) {
        option = 0;
        newVbattPre = 0x01; // 6.0V
    } else if (mVoltLevel <= 6200) {
        option = 0;
        newVbattPre = 0x02; // 6.2V
    } else if (mVoltLevel <= 6400) {
        option = 0;
        newVbattPre = 0x03; // 6.4V
    } else if (mVoltLevel <= 6600) {
        option = 1;
        newVbattPre = 0x00; // 6.6V
    } else if (mVoltLevel <= 6800) {
        option = 1;
        newVbattPre = 0x01; // 6.8V
    } else if (mVoltLevel <= 7200) {
        option = 1;
        newVbattPre = 0x03; // 7.2V
    } else if (mVoltLevel <= 7400) {
        option = 1;
        newVbattPre = 0x02; // 7.4V
    }

    // Set the Precharge bits
    uint8_t status = 0;
    if (!mp2762aReadRegister8Checked(MP2762A_PRECHARGE_THRESHOLD, &status)) {
        return;
    }
    status &= ~(0x03 << 4); // Clear bits 4 and 5
    newVbattPre <<= 4;      // Shift to correct position
    status |= newVbattPre;  // Set bits accordingly
    mp2762aWriteRegister8(MP2762A_PRECHARGE_THRESHOLD, status);

    // Set the option bit
    if (!mp2762aReadRegister8Checked(MP2762A_PRECHARGE_THRESHOLD_OPTION, &status)) {
        return;
    }
    status &= ~(1 << 3); // Clear bit 3
    option <<= 3;        // Shift to correct position
    status |= option;    // Set bit accordingly
    mp2762aWriteRegister8(MP2762A_PRECHARGE_THRESHOLD_OPTION, status);
}

uint8_t
mp2762setFastChargeCurrentMa(uint16_t currentLevelMa) {
    // defualt to 1A
    uint8_t newIFast = 0x01;
    if (currentLevelMa <= 50) {
        newIFast = 0x01;
    } else {
        uint16_t steps = currentLevelMa / 50;
        newIFast = 0x01 * steps;
    }

    mp2762aWriteRegister8(MP2762A_SETCHARGE_CURRENT, newIFast);
    return mp2762aReadRegister8(MP2762A_SETCHARGE_CURRENT);
}

void
mp2762setPrechargeCurrentMa(uint16_t currentLevelMa) {
    uint16_t steps = currentLevelMa <= 180U ? 3U : currentLevelMa <= 240U ? 5U : 5U + (currentLevelMa - 240U) / 60U;
    const uint8_t newIPre = steps > 15U ? 15U : static_cast<uint8_t>(steps);

    // Set the Precharge current bits
    uint8_t status = 0;
    if (!mp2762aReadRegister8Checked(MP2762A_PRECHARGE_CURRENT, &status)) {
        return;
    }
    status &= ~(0x0F << 4); // Clear bits 7, 6, 5, 4
    status |= static_cast<uint8_t>(newIPre << 4);
    mp2762aWriteRegister8(MP2762A_PRECHARGE_CURRENT, status);
}

uint8_t
mp2762getChargeStatus() {
    uint8_t status = 0;
    status = mp2762aReadRegister8(MP2762A_STATUS);
    status >>= 2;
    status &= 0x03;
    return (status);
}

bool
mp2762setChargerEnabled(bool enabled, uint8_t* config) {
    uint8_t status;
    if (!config || !mp2762aReadRegister8Checked(MP2762A_CONFIG_0, &status)) {
        return false;
    }
    status = enabled ? status | (1U << 4) : status & ~(1U << 4);
    if (!mp2762aWriteRegister8(MP2762A_CONFIG_0, status) || !mp2762aReadRegister8Checked(MP2762A_CONFIG_0, config)) {
        return false;
    }
    return ((*config & (1U << 4)) != 0U) == enabled;
}

void
mp2762registerReset() {
    uint8_t status = 0;
    if (!mp2762aReadRegister8Checked(MP2762A_CONFIG_0, &status)) {
        return;
    }
    status |= 1 << 7; // Set REG_RST
    mp2762aWriteRegister8(MP2762A_CONFIG_0, status);
}