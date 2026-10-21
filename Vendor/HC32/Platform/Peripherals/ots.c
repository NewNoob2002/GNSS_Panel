#include "ots.h"

/**
 * Specifies a clock source for OTS in this example.
 * 'OTS_CLK_SEL' can be defined as 'OTS_CLK_XTAL' or 'OTS_CLK_HRC'. */
#define OTS_CLK_SEL                     (OTS_CLK_XTAL)

/**
 * Function control of OTS.
 * Defines the following macro as non-zero to enable the corresponding function.
 *
 * 'OTS_USE_INTERRUPT': Interrupt function control.
 * 'OTS_USE_TRIG': Hardware trigger conditions control. The condition that used to start OTS.
 */
#define OTS_USE_TRIG                    (0U)

#if (OTS_USE_TRIG > 0U)
#define OTS_USE_INTERRUPT               (OTS_USE_TRIG)
#else
#define OTS_USE_INTERRUPT               (1U)
#endif

/**
 * Definitions about OTS interrupt for the example.
 * OTS independent IRQn: [INT000_IRQn, INT031_IRQn], [INT116_IRQn, INT121_IRQn].
 */
#if (OTS_USE_INTERRUPT > 0U)
#define OTS_INT_PRIO                    (DDL_IRQ_PRIO_03)
#define OTS_INT_SRC                     (INT_SRC_OTS)
#define OTS_INT_IRQn                    (INT110_IRQn)
#endif /* #if (OTS_USE_INTERRUPT > 0U) */

/* OTS parameters, slope K and offset M. Different chip, different parameters. */
#define OTS_XTAL_K                      (737272.73F)
#define OTS_XTAL_M                      (27.55F)
#define OTS_HRC_K                       (3002.59F)
#define OTS_HRC_M                       (27.92F)

/* Timeout value. */
#define OTS_TIMEOUT_VAL                 (10000U)

#if (OTS_USE_INTERRUPT > 0U)
/**
 * @brief  OTS interrupt configuration.
 * @param  None
 * @retval None
 */
static void OtsIrqConfig(func_ptr_t callback)
{
    stc_irq_signin_config_t stcIrq;

    stcIrq.enIntSrc    = OTS_INT_SRC;
    stcIrq.enIRQn      = OTS_INT_IRQn;
    stcIrq.pfnCallback = callback;

    /* Independent interrupt. */
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, OTS_INT_PRIO);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    /* Enable the specified interrupts of OTS. */
    OTS_IntCmd(ENABLE);
}

#if (OTS_USE_TRIG > 0U)
/**
 * @brief  Specifies event 'EVT_SRC_TMR0_1_CMP_A' of TIMER0 unit 1 channel A as the trigger source event of OTS. \
 *         Event 'EVT_SRC_TMR0_1_CMP_A' occurs every second.
 * @param  None
 * @retval None
 */
static void OtsTriggerConfig(void)
{
    /**
     * If a peripheral is used to generate the event which is used as a start trigger condition of OTS, \
     *   call the API of the peripheral to configure the peripheral.
     * The following operations are only used in this example.
     */

    stc_tmr0_init_t stcTMR0Init;

    /* Initials TIMER0. */
    (void)TMR0_StructInit(&stcTMR0Init);
    stcTMR0Init.u32ClockSrc     = TMR0_CLK_SRC_INTERN_CLK;
    stcTMR0Init.u32ClockDiv     = TMR0_CLK_DIV256;
    stcTMR0Init.u32Func         = TMR0_FUNC_CMP;
    stcTMR0Init.u16CompareValue = 31250UL;

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_1, ENABLE);
    (void)TMR0_Init(CM_TMR0_1, TMR0_CH_A, &stcTMR0Init);

    /* Specifies event 'EVT_SRC_TMR0_1_CMP_A' as the trigger source event of OTS. */
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    AOS_SetTriggerEventSrc(AOS_OTS, EVT_SRC_TMR0_1_CMP_A);
}
#endif /* #if (OTS_USE_TRIG > 0U) */

#endif /* #if (OTS_USE_INTERRUPT > 0U) */

/**
 * @brief  OTS initialization configuration.
 * @param  None
 * @retval None
 */
void OtsInitConfig(const func_ptr_t callback)
{
    stc_ots_init_t stcOTSInit;

    (void)OTS_StructInit(&stcOTSInit);
    stcOTSInit.u16ClockSrc = OTS_CLK_SEL;

#if (OTS_CLK_SEL == OTS_CLK_XTAL)
    stcOTSInit.f32SlopeK   = OTS_XTAL_K;
    stcOTSInit.f32OffsetM  = OTS_XTAL_M;
#else
    stcOTSInit.f32SlopeK   = OTS_HRC_K;
    stcOTSInit.f32OffsetM  = OTS_HRC_M;
#endif /* #if (OTS_CLK_SEL == OTS_CLK_XTAL) */

    /* 1. Enable OTS peripheral clock. */
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_OTS, ENABLE);

    /* 2. Initialize OTS. */
    (void)OTS_Init(&stcOTSInit);

#if (OTS_USE_INTERRUPT > 0U)
    OtsIrqConfig(callback);
#endif

#if (OTS_USE_TRIG > 0U)
    OtsTriggerConfig();
#endif
}

#if ((OTS_USE_INTERRUPT > 0U) || (OTS_USE_TRIG > 0U))
/**
 * @brief  Start OTS.
 * @param  None
 * @retval None
 */
void OtsStart(void)
{
    /**
     * If a peripheral is used to generate the event which is used as a start trigger condition of OTS, \
     *   call the API of the peripheral to start the peripheral here or anywhere else you need.
     * The following operations are only used in this example.
     */

#if (OTS_USE_TRIG > 0U)
    TMR0_Start(CM_TMR0_1, TMR0_CH_A);
#elif (OTS_USE_INTERRUPT > 0U)
    OTS_Start();
#endif
}
#endif