#include "HAL.h"
#include "mcu_define.h"

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} hardfault_stack_frame_t;

typedef struct {
    uint32_t magic;
    uint32_t exc_return;
    uint32_t msp;
    uint32_t psp;
    uint32_t control;
    uint32_t ipsr;
    hardfault_stack_frame_t stacked;
    uint32_t hfsr;
    uint32_t cfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t afsr;
    uint32_t dfsr;
    uint32_t shcsr;
} hardfault_info_t;

#define HARDFAULT_INFO_MAGIC (0x48465254UL) /* "HFRT" */

volatile hardfault_info_t g_hardfault_info;

static void HardFault_HandlerC(uint32_t* stack_frame, uint32_t exc_return) __attribute__((used, noinline));

static void
HardFault_HandlerC(uint32_t* stack_frame, uint32_t exc_return) {
    g_hardfault_info.magic = HARDFAULT_INFO_MAGIC;
    g_hardfault_info.exc_return = exc_return;
    g_hardfault_info.msp = __get_MSP();
    g_hardfault_info.psp = __get_PSP();
    g_hardfault_info.control = __get_CONTROL();
    g_hardfault_info.ipsr = __get_IPSR();

    g_hardfault_info.stacked.r0 = stack_frame[0];
    g_hardfault_info.stacked.r1 = stack_frame[1];
    g_hardfault_info.stacked.r2 = stack_frame[2];
    g_hardfault_info.stacked.r3 = stack_frame[3];
    g_hardfault_info.stacked.r12 = stack_frame[4];
    g_hardfault_info.stacked.lr = stack_frame[5];
    g_hardfault_info.stacked.pc = stack_frame[6];
    g_hardfault_info.stacked.xpsr = stack_frame[7];

    g_hardfault_info.hfsr = SCB->HFSR;
    g_hardfault_info.cfsr = SCB->CFSR;
    g_hardfault_info.mmfar = SCB->MMFAR;
    g_hardfault_info.bfar = SCB->BFAR;
    g_hardfault_info.afsr = SCB->AFSR;
    g_hardfault_info.dfsr = SCB->DFSR;
    g_hardfault_info.shcsr = SCB->SHCSR;

    CORE_DEBUG_PRINTF("\r\n*** HardFault ***\r\n");
    CORE_DEBUG_PRINTF("PC=0x%08lx LR=0x%08lx xPSR=0x%08lx EXC_RETURN=0x%08lx\r\n",
                      (unsigned long)g_hardfault_info.stacked.pc, (unsigned long)g_hardfault_info.stacked.lr,
                      (unsigned long)g_hardfault_info.stacked.xpsr, (unsigned long)g_hardfault_info.exc_return);
    CORE_DEBUG_PRINTF("R0=0x%08lx R1=0x%08lx R2=0x%08lx R3=0x%08lx R12=0x%08lx\r\n",
                      (unsigned long)g_hardfault_info.stacked.r0, (unsigned long)g_hardfault_info.stacked.r1,
                      (unsigned long)g_hardfault_info.stacked.r2, (unsigned long)g_hardfault_info.stacked.r3,
                      (unsigned long)g_hardfault_info.stacked.r12);
    CORE_DEBUG_PRINTF("MSP=0x%08lx PSP=0x%08lx CONTROL=0x%08lx IPSR=0x%08lx\r\n", (unsigned long)g_hardfault_info.msp,
                      (unsigned long)g_hardfault_info.psp, (unsigned long)g_hardfault_info.control,
                      (unsigned long)g_hardfault_info.ipsr);
    CORE_DEBUG_PRINTF("HFSR=0x%08lx CFSR=0x%08lx DFSR=0x%08lx AFSR=0x%08lx SHCSR=0x%08lx\r\n",
                      (unsigned long)g_hardfault_info.hfsr, (unsigned long)g_hardfault_info.cfsr,
                      (unsigned long)g_hardfault_info.dfsr, (unsigned long)g_hardfault_info.afsr,
                      (unsigned long)g_hardfault_info.shcsr);

    if ((g_hardfault_info.cfsr & SCB_CFSR_MMARVALID_Msk) != 0UL) {
        CORE_DEBUG_PRINTF("MMFAR=0x%08lx\r\n", (unsigned long)g_hardfault_info.mmfar);
    }
    if ((g_hardfault_info.cfsr & SCB_CFSR_BFARVALID_Msk) != 0UL) {
        CORE_DEBUG_PRINTF("BFAR=0x%08lx\r\n", (unsigned long)g_hardfault_info.bfar);
    }

    __DSB();
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0UL) {
        __BKPT(0);
    }

    while (1) {
        __NOP();
    }
}

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void
NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1) {}
    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Memory management fault.
 */
void
MemManage_Handler(void) {
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */

    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void
BusFault_Handler(void) {
    /* USER CODE BEGIN BusFault_IRQn 0 */

    /* USER CODE END BusFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void
UsageFault_Handler(void) {
    /* USER CODE BEGIN UsageFault_IRQn 0 */

    /* USER CODE END UsageFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void
SVC_Handler(void) {
    /* USER CODE BEGIN SVCall_IRQn 0 */

    /* USER CODE END SVCall_IRQn 0 */
    /* USER CODE BEGIN SVCall_IRQn 1 */

    /* USER CODE END SVCall_IRQn 1 */
}

/**
 * @brief This function handles Debug monitor.
 */
void
DebugMon_Handler(void) {
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void
PendSV_Handler(void) {
    /* USER CODE BEGIN PendSV_IRQn 0 */

    /* USER CODE END PendSV_IRQn 0 */
    /* USER CODE BEGIN PendSV_IRQn 1 */

    /* USER CODE END PendSV_IRQn 1 */
}

__attribute__((naked)) void
HardFault_Handler(void) {
    __asm volatile("tst lr, #4            \n"
                   "ite eq                \n"
                   "mrseq r0, msp         \n"
                   "mrsne r0, psp         \n"
                   "mov r1, lr            \n"
                   "b HardFault_HandlerC  \n");
}

void
SysTick_Handler() {
    HAL_IncTick();
}
