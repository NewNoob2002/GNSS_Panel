#include "board_peripherals.h"

// 配置依据：原 i2c_master.cpp 的主板外设、时钟与复用选择；静态构造不访问硬件。
I2cMaster board_i2c({CM_I2C2, PWC_FCG1_I2C2, PA9, PA8, GPIO_FUNC_51, GPIO_FUNC_50});
