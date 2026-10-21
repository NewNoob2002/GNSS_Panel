#ifndef __MP2762A_H__
#define __MP2762A_H__
#include "I2cMaster.h"

#ifdef __cplusplus
extern "C" {
#endif
//device address
#define MP2762A_DEVICE_ADDRESS             0x5C
//register address
#define MP2762A_SETCHARGE_CURRENT          0x02
#define MP2762A_PRECHARGE_CURRENT          0x03
#define MP2762A_PRECHARGE_THRESHOLD        0x07
#define MP2762A_CONFIG_0                   0x08
#define MP2762A_CONFIG_1                   0x09
#define MP2762A_STATUS                     0x13
#define MP2762A_FAULT_REGISTER             0x14
#define MP2762A_BATTERY_VOLTAGE            0x16
#define MP2762A_SYSTEM_VOLTAGE             0x18
#define MP2762A_CHARGE_CURRENT_L           0x1A
#define MP2762A_CHARGE_CURRENT_H           0x1B
#define MP2762A_INPUT_VOLTAGE_L            0x1C
#define MP2762A_INPUT_VOLTAGE_H            0x1D
#define MP2762A_INPUT_CURRENT_L            0x1E
#define MP2762A_INPUT_CURRENT_H            0x1F
#define MP2762A_PRECHARGE_THRESHOLD_OPTION 0x30

bool mp2762aBegin(I2cMaster* i2cBus);

bool mp2762aReadRegister8Checked(uint8_t reg, uint8_t* value);
bool mp2762aReadRegister16Checked(uint8_t reg, uint16_t* value);
uint8_t mp2762aReadRegister8(uint8_t reg);

uint16_t mp2762aReadRegister16(uint8_t reg);

uint8_t mp2762aWriteRegister8(uint8_t reg, uint8_t data);

// Given a bit field, and a startingBitValue
// Example: Battery voltage is bit 12.5mV per bit
float convertBitsToDoubler(uint16_t bitField, float startingBitValue);

// Set the Precharge threshold
// 5.8V, 6.0, 6.2, 6.4, 6.6, 6.8, 7.4, 7.2 (oddly out of order)
void mp2762setFastChargeVoltageMv(uint16_t mVoltLevel);

uint8_t mp2762setFastChargeCurrentMa(uint16_t currentLevelMa);

void mp2762setPrechargeCurrentMa(uint16_t currentLevelMa);

float mp2762getBatteryVoltageMv();

float mp2762getSystemVoltageMv();

uint8_t mp2762getChargeStatus();

float mp2762getChargeCurrentMa();

float mp2762getInputVolatgeMv();

float mp2762getInputCurrentMa();

void mp2762resetSafetyTimer();

bool mp2762setChargerEnabled(bool enabled, uint8_t* config);
uint8_t mp2762disableCharger();

uint8_t mp2762enableCharger();

void mp2762updateConfig0Status(uint8_t* config);

void mp2762registerReset();

#ifdef __cplusplus
}
#endif

#endif
