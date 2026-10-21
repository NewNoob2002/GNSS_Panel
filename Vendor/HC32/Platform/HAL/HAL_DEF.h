/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HAL_DEF
#define __HAL_DEF

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/**
  * @brief  HAL Status structures definition
  */
typedef enum { HAL_OK = 0x00U, HAL_ERROR = 0x01U, HAL_BUSY = 0x02U, HAL_TIMEOUT = 0x03U } HAL_StatusTypeDef;

/**
  * @brief  HAL Lock structures definition
  */
typedef enum { HAL_UNLOCKED = 0x00U, HAL_LOCKED = 0x01U } HAL_LockTypeDef;

/* Exported macro ------------------------------------------------------------*/

#if !defined(UNUSED)
#define UNUSED(X) (void)X /* To avoid gcc/g++ warnings */
#endif                    /* UNUSED */

#define HAL_MAX_DELAY            0xFFFFFFFFU

#define HAL_IS_BIT_SET(REG, BIT) (((REG) & (BIT)) == (BIT))
#define HAL_IS_BIT_CLR(REG, BIT) (((REG) & (BIT)) == 0U)

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#elif defined(__GNUC__) && !defined(__CC_ARM) /* GNU Compiler */
#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif /* __packed */
#endif /* __GNUC__ */

#define TICK_INT_PRIORITY 15U /*!< tick interrupt priority */

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif
