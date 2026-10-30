#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include "Utils/lv_ext/LvglLog.h"

namespace {
struct Message {
    AppLogLevel level;
    std::string text;
};

void
capture(AppLogLevel level, const char* module, const char* message, void* context) {
    assert(std::strcmp(module, "LVGL") == 0);
    auto& messages = *static_cast<std::vector<Message>*>(context);
    messages.push_back({level, message});
}
} // namespace

int
main() {
    lv_init();
    app_lvgl_log_init();
    std::vector<Message> messages;
    app_log_set_sink(capture, &messages);
    assert(app_log_set_level(APP_LOG_DEBUG));
    // 使用 LVGL 真正的注册/分发路径，验证级别、固定 %s 格式和去掉末尾换行。
    LV_LOG_TRACE("trace 100%%");
    LV_LOG_INFO("object created");
    LV_LOG_WARN("warning 100%%");
    LV_LOG_ERROR("failure");
    LV_LOG_USER("user");
#if APP_LOG_BUILD_DEBUG
    // Debug 从 INFO 起：TRACE 为逐次内存分配与定时器调度，过于密集。
    static_assert(LV_LOG_LEVEL == LV_LOG_LEVEL_INFO);
    assert(messages.size() == 4);
    assert(messages[0].level == APP_LOG_DEBUG && messages[0].text.find("object created") != std::string::npos);
    const unsigned offset = 1;
#else
    static_assert(LV_LOG_LEVEL == LV_LOG_LEVEL_WARN);
    assert(messages.size() == 3);
    const unsigned offset = 0;
#endif
    assert(messages[offset].level == APP_LOG_WARN);
    assert(messages[offset].text.find("warning 100%") != std::string::npos);
    assert(messages[offset + 1].level == APP_LOG_ERROR);
    assert(messages[offset + 2].level == APP_LOG_INFO);
    for (const auto& message : messages) {
        assert(message.text.find_first_of("\r\n") == std::string::npos);
    }
    assert(app_log_set_level(APP_LOG_ERROR));
    const auto count = messages.size();
    LV_LOG_WARN("filtered");
    assert(messages.size() == count);
    LV_LOG_ERROR("delivered");
    assert(messages.size() == count + 1);
    app_lvgl_log(LV_LOG_LEVEL_NONE, "ignored");
    app_lvgl_log(LV_LOG_LEVEL_ERROR, nullptr);
    assert(messages.size() == count + 1);
    app_log_set_sink(nullptr, nullptr);
    lv_deinit();
}
