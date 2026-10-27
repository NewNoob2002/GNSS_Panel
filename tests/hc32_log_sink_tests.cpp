#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "AppLogSink.h"

static unsigned calls;
static uint8_t captured_level;
static const char* captured_tag;
static char captured_message[256];

extern "C" void
elog_output(uint8_t level, const char* tag, const char* file, const char* function, const long line, const char* format,
            ...) {
    ++calls;
    captured_level = level;
    captured_tag = tag;
    assert(!*file && !*function && line == 0);
    va_list args;
    va_start(args, format);
    vsnprintf(captured_message, sizeof(captured_message), format, args);
    va_end(args);
}

int
main() {
    app_log_set_sink(hc32_app_log_sink, nullptr);
    assert(app_log_set_level(APP_LOG_DEBUG));
    const uint8_t expected[] = {ELOG_LVL_DEBUG, ELOG_LVL_INFO, ELOG_LVL_WARN, ELOG_LVL_ERROR};
    for (unsigned i = 0; i < 4; ++i) {
        assert(app_log_write(static_cast<AppLogLevel>(i), "GNSS", "fix %u%%", i));
        assert(captured_level == expected[i]);
        assert(!strcmp(captured_tag, "GNSS"));
        char message[32];
        snprintf(message, sizeof(message), "fix %u%%", i);
        assert(!strcmp(captured_message, message));
    }
    hc32_app_log_sink(APP_LOG_OFF, "GNSS", "ignored", nullptr);
    hc32_app_log_sink(APP_LOG_INFO, nullptr, "ignored", nullptr);
    assert(calls == 4);
}
