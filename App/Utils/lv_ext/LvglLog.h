#pragma once

#include "Log.h"
#include "lvgl.h"
#include <string.h>

#if LV_USE_LOG
/** LVGL 的对象/渲染 INFO 也属于调试细节，避免逐帧进入应用 INFO。 */
static inline void app_lvgl_log(lv_log_level_t level, const char* message) {
    if (!message)
        return;
    switch (level) {
    case LV_LOG_LEVEL_TRACE:
    case LV_LOG_LEVEL_INFO:
        APP_LOG_D("LVGL", "%.*s", (int)strcspn(message, "\r\n"), message);
        break;
    case LV_LOG_LEVEL_WARN:
        APP_LOG_W("LVGL", "%.*s", (int)strcspn(message, "\r\n"), message);
        break;
    case LV_LOG_LEVEL_ERROR:
        APP_LOG_E("LVGL", "%.*s", (int)strcspn(message, "\r\n"), message);
        break;
    case LV_LOG_LEVEL_USER:
        APP_LOG_I("LVGL", "%.*s", (int)strcspn(message, "\r\n"), message);
        break;
    default:
        break;
    }
}
#endif

static inline void app_lvgl_log_init(void) {
#if LV_USE_LOG
    lv_log_register_print_cb(app_lvgl_log);
#endif
}
