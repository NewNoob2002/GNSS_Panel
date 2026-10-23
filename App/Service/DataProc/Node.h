#pragma once
#include <new>
#include "Helper/NodeHelper.h"
#include "SystemBackend.h"

namespace dp {
/** @brief DataProc 独占的节点对象，反初始化时逆依赖顺序销毁。 */
class Provider : public DataNode {
public:
    Provider(const char* name, DataBroker* broker) : DataNode(name, broker) {}
    virtual bool init() { return isRegistered(); }
};
/** @brief 值节点：发布局部快照，重入修改不影响当前分发。 */
template<class State> class ValueNode : public Provider {
public:
    ValueNode(const char* name, DataBroker* broker) : Provider(name, broker) {}
    int onEvent(EventParam_t* event) override {
        if (!event) return RES_PARAM_ERROR;
        if (event->event != EVENT_PULL) return RES_UNSUPPORTED_REQUEST;
        if (!event->data_p) return RES_PARAM_ERROR;
        if (event->size != sizeof(State)) return RES_SIZE_MISMATCH;
        std::memcpy(event->data_p, &state_, sizeof(state_));
        return RES_OK;
    }
protected:
    State state_{};
    void commit(const State& next) {
        if (next == state_) return;
        state_ = next;
        const auto snapshot = state_;
        (void)publish(&snapshot, sizeof(snapshot));
    }
};
/** @brief 领域节点定时读取 HAL；失败保留上次成功值，命令后立即采样反馈 pending。 */
template<class State> class PollingNode : public ValueNode<State> {
public:
    PollingNode(const char* name, DataBroker* broker, const char* device, uint32_t period)
        : ValueNode<State>(name, broker), device_(device), period_(period) {}
    bool init() override {
        refresh();
        this->startTimer(period_);
        return this->isRegistered() && this->hasTimer();
    }
    int onEvent(DataNode::EventParam_t* event) override {
        if (event && event->event == DataNode::EVENT_TIMER) { refresh(); return DataNode::RES_OK; }
        return ValueNode<State>::onEvent(event);
    }
protected:
    const char* device_;
    virtual void refresh() {
        State next{};
        if (hal::read(system_backend::devices().getDevice(device_), next) == DeviceObject::RES_OK && valid(next)) this->commit(next);
    }
    virtual bool valid(const State&) const { return true; }
    template<class Command> int execute(const Command& command) {
        const int result = hal::write(system_backend::devices().getDevice(device_), command);
        if (result != DeviceObject::RES_OK) return DataNode::RES_UNKNOWN;
        refresh();
        return DataNode::RES_OK;
    }
private:
    uint32_t period_;
};
}

#define DP_CREATE(NAME, TYPE) dp::Provider* _DP_##NAME##_Create(DataBroker* broker) { return new (std::nothrow) TYPE(broker); }
