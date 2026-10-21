#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include "Frameworks/DataBroker/DataBroker.h"
#include "Utils/Log/Log.h"

namespace {
struct LogCapture {
    unsigned count = 0;
    AppLogLevel level{};
    std::string module, message;
};
void sink(AppLogLevel level, const char* module, const char* message, void* context) {
    auto& output = *static_cast<LogCapture*>(context);
    ++output.count;
    output.level = level;
    output.module = module;
    output.message = message;
    assert(!APP_LOG_E("Recursive", "must not recurse"));
}
struct Received {
    unsigned value = 0, calls = 0;
    std::unique_ptr<DataNode>* remove = nullptr;
};

int
receive(DataNode* account, DataNode::EventParam_t* event) {
    if (!event || !event->data_p || event->size != sizeof(unsigned))
        return DataNode::RES_PARAM_ERROR;
    auto& target = *static_cast<Received*>(account->getUserData());
    std::memcpy(&target.value, event->data_p, sizeof(target.value));
    ++target.calls;
    if (target.remove)
        target.remove->reset();
    return DataNode::RES_OK;
}
} // namespace
int main() {
    LogCapture logs;
    app_log_set_sink(sink, &logs);
    assert(app_log_set_level(APP_LOG_INFO));
    assert(!APP_LOG_D("Test", "hidden"));
    assert(APP_LOG_I("Test", "value=%u", 42u));
    assert(logs.count == 1 && logs.level == APP_LOG_INFO && logs.message == "value=42" && logs.module == "Test");
    assert(app_log_set_level(APP_LOG_DEBUG));
    assert(APP_LOG_D("Test", "visible"));
    assert(!app_log_set_level(static_cast<AppLogLevel>(99)));
    assert(app_log_get_level() == APP_LOG_DEBUG);
    const std::string long_text(500, 'x');
    assert(APP_LOG_W("Test", "%s", long_text.c_str()));
    assert(logs.message.size() == 255 && logs.message.substr(252) == "...");
    assert(!app_log_write(APP_LOG_INFO, nullptr, "invalid"));
    assert(app_log_set_level(APP_LOG_OFF));
    assert(!APP_LOG_E("Test", "filtered"));
    {
        DataBroker center("CENTER");
        auto publisher = std::make_unique<DataNode>("Publisher", &center);
        DataNode duplicate("Publisher", &center);
        assert(publisher->isRegistered() && !duplicate.isRegistered());
        Received first, second, third;
        DataNode a("A", &center, &first), c("C", &center, &third);
        auto b = std::make_unique<DataNode>("B", &center, &second);
        for (auto* account : {&a, b.get(), &c}) {
            assert(account->subscribe("Publisher"));
            account->setEventCallback(receive);
        }
        first.remove = &b;
        unsigned value = 42;
        assert(publisher->publish(&value, sizeof(value)) == DataNode::RES_OK);
        assert(first.calls == 1 && second.calls == 0 && third.calls == 1);
        assert(first.value == 42 && third.value == 42);
        assert(!a.subscribe("Publisher"));
        publisher->setEventCallback(receive);
        publisher->setUserData(&second);
        assert(a.notify("Publisher", &value, sizeof(value)) == DataNode::RES_OK);
        assert(second.value == 42 && second.calls == 1);
        publisher.reset(); // Regression: publisher teardown with several subscribers.
        assert(a.getPublishersNumber() == 0 && c.getPublishersNumber() == 0);
        assert(a.notify("Publisher", &value, sizeof(value)) == DataNode::RES_NOT_FOUND);
    }
    app_log_set_sink(nullptr, nullptr);
    assert(app_log_set_level(APP_LOG_INFO));
}
