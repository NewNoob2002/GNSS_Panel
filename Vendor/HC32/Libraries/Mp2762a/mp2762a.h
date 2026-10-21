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
#define MP2762A_STATUS                     0x13
#define MP2762A_PRECHARGE_THRESHOLD_OPTION 0x30

bool mp2762aBegin(I2cMaster* i2cBus);

bool mp2762aReadRegister8Checked(uint8_t reg, uint8_t* value);
uint8_t mp2762aReadRegister8(uint8_t reg);

uint8_t mp2762aWriteRegister8(uint8_t reg, uint8_t data);

// Set the Precharge threshold
// 5.8V, 6.0, 6.2, 6.4, 6.6, 6.8, 7.4, 7.2 (oddly out of order)
void mp2762setFastChargeVoltageMv(uint16_t mVoltLevel);

uint8_t mp2762setFastChargeCurrentMa(uint16_t currentLevelMa);

void mp2762setPrechargeCurrentMa(uint16_t currentLevelMa);

uint8_t mp2762getChargeStatus();

bool mp2762setChargerEnabled(bool enabled, uint8_t* config);
void mp2762registerReset();

#ifdef __cplusplus
}
#endif

#endif
