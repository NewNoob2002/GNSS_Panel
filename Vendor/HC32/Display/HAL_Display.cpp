#include <HAL.h>
#include "Utils/lv_ext/LvglLog.h"
#include "lv_port.h"
#if defined(HC32_LOG_ENABLED)
#include "AppLogSink.h"
#endif

void
HAL::Display_SetBrightness(uint8_t value) {
    lv_port_set_brightness(value);
}


void
HAL::Display_Init() {
    if (!lv_is_initialized()) {
        lv_init();
    }
#if defined(HC32_LOG_ENABLED)
    app_log_set_sink(hc32_app_log_sink, nullptr);
#endif
    app_lvgl_log_init();
    APP_LOG_I("HAL", "Display Init");
    lv_port_init();
}
