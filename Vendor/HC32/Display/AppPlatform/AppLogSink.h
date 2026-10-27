#pragma once
#include "Log.h"
#include "elog.h"

// HAL_Init 已启动 EasyLogger；保留调用方模块标签，不伪造桥接层文件/函数位置。
inline void
hc32_app_log_sink(AppLogLevel level, const char* module, const char* message, void*) {
    static constexpr uint8_t levels[] = {ELOG_LVL_DEBUG, ELOG_LVL_INFO, ELOG_LVL_WARN, ELOG_LVL_ERROR};
    if (level < APP_LOG_DEBUG || level >= APP_LOG_OFF || !module || !message) {
        return;
    }
    elog_output(levels[level], module, "", "", 0, "%s", message);
}
