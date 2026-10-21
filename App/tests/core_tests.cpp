#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
#include <array>
#include <new>
#include <cstdio>
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "Frameworks/DataBroker/DataTimer.h"

namespace {
uint32_t tick = 0;
uint32_t clock_ms() {
    return tick;
}
void count_timer(DataTimer* timer) {
    ++*static_cast<unsigned*>(timer->getUserData());
}
struct Removal {
    unsigned calls = 0;
    std::unique_ptr<DataTimer>* self;
    std::unique_ptr<DataTimer>* other;
};
void remove_timers(DataTimer* timer) {
    auto& state = *static_cast<Removal*>(timer->getUserData());
    ++state.calls;
    state.other->reset();
    state.self->reset();
}
int count_account_timer(DataNode* account, DataNode::EventParam_t* event) {
    assert(event->event == DataNode::EVENT_TIMER && !event->data_p);
    ++*static_cast<unsigned*>(account->getUserData());
    return DataNode::RES_OK;
}

/** 单独验证节点注册、同步分发、销毁和入口校验，不依赖业务服务。 */
void test_broker() {
    DataBroker broker("BROKER");
    assert(broker.handleTimer() == UINT32_MAX);
    DataNode publisher("Publisher", &broker);
    DataNode duplicate("Publisher", &broker), empty("", &broker), null_id(nullptr, &broker), no_broker("NoBroker", nullptr);
    assert(publisher.isRegistered() && !duplicate.isRegistered() && !empty.isRegistered()
           && !null_id.isRegistered() && !no_broker.isRegistered());
    assert(!duplicate.subscribe("Publisher") && !publisher.subscribe(nullptr) && !publisher.subscribe("")
           && !publisher.subscribe("Publisher") && !publisher.subscribe("Missing"));
    unsigned value = 42, calls = 0;
    assert(publisher.publish(&value, sizeof(value)) == DataNode::RES_OK);
    assert(publisher.publish(nullptr, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(publisher.publish(&value, 0) == DataNode::RES_PARAM_ERROR);
    assert(duplicate.publish(&value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    DataNode sub("Subscriber", &broker);
    assert(sub.subscribe("Publisher") && !sub.subscribe("Publisher"));
    assert(!sub.unsubscribe(nullptr) && !sub.unsubscribe("") && !sub.unsubscribe("Missing"));
    assert(sub.pull(static_cast<const char*>(nullptr), &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull("", &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull("Publisher", nullptr, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull("Publisher", &value, 0) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull("Missing", &value, sizeof(value)) == DataNode::RES_NOT_FOUND);
    assert(sub.pull(static_cast<const DataNode*>(nullptr), &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull(&publisher, nullptr, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull(&publisher, &value, 0) == DataNode::RES_PARAM_ERROR);
    assert(sub.pull(&sub, &value, sizeof(value)) == DataNode::RES_NOT_FOUND);
    assert(sub.notify(static_cast<const char*>(nullptr), &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify("", &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify("Publisher", nullptr, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify("Publisher", &value, 0) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify("Missing", &value, sizeof(value)) == DataNode::RES_NOT_FOUND);
    assert(sub.notify(static_cast<const DataNode*>(nullptr), &value, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify(&publisher, nullptr, sizeof(value)) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify(&publisher, &value, 0) == DataNode::RES_PARAM_ERROR);
    assert(sub.notify(&sub, &value, sizeof(value)) == DataNode::RES_NOT_FOUND);
    assert(!broker.search(nullptr) && !broker.search("") && broker.search("Publisher") == &publisher);
    publisher.setUserData(&calls);
    publisher.setEventCallback([](DataNode* n, DataNode::EventParam_t* e) -> int {
        ++*static_cast<unsigned*>(n->getUserData());
        assert(e->size == sizeof(unsigned));
        unsigned payload;
        std::memcpy(&payload, e->data_p, sizeof(payload));
        assert(payload == 42);
        return DataNode::RES_OK;
    });
    assert(sub.pull(&publisher, &value, sizeof(value)) == DataNode::RES_OK);
    assert(sub.notify(&publisher, &value, sizeof(value)) == DataNode::RES_OK && calls == 2);
    assert(sub.unsubscribe("Publisher") && sub.getPublishersNumber() == 0);
    // 删除后续订阅者并在相同地址重建，地址相等也不能接收本次旧快照。
    struct Recreate {
        DataBroker* broker;
        DataNode* victim;
        unsigned old_calls = 0, new_calls = 0;
    } state{&broker, nullptr};
    alignas(DataNode) std::array<unsigned char, sizeof(DataNode)> storage{};
    DataNode first("First", &broker, &state), last("Last", &broker, &calls);
    state.victim = new(storage.data()) DataNode("Victim", &broker, &state);
    first.setEventCallback([](DataNode* n, DataNode::EventParam_t*) -> int {
        auto& st = *static_cast<Recreate*>(n->getUserData());
        auto* address = st.victim;
        address->~DataNode();
        st.victim = new(address) DataNode("Victim", st.broker, &st);
        st.victim->setEventCallback([](DataNode* node, DataNode::EventParam_t*) -> int {
            ++static_cast<Recreate*>(node->getUserData())->new_calls;
            return DataNode::RES_OK;
        });
        assert(st.victim->subscribe("Publisher"));
        return DataNode::RES_OK;
    });
    state.victim->setEventCallback([](DataNode* n, DataNode::EventParam_t*) -> int {
        ++static_cast<Recreate*>(n->getUserData())->old_calls;
        return DataNode::RES_OK;
    });
    last.setEventCallback([](DataNode* n, DataNode::EventParam_t*) -> int {
        ++*static_cast<unsigned*>(n->getUserData());
        return DataNode::RES_OK;
    });
    assert(first.subscribe("Publisher") && state.victim->subscribe("Publisher") && last.subscribe("Publisher"));
    assert(publisher.publish(&value, sizeof(value)) == DataNode::RES_OK);
    assert(state.old_calls == 0 && state.new_calls == 0 && calls == 3);
    std::unique_ptr<DataNode> self;
    self = std::make_unique<DataNode>("Self", &broker, &self);
    self->setEventCallback([](DataNode* n, DataNode::EventParam_t*) -> int {
        static_cast<std::unique_ptr<DataNode>*>(n->getUserData())->reset();
        return DataNode::RES_OK;
    });
    assert(self->subscribe("Publisher"));
    assert(publisher.publish(&value, sizeof(value)) == DataNode::RES_OK && !self);
    state.victim->~DataNode(); // 第二次发布的 First 重新创建了 Victim。
    assert(first.unsubscribe("Publisher"));
    first.setEventCallback([](DataNode*, DataNode::EventParam_t*) -> int { return DataNode::RES_STOP_PROCESS; });
    assert(first.subscribe("Publisher"));
    assert(publisher.publish(&value, sizeof(value)) == DataNode::RES_STOP_PROCESS);
}
/** 节点数固定为 16，失败对象析构不能影响同名有效节点。 */
void test_node_capacity() {
    DataBroker broker("CAPACITY");
    std::array<std::array<char, 16>, DataBroker::MaxNodes + 1> names{};
    std::array<std::unique_ptr<DataNode>, DataBroker::MaxNodes + 1> nodes;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::snprintf(names[i].data(), names[i].size(), "Node%u", static_cast<unsigned>(i));
        nodes[i] = std::make_unique<DataNode>(names[i].data(), &broker);
        assert(nodes[i]->isRegistered() == (i < DataBroker::MaxNodes));
    }
    assert(broker.getNodeNumber() == 16);
    nodes[0].reset();
    nodes.back() = std::make_unique<DataNode>(names.back().data(), &broker);
    assert(nodes.back()->isRegistered() && broker.getNodeNumber() == 16);
    // 所有订阅关系也应能覆盖满额节点。
    for (size_t i = 1; i < 16; ++i) assert(nodes.back()->subscribe(names[i].data()));
    assert(nodes.back()->getPublishersNumber() == 15);
}
} // namespace

// This backend and the entire test link without LVGL, a screen, or platform headers.

namespace {
/** 演示/测试电源设备，保留原即时或延后确认行为。 */
class BackendPower : public system_backend::PowerObject {
public:
    /** 读取服务已有的电源报告，无硬件操作。 */
    system_backend::PowerReport power_report() override {
        const auto s = SystemService::snapshot();
        return {s.powered_on, s.shutting_down, s.standby, s.power_cause};
    }
    /** 原后端开机操作。 */
    bool request_power_on() override;
    /** 原后端关机/重启操作。 */
    bool execute_power(DataProc::PowerAction) override;
};
/** 演示/测试业务设备，保留原请求结果及快照。 */
class BackendDevice : public system_backend::BusinessObject {
public:
    /** 返回设备及服务已知的 Wi-Fi 状态。 */
    system_backend::DeviceReport device_report() override;
    /** 原后端业务请求。 */
    bool execute_device(const DataProc::DeviceRequest&) override;
    /** 原后端 Wi-Fi 请求。 */
    bool request_wifi(bool) override;
};
/** 演示/测试状态设备，提供能力与初始化检查。 */
class BackendStatus : public system_backend::StatusObject {
public:
    /** 返回已发布状态，测试可继续直接注入快照。 */
    DataProc::StatusSnapshot status_snapshot() override { return {}; }
    /** 原后端能力。 */
    system_backend::Capabilities capabilities() override;
    /** 原后端初始化元数据。 */
    const char* initialization_name(unsigned) override;
    /** 原后端步骤标题。 */
    i18n::TextId initialization_title(unsigned) override;
    /** 原后端非阻塞检查。 */
    DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t) override;
};
}
/** 三个静态设备不依赖 LVGL 或实际屏幕即可初始化。 */
DeviceManager& system_backend::devices() {
    static BackendPower power;
    static BackendDevice device;
    static BackendStatus status;
    static ::DeviceObject* list[] = {&power, &device, &status};
    static DeviceManager manager(list, 3);
    static const bool initialized = (manager.init(), true);
    (void)initialized;
    return manager;
}

system_backend::Capabilities BackendStatus::capabilities() {
    return {};
}
bool BackendPower::request_power_on() {
    return true;
}
bool BackendPower::execute_power(DataProc::PowerAction) {
    return true;
}
bool BackendDevice::request_wifi(bool) {
    return false;
}
system_backend::DeviceReport BackendDevice::device_report() {
    return {};
}
bool BackendDevice::execute_device(const DataProc::DeviceRequest&) {
    return false;
}

/** 无屏核心测试无需实际初始化器件。 */
const char* BackendStatus::initialization_name(unsigned) { return "test"; }
/** 无屏测试的默认步骤标题。 */
i18n::TextId BackendStatus::initialization_title(unsigned) { return i18n::TextId::Initializing; }
/** 核心测试只验证同步设备调用，不等待硬件。 */
DataProc::InitializationStatus BackendStatus::poll_initialization_step(unsigned, uint32_t, char*, size_t) {
    return DataProc::InitializationStatus::Ok;
}

/** 三类设备共享相同的精确尺寸命令表，并按值访问未对齐缓冲区。 */
void test_device_contract() {
    auto& manager = system_backend::devices();
    auto* power = manager.getDevice("Power");
    auto* device = manager.getDevice("Device");
    auto* status = manager.getDevice("Status");
    assert(power && device && status && !manager.getDevice("Missing"));
    ::DeviceObject uninitialized("Uninitialized");
    unsigned value = 0;
    assert(uninitialized.read(&value, sizeof(value)) == ::DeviceObject::RES_INIT_FAILED);
    assert(uninitialized.write(&value, sizeof(value)) == ::DeviceObject::RES_INIT_FAILED);
    assert(uninitialized.ioctl(0) == ::DeviceObject::RES_INIT_FAILED);
    system_backend::Capabilities caps{};
    assert(status->read(nullptr, 0) == ::DeviceObject::RES_PARAM_ERROR);
    assert(status->write(nullptr, 0) == ::DeviceObject::RES_PARAM_ERROR);
    assert(status->ioctl(system_backend::kCapabilities, &caps, sizeof(caps) - 1) == ::DeviceObject::RES_PARAM_ERROR);
    assert(status->ioctl(system_backend::kCapabilities, nullptr, sizeof(caps)) == ::DeviceObject::RES_PARAM_ERROR);
    assert(status->ioctl(system_backend::kCapabilities ^ (1U << 30), &caps, sizeof(caps)) == ::DeviceObject::RES_UNSUPPORT);
    assert(status->ioctl(system_backend::kCapabilities, &caps, sizeof(caps)) == ::DeviceObject::RES_OK);
    assert(power->ioctl(system_backend::kSetWifi, &value, sizeof(bool)) == ::DeviceObject::RES_UNSUPPORT);
    assert(device->ioctl(system_backend::kRequestPowerOn) == ::DeviceObject::RES_UNSUPPORT);
    std::array<unsigned char, sizeof(system_backend::DeviceReport) + 1> unaligned{};
    assert(device->read(unaligned.data() + 1, sizeof(system_backend::DeviceReport)) == ::DeviceObject::RES_OK);
    assert(device->read(unaligned.data() + 1, sizeof(system_backend::DeviceReport) - 1) == ::DeviceObject::RES_PARAM_ERROR);
    DataProc::DeviceRequest request{};
    request.radio_protocol = DataProc::kRadioProtocolCount;
    std::memcpy(unaligned.data() + 1, &request, sizeof(request));
    assert(device->write(unaligned.data() + 1, sizeof(request)) == ::DeviceObject::RES_PARAM_ERROR);
    assert(device->write(&request, sizeof(request) - 1) == ::DeviceObject::RES_PARAM_ERROR);
    request.command = static_cast<DataProc::DeviceCommand>(99);
    assert(device->write(&request, sizeof(request)) == ::DeviceObject::RES_PARAM_ERROR);
    system_backend::InitializationInfo info;
    info.step = DataProc::kInitializationStepCount;
    assert(status->ioctl(system_backend::kInitializationInfo, &info, sizeof(info)) == ::DeviceObject::RES_PARAM_ERROR);
    info.step = 0;
    std::memcpy(unaligned.data() + 1, &info, sizeof(info));
    assert(status->ioctl(system_backend::kInitializationInfo, unaligned.data() + 1, sizeof(info)) == ::DeviceObject::RES_OK);
}

int main() {
    test_broker();
    test_node_capacity();
    test_device_contract();
    DataTimerManager manager;
    unsigned calls = 0;
    assert(!manager.setTickCallback(nullptr) && manager.getTick() == 0);
    {
        DataTimer invalid(&manager, count_timer, 10, &calls);
        assert(!invalid.isRegistered() && manager.handler() == UINT32_MAX);
    }
    assert(manager.setTickCallback(clock_ms));
    tick = UINT32_MAX - 9;
    {
        DataTimer timer(&manager, count_timer, 20, &calls);
        assert(timer.isRegistered() && manager.handler() == 20);
        assert(!manager.setTickCallback(+[]() -> uint32_t { return tick; }));
        assert(timer.remain() == 20);
        tick += 19;
        assert(manager.handler() == 1 && calls == 0);
        ++tick;
        assert(manager.handler() == 20 && calls == 1);
        tick += 100;
        assert(manager.handler() == 20 && calls == 2); // A stalled loop emits one event, no catch-up storm.
        timer.pause();
        tick += 100;
        assert(manager.handler() == UINT32_MAX && calls == 2 && timer.remain() == UINT32_MAX);
        timer.reset();
        timer.resume();
        assert(manager.handler() == 20 && calls == 2);
    }
    {
        std::unique_ptr<DataTimer> first, second;
        Removal removal{0, &first, &second};
        first = std::make_unique<DataTimer>(&manager, remove_timers, 10, &removal);
        second = std::make_unique<DataTimer>(&manager, count_timer, 10, &calls);
        tick += 10;
        assert(manager.handler() == UINT32_MAX && removal.calls == 1 && calls == 2);
    }
    {
        unsigned reset_calls = 0;
        DataTimer* later = nullptr;
        DataTimer first(
            &manager,
            [](DataTimer* timer) {
                tick += 5; // Time may advance inside an earlier callback before it resets another timer.
                (*static_cast<DataTimer**>(timer->getUserData()))->reset();
                timer->pause();
            },
            10, &later);
        DataTimer second(&manager, count_timer, 10, &reset_calls);
        later = &second;
        tick += 10;
        assert(manager.handler() == 10 && reset_calls == 0);
        tick += 10;
        assert(manager.handler() == 10 && reset_calls == 1);
    }
    {
        std::vector<std::unique_ptr<DataTimer>> timers;
        for (std::size_t i = 0; i <= DataTimerManager::kMaxTimers; ++i) {
            timers.push_back(std::make_unique<DataTimer>(&manager, count_timer, 10, &calls));
            assert(timers.back()->isRegistered() == (i < DataTimerManager::kMaxTimers));
        }
    }
    assert(manager.handler() == UINT32_MAX);

    assert(!DataProc_Init(nullptr));
    assert(DataProc_Init(clock_ms));
    assert(!DataProc_Init(clock_ms));
    {
        DataNode startup("StartupModel", DataProc::Broker());
        assert(startup.subscribe("System"));
        DataProc::SystemRequest request{};
        request.command = DataProc::SystemCommand::RequestPowerOn;
        assert(startup.notify("System", &request, sizeof(request)) == DataNode::RES_OK);
        assert(SystemService::snapshot().powered_on);
        assert(!DataProc_Deinit());
        unsigned timer_calls = 0;
        startup.setUserData(&timer_calls);
        startup.setEventCallback(count_account_timer);
        startup.setTimerPeriod(100);
        assert(DataProc_RunLoop() == 100);
        tick += 100;
        assert(DataProc_RunLoop() == 100 && timer_calls == 1);
        tick += 50;
        startup.resetTimer();
        assert(DataProc_RunLoop() == 100 && timer_calls == 1);
        tick += 99;
        assert(DataProc_RunLoop() == 1 && timer_calls == 1);
        ++tick;
        assert(DataProc_RunLoop() == 100 && timer_calls == 2);
        startup.stopTimer();
        tick += 100;
        assert(DataProc_RunLoop() == UINT32_MAX && timer_calls == 2);
        startup.resetTimer(); // 暂停时重置不应隐式恢复。
        assert(DataProc_RunLoop() == UINT32_MAX);
        startup.startTimer(100);
        assert(DataProc_RunLoop() == 100);
        tick += 100;
        assert(DataProc_RunLoop() == 100 && timer_calls == 3);
        startup.setTimerPeriod(0);
        assert(DataProc_RunLoop() == UINT32_MAX);
    }
    DataProc::StatusSnapshot status;
    std::strcpy(status.position.data(), "FIX");
    assert(StatusService::update(status));
    status.position.fill('x');
    assert(!StatusService::update(status));
    assert(DataProc_Deinit() && DataProc_RunLoop() == UINT32_MAX);
    assert(DataProc_Init(clock_ms) && DataProc_Deinit());
}
