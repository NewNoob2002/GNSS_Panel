#include "mp2762a.h"

i2c_master_t* mp2762aI2c = nullptr;

bool
mp2762aBegin(i2c_master_t* i2cBus) {
    if (i2cBus == nullptr) {
        return false;
    }
    mp2762aI2c = i2cBus;
    return true;
}

bool
mp2762aReadRegister8Checked(uint8_t reg, uint8_t* value) {
    uint8_t byte = 0;
    if (!value || !mp2762aI2c || !i2c_master_write_read(mp2762aI2c, MP2762A_DEVICE_ADDRESS, reg, &byte, 1)) {
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

bool
mp2762aReadRegister16Checked(uint8_t reg, uint16_t* value) {
    uint8_t bytes[2] = {};
    if (!value || !mp2762aI2c || !i2c_master_write_read(mp2762aI2c, MP2762A_DEVICE_ADDRESS, reg, bytes, 2)) {
        return false;
    }
    *value = static_cast<uint16_t>((static_cast<uint16_t>(bytes[1]) << 8) | bytes[0]);
    return true;
}

uint16_t
mp2762aReadRegister16(uint8_t reg) {
    uint16_t value = UINT16_MAX;
    (void)mp2762aReadRegister16Checked(reg, &value);
    return value;
}

uint8_t
mp2762aWriteRegister8(uint8_t reg, uint8_t data) {
    const uint8_t bytes[2] = {reg, data};
    return i2c_master_write(mp2762aI2c, MP2762A_DEVICE_ADDRESS, bytes, 2);
}

// Given a bit field, and a startingBitValue
// Example: Battery voltage is bit 12.5mV per bit
float
convertBitsToDoubler(uint16_t bitField, float startingBitValue) {
    float totalMv = 0;
    for (int x = 0; x < 16; x++) {
        if (bitField & 0x0001) {
            totalMv += startingBitValue;
        }

        bitField >>= 1;

        startingBitValue *= 2;
    }
    return (totalMv);
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

float
mp2762getBatteryVoltageMv() {
    uint16_t voltage = mp2762aReadRegister16(MP2762A_BATTERY_VOLTAGE);
    float batteryVoltage =
        convertBitsToDoubler(voltage >>= 6, 12.5); // Battery voltage is bit 15:6 so we need a 6 bit shift
    return (batteryVoltage);
}

float
mp2762getSystemVoltageMv() {
    uint16_t voltage = mp2762aReadRegister16(MP2762A_SYSTEM_VOLTAGE);
    float batteryVoltage =
        convertBitsToDoubler(voltage >>= 6, 12.5); // Battery voltage is bit 15:6 so we need a 6 bit shift
    return (batteryVoltage);
}

uint8_t
mp2762getChargeStatus() {
    uint8_t status = 0;
    status = mp2762aReadRegister8(MP2762A_STATUS);
    status >>= 2;
    status &= 0x03;
    return (status);
}

float
mp2762getChargeCurrentMa() {
    uint8_t CurrentL = 0, CurrentH = 0;
    CurrentL = mp2762aReadRegister8(MP2762A_CHARGE_CURRENT_L);
    CurrentH = mp2762aReadRegister8(MP2762A_CHARGE_CURRENT_H);

    uint16_t Current = (CurrentH << 8) | CurrentL;
    Current = (Current >> 6) & 0x03FF;

    float charge_current_mA = Current * 12.5;
    return charge_current_mA;
}

float
mp2762getInputVolatgeMv() {
    uint8_t VoltageL = 0, VoltageH = 0;
    VoltageL = mp2762aReadRegister8(MP2762A_INPUT_VOLTAGE_L);
    VoltageH = mp2762aReadRegister8(MP2762A_INPUT_VOLTAGE_H);

    uint16_t Voltage = (static_cast<uint16_t>(VoltageH) << 8) | VoltageL;
    Voltage = (Voltage >> 6) & 0x03FF;

    float Input_voltage_mV = Voltage * 25;
    return Input_voltage_mV;
}

float
mp2762getInputCurrentMa() {
    uint8_t CurrentL = 0, CurrentH = 0;
    CurrentL = mp2762aReadRegister8(MP2762A_INPUT_CURRENT_L);
    CurrentH = mp2762aReadRegister8(MP2762A_INPUT_CURRENT_H);

    uint16_t Current = (CurrentH << 8) | CurrentL;
    Current = (Current >> 6) & 0x03FF;

    float charge_current_mA = Current * 6.25;
    return charge_current_mA;
}

void
mp2762resetSafetyTimer() {
    uint8_t status = 0;
    if (!mp2762aReadRegister8Checked(MP2762A_CONFIG_0, &status)) {
        return;
    }

    status &= ~(1 << 4); // Clear the CHG_EN bit
    mp2762aWriteRegister8(MP2762A_CONFIG_0, status);

    status |= (1 << 4); // Set the CHG_EN bit
    mp2762aWriteRegister8(MP2762A_CONFIG_0, status);
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

uint8_t
mp2762disableCharger() {
    uint8_t config = UINT8_MAX;
    (void)mp2762setChargerEnabled(false, &config);
    return config;
}

uint8_t
mp2762enableCharger() {
    uint8_t config = UINT8_MAX;
    (void)mp2762setChargerEnabled(true, &config);
    return config;
}

void
mp2762updateConfig0Status(uint8_t* config) {
    *config = mp2762aReadRegister8(MP2762A_CONFIG_0);
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