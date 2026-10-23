#pragma once
#include "Node.h"
#include <tuple>
namespace dp {
/** @brief 过渡聚合节点仅订阅领域值；字段级比较避免填充字节导致误发布。 */
template<class State, class... Sources> class Aggregate : public ValueNode<State> {
public:
    Aggregate(const char* name, DataBroker* broker) : ValueNode<State>(name, broker) {}
    bool init() override {
        const bool ok = (watch<Sources>() && ...);
        if (ok) this->state_ = compose();
        return this->isRegistered() && ok;
    }
    int onEvent(DataNode::EventParam_t* event) override {
        if (!event || event->event != DataNode::EVENT_PUBLISH) return ValueNode<State>::onEvent(event);
        int result = DataNode::RES_UNSUPPORTED_REQUEST;
        (receive<Sources>(event, result), ...);
        if (result == DataNode::RES_OK) this->commit(compose());
        return result;
    }
protected:
    template<class Source> const typename Source::State& get() const { return std::get<typename Source::State>(values_); }
    virtual State compose() const = 0;
private:
    std::tuple<typename Sources::State...> values_{};
    template<class Source> bool watch() {
        return Source::subscribe(*this) && Source::pull(*this, std::get<typename Source::State>(values_)) == DataNode::RES_OK;
    }
    template<class Source> void receive(DataNode::EventParam_t* event, int& result) {
        const int received = Source::receive(event, std::get<typename Source::State>(values_));
        if (received != DataNode::RES_UNSUPPORTED_REQUEST) result = received;
    }
};
}
