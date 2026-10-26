#include <HAL.h>
#include "lv_port.h"

void
HAL::Display_SetBrightness(uint8_t value) {
    lv_port_set_brightness(value);
}

#ifdef __CORE_DEBUG
static void
lv_log_print(lv_log_level_t, const char* buf) {
    CORE_DEBUG_PRINTF("%s", buf);
}
#endif

void
HAL::Display_Init() {
    if (!lv_is_initialized()) {
        lv_init();
    }
#ifdef __CORE_DEBUG
    lv_log_register_print_cb(lv_log_print);
#endif
    CORE_DEBUG_PRINTF("Display Init");
    lv_port_init();
}
