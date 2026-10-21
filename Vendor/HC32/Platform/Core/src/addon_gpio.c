#include "addon_gpio.h"

#if (LL_GPIO_ENABLE == DDL_ON)

void
disable_JTAG() {
    WRITE_REG16(CM_GPIO->PSPCR, 0x03);
}
#endif
