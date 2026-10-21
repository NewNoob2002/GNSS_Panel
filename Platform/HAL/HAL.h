#ifndef __HAL_H
#define __HAL_H

#include "../../../Arduino/core_debug.h"
#include "CommonMacro.h"
#include "HAL_CONFIG.h"
#include "mcu_config.h"

// pi-lens-ignore: identifier
#ifdef __cplusplus
namespace HAL {
void ResetInterruptController();
void HAL_Init();
void HAL_Update();

void HAL_Deinit();
/* I2C */
int I2C_Scan();
/* 重新探测单个 I2C 器件（SystemLoading 自检）：开机扫描时未发现、现在应答的器件按扫描流程补登记并初始化；
 * 不清除已登记的在线标志（电量计标志清除会解除充电温度保护，充电芯片标志清除会在运行中切换充电方式）。 */
bool I2C_ProbeDevice(uint8_t address);

/* Power */
void Power_Init();
void Battery_Init();
void Power_OnCheck(void (*idle)() = nullptr);
void Power_Shutdown(bool en);
void Power_PowerOffMonitor();
bool Power_ShutdownSoftReset();
void Power_Update();
void Power_GetInfo(Power_Monitor_t* info);
/* 产品电源扩展点（弱符号默认实现） */
void Power_ProductInit();
void Power_ProductUpdate();
uint16_t Power_AdjustPercent(uint16_t actualPercent);
const char* Power_GetPowerOffCause();
void WatchDog_Feed();
/* KEY：Key_Init/Key_Update 共用（含 Function 长按强制关机），产品按键动作走扩展点（弱符号默认实现） */
void Key_Init();
void Key_Update();
void Key_ProductInit();
void Key_ProductUpdate();
// 带屏产品两键的当前电平（已按有效电平归一化），共享应用按 RK3506 两键语义解释。
bool Key_PowerPressed();
bool Key_FunctionPressed();
/* LED */
void Led_Update();
/* Dispaly */
void Display_Init();
} // namespace HAL
#endif /*__cplusplus*/

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup HAL_Exported_Constants HAL Exported Constants
  * @{
  */

/** @defgroup HAL_TICK_FREQ Tick Frequency
  * @{
  */
typedef enum {
    HAL_TICK_FREQ_10HZ = 100U,
    HAL_TICK_FREQ_100HZ = 10U,
    HAL_TICK_FREQ_1KHZ = 1U,
    HAL_TICK_FREQ_DEFAULT = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;

/** @defgroup HAL_Private_Macros HAL Private Macros
  * @{
  */
#define IS_TICKFREQ(FREQ)                                                                                              \
    (((FREQ) == HAL_TICK_FREQ_10HZ) || ((FREQ) == HAL_TICK_FREQ_100HZ) || ((FREQ) == HAL_TICK_FREQ_1KHZ))

/** @addtogroup HAL_Exported_Variables
  * @{
  */
extern volatile uint32_t uwTick;
extern uint32_t uwTickPrio;
extern HAL_TickFreqTypeDef uwTickFreq;

/* Exported functions --------------------------------------------------------*/
/** @addtogroup HAL_Exported_Functions
  * @{
  */
/** @addtogroup HAL_Exported_Functions_Group1
  * @{
  */
/* Initialization and Configuration functions  ******************************/
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);

void HAL_IncTick(void);
uint32_t HAL_GetTick(void);
uint32_t HAL_GetTickPrio(void);
HAL_StatusTypeDef HAL_SetTickFreq(HAL_TickFreqTypeDef Freq);
HAL_TickFreqTypeDef HAL_GetTickFreq(void);

void HAL_SuspendTick(void);
void HAL_ResumeTick(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif