#include "Log.h"

// 保留共享 ABI，直接调用 API 也不会触发 stdio 格式化。
int
app_log_set_level(AppLogLevel level) {
    return level >= APP_LOG_DEBUG && level <= APP_LOG_OFF;
}

AppLogLevel
app_log_get_level(void) {
    return APP_LOG_OFF;
}

void
app_log_set_sink(AppLogSink, void*) {}

int
app_log_write(AppLogLevel, const char*, const char*, ...) {
    return 0;
}
