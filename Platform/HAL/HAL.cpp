#include "HAL.h"
#include "Arduino.h"
#include "Charger.h"
#include "MillisTaskManager/MillisTaskManager.h"
#include "SystemState.h"
#include "elog.h"

static MillisTaskManager taskManager;

static void
HAL_MONITOR_TASK(void* e) {
    (void)e;
    HAL::Power_PowerOffMonitor();
    HAL::Key_Update();
}

static void
HAL_POWER_TASK(void* e) {
    (void)e;
    HAL::Power_Update();
}

static void
HAL_LED_TASK(void* e) {
    (void)e;
    HAL::Led_Update();
}

void SystemClock_Config(void);

// Boot 跳转后中断控制器可能残留 Boot 的配置，入口处统一复位。
void
HAL::ResetInterruptController() {
    constexpr uint32_t irq_count = 128U;
    constexpr uint32_t vector_irq_count = 16U;
    constexpr uint32_t intc_sel_reset_value = 0x1FFUL;

    __disable_irq();

    for (uint32_t i = 0; i < irq_count / 32U; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    volatile uint32_t* const intc_sel = &CM_INTC->SEL0;
    for (uint32_t i = 0; i < irq_count; i++) {
        intc_sel[i] = intc_sel_reset_value;
    }

    volatile uint32_t* const intc_vssel = &CM_INTC->VSSEL128;
    for (uint32_t i = 0; i < vector_irq_count; i++) {
        intc_vssel[i] = 0UL;
    }

    CM_INTC->SWIER = 0UL;
    CM_INTC->EVTER = 0UL;
    CM_INTC->WUPEN = 0UL;
    CM_INTC->EIFCR = 0xFFFFUL;

    __DSB();
    __ISB();
    __enable_irq();
}

// namesapce HAL
void
HAL::HAL_Init() {
    disable_JTAG();
    dwt_init();
    SystemClock_Config();
#ifdef __CORE_DEBUG
    /* set EasyLogger log format */
    elog_init();
    /* set EasyLogger log format */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
    /* start EasyLogger */
    elog_start();
#endif
    Power_Init();
    Battery_Init();
    /* Set Interrupt Group Priority */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    State::Reset();
    /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) */
    HAL_InitTick(TICK_INT_PRIORITY);

    I2C_Scan();
    // 扫描结果决定主板版本（V1.5 带 MP2762A，V1.3 用 GPIO 控制充电）。
    Charger_Get().begin(State::Power().batteryInfo);
    CORE_DEBUG_PRINTF("Board %s, charger %s", systemInfo.board.hardware_version, Charger_Get().name());
    Key_Init();

    taskManager.Register(HAL_MONITOR_TASK, PRODUCT_MONITOR_PERIOD_MS);
    taskManager.Register(HAL_LED_TASK, 100);
    taskManager.Register(HAL_POWER_TASK, 500);
}

void
HAL::HAL_Update() {
    taskManager.Running(millis());
    static bool reset_pending = false;
    static uint32_t reset_started_ms = 0;
    if (!reset_pending && HAL::Power_ShutdownSoftReset()) {
        reset_pending = true;
        reset_started_ms = millis();
    }
    if (reset_pending && millis() - reset_started_ms >= 100U) {
        // 当前 Boot 根据软件复位标志进入升级窗口，无共享 SRAM 命令。
        __DSB();
        NVIC_SystemReset();
    }
}

/** @addtogroup HAL_Private_Variables
 * @{
 */
__IO uint32_t uwTick;
uint32_t uwTickPrio = (1UL << __NVIC_PRIO_BITS);        /* Invalid PRIO */
HAL_TickFreqTypeDef uwTickFreq = HAL_TICK_FREQ_DEFAULT; /* 1KHz */

/**
 * @brief This function configures the source of the time base.
 *        The time source is configured  to have 1ms time base with a dedicated
 *        Tick interrupt priority.
 * @note This function is called  automatically at the beginning of program after
 *       reset by HAL_Init() or at any time when clock is reconfigured  by HAL_RCC_ClockConfig().
 * @note In the default implementation, SysTick timer is the source of time base.
 *       It is used to generate interrupts at regular time intervals.
 *       Care must be taken if HAL_Delay() is called from a peripheral ISR process,
 *       The SysTick interrupt must have higher priority (numerically lower)
 *       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
 *       The function is declared as __weak  to be overwritten  in case of other
 *       implementation  in user file.
 * @param TickPriority Tick interrupt priority.
 * @retval HAL status
 */
__weak HAL_StatusTypeDef
HAL_InitTick(uint32_t TickPriority) {
    /* Configure the SysTick to have interrupt in 1ms time basis*/
    if (HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq)) > 0U) {
        return HAL_ERROR;
    }

    /* Configure the SysTick IRQ priority */
    if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
        HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
        uwTickPrio = TickPriority;
    } else {
        return HAL_ERROR;
    }

    /* Return function status */
    return HAL_OK;
}

