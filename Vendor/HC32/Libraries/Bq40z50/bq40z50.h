#ifndef BQ40Z50_H
#define BQ40Z50_H

#include "Wire.h"

//device address
#define BQ40Z50_DEVICE_ADDRESS           0x0B
//register address
#define BQ40Z50_TEMPERATURE              0x08
#define BQ40Z50_VOLTAGE                  0x09
#define BQ40Z50_CURRENT                  0x0A
#define BQ40Z50_AVERAGE_CURRENT          0x0B
#define BQ40Z50_MAX_ERROR                0x0C
#define BQ40Z50_RELATIVE_STATE_OF_CHARGE 0x0D
#define BQ40Z50_ABSOLUTE_STATE_OF_CHARGE 0x0E
#define BQ40Z50_REMAINING_CAPACITY       0x0F
#define BQ40Z50_FULL_CHARGE_CAPACITY     0x10
#define BQ40Z50_RUNTIME_TO_EMPTY         0x11
#define BQ40Z50_AVERAGE_TIME_TO_EMPTY    0x12
#define BQ40Z50_AVERAGE_TIME_TO_FULL     0x13
#define BQ40Z50_CHARGING_CURRENT         0x14
#define BQ40Z50_CHARGING_VOLTAGE         0x15
#define BQ40Z50_CYCLE_COUNT              0x17
#define BQ40Z50_CELL_VOLTAGE_1           0x3F
#define BQ40Z50_CELL_VOLTAGE_2           0x3E
#define BQ40Z50_CELL_VOLTAGE_3           0x3D
#define BQ40Z50_CELL_VOLTAGE_4           0x3C

#ifdef __cplusplus
extern "C" {
#endif

bool bq40z50Begin(TwoWire* i2cBus);

uint8_t bq40z50ReadRegister8(uint8_t reg);

bool bq40z50ReadRegister16Checked(uint8_t reg, uint16_t* value);
uint16_t bq40z50ReadRegister16(uint8_t reg);

uint16_t bq40z50getTemperatureC();

uint16_t bq40z50getVoltageMv();

int16_t bq40z50getCurrentMa();

int16_t bq40z50getAverageCurrentMa();

uint8_t bq40z50getMaxError();

float bq40z50getRelativeStateOfCharge_float();
uint16_t bq40z50getRelativeStateOfCharge();

uint8_t bq40z50getAbsoluteStateOfCharge();

uint16_t bq40z50getRemainingCapacityMah();

uint16_t bq40z50getFullChargeCapacityMah();

uint16_t bq40z50getRunTimeToEmptyMin();

uint16_t bq40z50getAverageTimeToEmptyMin();

uint16_t bq40z50getAverageTimeToFullMin();

uint16_t bq40z50getChargingCurrentMa();

uint16_t bq40z50getChargingVoltageMv();

uint16_t bq40z50getCycleCount();

uint16_t bq40z50getCellVoltage1Mv();

uint16_t bq40z50getCellVoltage2Mv();

uint16_t bq40z50getCellVoltage3Mv();

uint16_t bq40z50getCellVoltage4Mv();

#ifdef __cplusplus
}
#endif

#endif