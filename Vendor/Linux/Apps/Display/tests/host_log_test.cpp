#include "HostLog.h"
#include "Log.h"

#include <cassert>
#include <csignal>
#include <cstdlib>
#include <regex>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

int
main() {
    assert(unsetenv("GNSS_LOG_LEVEL") == 0);
    auto ring = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(16);
    host_log_init(ring);
    assert(app_log_get_level() == APP_LOG_DEFAULT_LEVEL);
#if APP_LOG_BUILD_DEBUG
    static_assert(APP_LOG_COMPILE_LEVEL == APP_LOG_DEBUG);
    assert(app_log_get_level() == APP_LOG_DEBUG);
#else
    static_assert(APP_LOG_COMPILE_LEVEL == APP_LOG_INFO);
    assert(app_log_get_level() == APP_LOG_INFO);
#endif
    assert(setenv("GNSS_LOG_LEVEL", "debug", 1) == 0);
    host_log_init(ring);
    assert(app_log_get_level() == APP_LOG_DEBUG);
    int evaluated = 0;
    APP_LOG_D("Test", "value=%d %s", ++evaluated, "{literal}");
    assert(evaluated == (APP_LOG_COMPILE_LEVEL == APP_LOG_DEBUG ? 1 : 0));
    APP_LOG_I("Test", "info");
    APP_LOG_W("Test", "warning");
    APP_LOG_E("Test", "error");
    auto raw = ring->last_raw();
    assert(raw.size() == 4 - APP_LOG_COMPILE_LEVEL);
    constexpr spdlog::level::level_enum expected[] = {spdlog::level::debug, spdlog::level::info, spdlog::level::warn,
                                                      spdlog::level::err};
    for (unsigned i = 0; i < raw.size(); ++i) {
        assert(raw[i].level == expected[i + APP_LOG_COMPILE_LEVEL]);
        assert(raw[i].logger_name == "app");
    }
#if APP_LOG_COMPILE_LEVEL == 0
    assert(std::regex_match(ring->last_formatted()[0],
                            std::regex(R"(\[\d{2}:\d{2}:\d{2}\.\d{3}\] \[debug\] \[Test\] value=1 \{literal\}\n)")));
#else
    assert(ring->last_formatted()[0].find("[info] [Test] info\n") != std::string::npos);
#endif

    // 逐档验证环境变量过滤，并确认运行时降回 DEBUG 不会被 spdlog 二次丢弃。
    constexpr const char* names[] = {"debug", "info", "warn", "error", "off"};
    for (unsigned minimum = 0; minimum < 5; ++minimum) {
        assert(setenv("GNSS_LOG_LEVEL", names[minimum], 1) == 0);
        ring = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(16);
        host_log_init(ring);
        for (unsigned level = 0; level < 4; ++level) {
            assert(app_log_write(static_cast<AppLogLevel>(level), "Filter", "message")
                   == (level >= minimum && static_cast<int>(level) >= APP_LOG_COMPILE_LEVEL));
        }
        const unsigned effective = minimum > APP_LOG_COMPILE_LEVEL ? minimum : APP_LOG_COMPILE_LEVEL;
        assert(ring->last_raw().size() == (effective < 4 ? 4 - effective : 0));
        assert(app_log_set_level(APP_LOG_DEBUG));
        assert(APP_LOG_D("Filter", "runtime debug") == (APP_LOG_COMPILE_LEVEL == APP_LOG_DEBUG));
#if APP_LOG_COMPILE_LEVEL == 0
        assert(ring->last_raw().back().level == spdlog::level::debug);
#endif
    }
    assert(setenv("GNSS_LOG_LEVEL", "invalid", 1) == 0);
    assert(app_log_set_level(APP_LOG_WARN));
    std::ostringstream output;
    host_log_init(std::make_shared<spdlog::sinks::ostream_sink_mt>(output));
    assert(app_log_get_level() == APP_LOG_WARN);
    APP_LOG_E("Stream", "formatted %d", 7);
    assert(output.str().find("[error] [Stream] formatted 7\n") != std::string::npos);
    assert(output.str().find('\033') == std::string::npos);

    unsigned delivered = 0;
    app_log_set_sink([](AppLogLevel, const char*, const char*, void* context) { ++*static_cast<unsigned*>(context); },
                     &delivered);
    APP_LOG_W("Custom", "replacement sink");
    assert(delivered == 1);

    // 子进程验证默认 fatal 在 abort 前已将同步 sink 刷新到管道。
    int descriptors[2];
    assert(pipe(descriptors) == 0);
    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(descriptors[0]);
        assert(dup2(descriptors[1], STDOUT_FILENO) >= 0);
        close(descriptors[1]);
        assert(setenv("GNSS_LOG_LEVEL", "info", 1) == 0);
        assert(setenv("GNSS_LOG_COLOR", "never", 1) == 0);
        host_log_init();
        app_fatal("Fatal", "test failure");
    }
    close(descriptors[1]);
    std::string fatal_output;
    char buffer[256];
    ssize_t count;
    while ((count = read(descriptors[0], buffer, sizeof(buffer))) > 0) {
        fatal_output.append(buffer, static_cast<std::size_t>(count));
    }
    assert(count == 0);
    close(descriptors[0]);
    int status;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    assert(fatal_output.find("[error] [Fatal] fatal: test failure\n") != std::string::npos);
    app_log_set_sink(nullptr, nullptr);
    return 0;
}
