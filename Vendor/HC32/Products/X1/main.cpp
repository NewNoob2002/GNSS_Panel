#include "Arduino.h"
#include "HAL.h"
#include "slave_i2c.h"

/**
 * @brief  X1 无屏产品入口
 */
int
main(void) {
    HAL::ResetInterruptController();
    /* Peripheral registers write unprotected */
    LL_PERIPH_WE(EXAMPLE_PERIPH_WE);
    HAL::HAL_Init();
    HAL::Power_OnCheck();
    // 沿用 X1_Panel_dev：开机后等待 1 s 再启动与主控的通信。
    delay_ms(1000);
    slave_i2c_init();
    /* Peripheral registers write protected */
    LL_PERIPH_WP(EXAMPLE_PERIPH_WP);
    while (1) {
        slave_i2c_update();
        HAL::HAL_Update();
    }
}
