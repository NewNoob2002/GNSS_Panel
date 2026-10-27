#include "HostLog.h"
#include "Log.h"

#include <cstdlib>
#include <cstring>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace {
constexpr spdlog::level::level_enum levels[] = {spdlog::level::debug, spdlog::level::info, spdlog::level::warn,
                                                spdlog::level::err, spdlog::level::off};
std::shared_ptr<spdlog::logger> logger;

void
write_log(AppLogLevel level, const char* module, const char* message, void*) {
    // app_log_set_level 仍是唯一过滤入口，运行时改级别也同步到 spdlog。
    logger->set_level(levels[app_log_get_level()]);
    logger->log(levels[level], "[{}] {}", module, message);
}
} // namespace

void
host_log_init(std::shared_ptr<spdlog::sinks::sink> sink) {
    if (!sink) {
        auto mode = spdlog::color_mode::automatic;
        if (const char* color = std::getenv("GNSS_LOG_COLOR")) {
            if (std::strcmp(color, "always") == 0) {
                mode = spdlog::color_mode::always;
            } else if (std::strcmp(color, "never") == 0) {
                mode = spdlog::color_mode::never;
            }
        }
        sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(mode);
    }
    logger = std::make_shared<spdlog::logger>("app", std::move(sink));
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    // 同步刷新 sink，确保弱 app_fatal 调用 abort 前日志已输出。
    logger->flush_on(spdlog::level::debug);
    if (const char* requested = std::getenv("GNSS_LOG_LEVEL")) {
        constexpr const char* names[] = {"debug", "info", "warn", "error", "off"};
        for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
            if (std::strcmp(requested, names[i]) == 0) {
                app_log_set_level(static_cast<AppLogLevel>(i));
                break;
            }
        }
    }
    logger->set_level(levels[app_log_get_level()]);
    app_log_set_sink(write_log, nullptr);
}
