#include "DomainFixture.hpp"
#include "hal_device_checks.h"
#include "fixed_format_tests.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
#include <array>
#include <new>
#include <cstdio>
#include "SystemBackend.h"
#include "SystemService.h"
#include "DataTimer.h"

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
/** 节点数固定为 32，失败对象析构不能影响同名有效节点。 */
void test_node_capacity() {
    DataBroker broker("CAPACITY");
    std::array<std::array<char, 16>, DataBroker::MaxNodes + 1> names{};
    std::array<std::unique_ptr<DataNode>, DataBroker::MaxNodes + 1> nodes;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::snprintf(names[i].data(), names[i].size(), "Node%u", static_cast<unsigned>(i));
        nodes[i] = std::make_unique<DataNode>(names[i].data(), &broker);
        assert(nodes[i]->isRegistered() == (i < DataBroker::MaxNodes));
    }
    assert(broker.getNodeNumber() == DataBroker::MaxNodes);
    nodes[0].reset();
    nodes.back() = std::make_unique<DataNode>(names.back().data(), &broker);
    assert(nodes.back()->isRegistered() && broker.getNodeNumber() == DataBroker::MaxNodes);
    // 所有订阅关系也应能覆盖满额节点。
    for (size_t i = 1; i < DataBroker::MaxNodes; ++i) assert(nodes.back()->subscribe(names[i].data()));
    assert(nodes.back()->getPublishersNumber() == DataBroker::MaxNodes - 1);
}
} // namespace

// 无屏测试使用九个可变细设备，全部值由 DataProc 定时器读取。
DeviceManager& system_backend::devices() { return fixture::devices(); }
void test_device_contract() {
    hal_test::shapes();
    hal_test::equality();
    assert(!system_backend::devices().getDevice("Device") && !system_backend::devices().getDevice("Status"));
    DeviceObject uninitialized("Uninitialized");
    unsigned value = 0;
    assert(uninitialized.read(&value,sizeof(value)) == DeviceObject::RES_INIT_FAILED);
    assert(uninitialized.write(&value,sizeof(value)) == DeviceObject::RES_INIT_FAILED);
    assert(uninitialized.ioctl(0) == DeviceObject::RES_INIT_FAILED);
    std::array<unsigned char,sizeof(hal::WorkState)+1> bytes{};
    auto* work = system_backend::devices().getDevice(hal::names::Work);
    assert(work->read(bytes.data()+1,sizeof(hal::WorkState)) == DeviceObject::RES_OK);
    assert(work->read(bytes.data()+1,sizeof(hal::WorkState)-1) == DeviceObject::RES_PARAM_ERROR);
    hal::WorkCommand invalid{hal::WorkTarget::Rover,hal::kRadioProtocolCount,1};
    std::memcpy(bytes.data()+1,&invalid,sizeof(invalid));
    assert(work->write(bytes.data()+1,sizeof(invalid)) == DeviceObject::RES_PARAM_ERROR);
    std::array<unsigned char,sizeof(hal::InitInfo)+1> information{};
    hal::InitInfo info;
    std::memcpy(information.data()+1,&info,sizeof(info));
    assert(system_backend::devices().getDevice(hal::names::Init)->ioctl(hal::kInitializationInfo,
        information.data()+1,sizeof(info)) == DeviceObject::RES_OK);
}

int main() {
    test_fixed_format();
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
        DataBroker timer_broker("TimerBroker");
        assert(timer_broker.initTimerManager(clock_ms));
        DataNode timed("Timed", &timer_broker);
        unsigned timer_calls = 0;
        timed.setUserData(&timer_calls);
        timed.setEventCallback(count_account_timer);
        timed.setTimerPeriod(100);
        assert(timer_broker.handleTimer() == 100);
        tick += 100;
        assert(timer_broker.handleTimer() == 100 && timer_calls == 1);
        tick += 50;
        timed.resetTimer();
        assert(timer_broker.handleTimer() == 100 && timer_calls == 1);
        tick += 99;
        assert(timer_broker.handleTimer() == 1 && timer_calls == 1);
        ++tick;
        assert(timer_broker.handleTimer() == 100 && timer_calls == 2);
        timed.stopTimer();
        tick += 100;
        assert(timer_broker.handleTimer() == UINT32_MAX && timer_calls == 2);
        timed.resetTimer(); // 暂停时重置不应隐式恢复。
        assert(timer_broker.handleTimer() == UINT32_MAX);
        timed.startTimer(100);
        assert(timer_broker.handleTimer() == 100);
        tick += 100;
        assert(timer_broker.handleTimer() == 100 && timer_calls == 3);
        timed.setTimerPeriod(0);
        assert(timer_broker.handleTimer() == UINT32_MAX);
    }
    assert(DataProc_Deinit() && DataProc_RunLoop() == UINT32_MAX);
    assert(DataProc_Init(clock_ms) && DataProc_Deinit());
}
