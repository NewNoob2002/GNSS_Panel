#ifndef __STM32F4xx_HAL_CORTEX_H
#define __STM32F4xx_HAL_CORTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL_DEF.h"
#if defined(HC32F460)
#include "hc32_ll.h"
#endif
/** @defgroup CORTEX_Exported_Constants CORTEX Exported Constants
  * @{
  */

/** @defgroup CORTEX_Preemption_Priority_Group CORTEX Preemption Priority Group
  * @{
  */
#define NVIC_PRIORITYGROUP_0                                                                                           \
    0x00000007U /*!< 0 bits for pre-emption priority
                                                      4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1                                                                                           \
    0x00000006U /*!< 1 bits for pre-emption priority
                                                      3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2                                                                                           \
    0x00000005U /*!< 2 bits for pre-emption priority
                                                      2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3                                                                                           \
    0x00000004U /*!< 3 bits for pre-emption priority
                                                      1 bits for subpriority */
#define NVIC_PRIORITYGROUP_4                                                                                           \
    0x00000003U /*!< 4 bits for pre-emption priority
                                                      0 bits for subpriority */

/** @addtogroup CORTEX_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions *****************************/
void HAL_NVIC_SetPriorityGrouping(uint32_t PriorityGroup);
#if defined(HC32F460)
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);
#endif
void HAL_NVIC_SystemReset(void);
uint32_t HAL_SYSTICK_Config(uint32_t TicksNumb);

#define IS_NVIC_PRIORITY_GROUP(GROUP)                                                                                  \
    (((GROUP) == NVIC_PRIORITYGROUP_0) || ((GROUP) == NVIC_PRIORITYGROUP_1) || ((GROUP) == NVIC_PRIORITYGROUP_2)       \
     || ((GROUP) == NVIC_PRIORITYGROUP_3) || ((GROUP) == NVIC_PRIORITYGROUP_4))

#define IS_NVIC_PREEMPTION_PRIORITY(PRIORITY) ((PRIORITY) < 0x10U)

#define IS_NVIC_SUB_PRIORITY(PRIORITY)        ((PRIORITY) < 0x10U)

#define IS_NVIC_DEVICE_IRQ(IRQ)               ((IRQ) >= (IRQn_Type)0x00U)

#ifdef __cplusplus
}
#endif

#endif