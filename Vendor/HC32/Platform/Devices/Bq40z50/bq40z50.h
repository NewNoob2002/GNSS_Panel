#ifndef BQ40Z50_H
#define BQ40Z50_H

#include "I2cMaster.h"

//device address
#define BQ40Z50_DEVICE_ADDRESS           0x0B
//register address
#define BQ40Z50_TEMPERATURE              0x08
#define BQ40Z50_VOLTAGE                  0x09
// TI bq40z50 TRM SLUUA43A §12.11/12.12（p.111）：Current=0x0A、AverageCurrent=0x0B，I2，mA。
// https://www.ti.com/lit/pdf/sluua43
// SBS 1.1 §5.1.11/5.1.12（p.24）：充电为正、放电为负，AverageCurrent 为滚动平均。
// https://sbs-forum.org/specs/sbdat110.pdf
#define BQ40Z50_CURRENT                  0x0A
#define BQ40Z50_AVERAGE_CURRENT          0x0B
#define BQ40Z50_RELATIVE_STATE_OF_CHARGE 0x0D
#define BQ40Z50_REMAINING_CAPACITY       0x0F
#define BQ40Z50_FULL_CHARGE_CAPACITY     0x10

#ifdef __cplusplus
extern "C" {
#endif

bool bq40z50Begin(I2cMaster* i2cBus);

bool bq40z50ReadRegister16Checked(uint8_t reg, uint16_t* value);
/** 读取电量计平滑电流（mA，充电为正）；失败不改写输出。 */
bool bq40z50ReadAverageCurrentChecked(int16_t* current_ma);
float bq40z50getRelativeStateOfCharge_float();
#ifdef __cplusplus
}
#endif

#endif