#include "Log.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
AppLogLevel minimum = APP_LOG_INFO;
AppLogSink output = nullptr;
void* output_context = nullptr;
bool delivering = false;
void stderr_sink(AppLogLevel level, const char* module, const char* message, void*) {
    constexpr const char* names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    std::fprintf(stderr, "[%s][%s] %s\n", names[level], module, message);
}
} // namespace

int app_log_set_level(AppLogLevel level) {
    if (level < APP_LOG_DEBUG || level > APP_LOG_OFF)
        return 0;
    minimum = level;
    return 1;
}
AppLogLevel app_log_get_level() {
    return minimum;
}
void app_log_set_sink(AppLogSink sink, void* context) {
    output = sink;
    output_context = context;
}
int app_log_write(AppLogLevel level, const char* module, const char* format, ...) {
    if (level < APP_LOG_DEBUG || level >= APP_LOG_OFF || level < minimum || !module || !format || delivering)
        return 0;
    char message[256];
    va_list arguments;
    va_start(arguments, format);
    const int count = std::vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (count < 0)
        return 0;
    if (static_cast<unsigned>(count) >= sizeof(message))
        std::memcpy(message + sizeof(message) - 4, "...", 4);
    delivering = true;
    (output ? output : stderr_sink)(level, module, message, output_context);
    delivering = false;
    return 1;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
void app_fatal(const char* module, const char* reason) {
    app_log_write(APP_LOG_ERROR, module ? module : "App", "fatal: %s", reason ? reason : "unknown");
    std::abort();
}
