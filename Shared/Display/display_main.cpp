/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "AppPlatform.h"
#include "HAL/HAL.h"
#include "lv_port.h"
#include "slave_i2c.h"

// 等待开机与主循环都需要继续刷新界面，并回收已完成的显示 DMA。
static void
display_update() {
    App_Update();
    lv_timer_handler();
    lv_port_disp_poll();
}

/**
 * @brief  X1Pro / XBase 带屏产品入口
 * @param  None
 * @return This function does not return.
 */
int
main(void) {
    /* Peripheral registers write unprotected */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    HAL::ResetInterruptController();
    HAL::HAL_Init();
    HAL::Display_Init();
    App_Init();
    HAL::Power_OnCheck(display_update);
    /* Configure BSP */
    slave_i2c_init();
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
    while (1) {
        slave_i2c_update();
        HAL::HAL_Update();
        display_update();
        __WFI();
    }
}
