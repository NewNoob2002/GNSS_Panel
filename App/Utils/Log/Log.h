#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { APP_LOG_DEBUG, APP_LOG_INFO, APP_LOG_WARN, APP_LOG_ERROR, APP_LOG_OFF } AppLogLevel;
/** Sink runs synchronously. Strings are borrowed for this call only; do not recursively log.
 * APIs are LVGL/main-thread-only, not ISR-safe or a concurrent logging transport.
 * Never log credentials or sensitive payloads. Default sink writes to stderr, default level INFO.
 */
typedef void (*AppLogSink)(AppLogLevel level, const char* module, const char* message, void* context);
int app_log_set_level(AppLogLevel level); /* 0 for invalid level, 1 for success. */
AppLogLevel app_log_get_level(void);
void app_log_set_sink(AppLogSink sink, void* context); /* NULL restores stderr. */
/** Fixed 256-byte message buffer, truncation marked by '...'. Returns 1 when delivered,
 * 0 when filtered, invalid, formatting failed or a recursive sink call was suppressed.
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 3, 4)))
#endif
int app_log_write(AppLogLevel level, const char* module, const char* format, ...);
/** Unrecoverable application error (e.g. a page Model failed to register its DataNode). Logs at ERROR and
 * never returns. The default aborts the process; a platform may provide its own strong definition
 * (e.g. an MCU that records the reason and resets) because the default is a weak symbol.
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
void app_fatal(const char* module, const char* reason);

#ifdef __cplusplus
}
#endif

#define APP_LOG_D(module, ...) app_log_write(APP_LOG_DEBUG, module, __VA_ARGS__)
#define APP_LOG_I(module, ...) app_log_write(APP_LOG_INFO, module, __VA_ARGS__)
#define APP_LOG_W(module, ...) app_log_write(APP_LOG_WARN, module, __VA_ARGS__)
#define APP_LOG_E(module, ...) app_log_write(APP_LOG_ERROR, module, __VA_ARGS__)