/** @defgroup HAL_Exported_Functions_Group2 HAL Control functions
 *  @brief    HAL Control functions
 *
@verbatim
 ===============================================================================
                      ##### HAL Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Provide a tick value in millisecond
      (+) Provide a blocking delay in millisecond
      (+) Suspend the time base source interrupt
      (+) Resume the time base source interrupt
      (+) Get the HAL API driver version
      (+) Get the device identifier
      (+) Get the device revision identifier
      (+) Enable/Disable Debug module during SLEEP mode
      (+) Enable/Disable Debug module during STOP mode
      (+) Enable/Disable Debug module during STANDBY mode

@endverbatim
  * @{
  */

/**
 * @brief This function is called to increment  a global variable "uwTick"
 *        used as application time base.
 * @note In the default implementation, this variable is incremented each 1ms
 *       in SysTick ISR.
 * @note This function is declared as __weak to be overwritten in case of other
 *      implementations in user file.
 * @retval None
 */
__weak void
HAL_IncTick(void) {
    uwTick += uwTickFreq;
}

/**
 * @brief Provides a tick value in millisecond.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval tick value
 */
__weak uint32_t
HAL_GetTick(void) {
    return uwTick;
}

/**
 * @brief This function returns a tick priority.
 * @retval tick priority
 */
uint32_t
HAL_GetTickPrio(void) {
    return uwTickPrio;
}

/**
 * @brief Set new tick Freq.
 * @retval Status
 */
HAL_StatusTypeDef
HAL_SetTickFreq(HAL_TickFreqTypeDef Freq) {
    HAL_StatusTypeDef status = HAL_OK;
    HAL_TickFreqTypeDef prevTickFreq;

    DDL_ASSERT(IS_TICKFREQ(Freq));

    if (uwTickFreq != Freq) {
        /* Back up uwTickFreq frequency */
        prevTickFreq = uwTickFreq;

        /* Update uwTickFreq global variable used by HAL_InitTick() */
        uwTickFreq = Freq;

        /* Apply the new tick Freq  */
        status = HAL_InitTick(uwTickPrio);

        if (status != HAL_OK) {
            /* Restore previous tick frequency */
            uwTickFreq = prevTickFreq;
        }
    }

    return status;
}

/**
 * @brief Return tick frequency.
 * @retval Tick frequency.
 *         Value of @ref HAL_TickFreqTypeDef.
 */
HAL_TickFreqTypeDef
HAL_GetTickFreq(void) {
    return uwTickFreq;
}

/**
 * @brief Suspend Tick increment.
 * @note In the default implementation , SysTick timer is the source of time base. It is
 *       used to generate interrupts at regular time intervals. Once HAL_SuspendTick()
 *       is called, the SysTick interrupt will be disabled and so Tick increment
 *       is suspended.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval None
 */
__weak void
HAL_SuspendTick(void) {
    /* Disable SysTick Interrupt */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

/**
 * @brief Resume Tick increment.
 * @note In the default implementation , SysTick timer is the source of time base. It is
 *       used to generate interrupts at regular time intervals. Once HAL_ResumeTick()
 *       is called, the SysTick interrupt will be enabled and so Tick increment
 *       is resumed.
 * @note This function is declared as __weak to be overwritten in case of other
 *       implementations in user file.
 * @retval None
 */
__weak void
HAL_ResumeTick(void) {
    /* Enable SysTick Interrupt */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void
SystemClock_Config(void) {

    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcMpllInit;

    GPIO_AnalogCmd(XTAL_PORT, XTAL_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    (void)CLK_PLLStructInit(&stcMpllInit);

    /* Set bus clk div. */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL, (CLK_HCLK_DIV1 | CLK_EXCLK_DIV2 | CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4
                                      | CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2));

    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    /* MPLL config (XTAL / pllmDiv * plln / PllpDiv = 200M). */
    stcMpllInit.PLLCFGR = 0UL;
    stcMpllInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLN = 50UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLP = 2UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLQ = 2UL - 1UL;
    stcMpllInit.PLLCFGR_f.PLLR = 2UL - 1UL;
    stcMpllInit.u8PLLState = CLK_PLL_ON;
    stcMpllInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcMpllInit);
    /* Wait MPLL ready. */
    while (SET != CLK_GetStableStatus(CLK_STB_FLAG_PLL)) {
        ;
    }

    /* sram init include read/write wait cycle setting */
    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE0, SRAM_WAIT_CYCLE0);
    SRAM_SetWaitCycle((SRAM_SRAM12 | SRAM_SRAM3 | SRAM_SRAMR), SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);

    /* flash read wait cycle setting */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE5);
    /* 3 cycles for 126MHz ~ 200MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT3);
    /* Switch driver ability */
    (void)PWC_HighSpeedToHighPerformance();
    /* Switch system clock source to MPLL. */
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
    /* Reset cache ram */
    EFM_CacheRamReset(ENABLE);
    EFM_CacheRamReset(DISABLE);
    /* Enable cache */
    EFM_CacheCmd(ENABLE);

    SystemCoreClockUpdate();
}