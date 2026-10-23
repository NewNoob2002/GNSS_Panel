#ifndef BQ40Z50_H
#define BQ40Z50_H

#include "I2cMaster.h"

//device address
#define BQ40Z50_DEVICE_ADDRESS           0x0B
//register address
#define BQ40Z50_TEMPERATURE              0x08
#define BQ40Z50_VOLTAGE                  0x09
#define BQ40Z50_RELATIVE_STATE_OF_CHARGE 0x0D
#define BQ40Z50_REMAINING_CAPACITY       0x0F
#define BQ40Z50_FULL_CHARGE_CAPACITY     0x10

#ifdef __cplusplus
extern "C" {
#endif

bool bq40z50Begin(I2cMaster* i2cBus);

bool bq40z50ReadRegister16Checked(uint8_t reg, uint16_t* value);
float bq40z50getRelativeStateOfCharge_float();
#ifdef __cplusplus
}
#endif

#endif