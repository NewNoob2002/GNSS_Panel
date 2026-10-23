#include "DomainFixture.hpp"
#include "Def/DP_Button.h"
#include "Def/DP_PageNavi.h"
#include "Def/DP_Power.h"
#include <cassert>
#include <cstdio>

namespace {
uint32_t tick;
uint32_t clock_ms() { return tick; }
void advance() { tick += 20; DataProc_RunLoop(); }
struct Events {
    dp::ButtonState button{};
    unsigned buttons = 0, navigation = 0, power = 0;
};
int receive(DataNode* node, DataNode::EventParam_t* event) {
    auto& events = *static_cast<Events*>(node->getUserData());
    if (dp::Button::receive(event, events.button) == DataNode::RES_OK) {
        ++events.buttons;
        return DataNode::RES_OK;
    }
    dp::PageNaviState navigation;
    if (dp::PageNavi::receive(event, navigation) == DataNode::RES_OK) {
        ++events.navigation;
        return DataNode::RES_OK;
    }
    dp::PowerState power;
    if (dp::Power::receive(event, power) == DataNode::RES_OK) {
        ++events.power;
        return DataNode::RES_OK;
    }
    return DataNode::RES_UNSUPPORTED_REQUEST;
}
template<class Node> typename Node::State pull(DataNode& caller) {
    typename Node::State value;
    assert(Node::pull(caller, value) == DataNode::RES_OK);
    return value;
}
void buttons(DataNode& caller, Events& events) {
    using Key = dp::ButtonKey;
    using Op = dp::ButtonOperation;
    using Action = ButtonGesture::Action;
    const auto level = [&](bool down, Key key = Key::Power) {
        assert(dp::Button::notify(caller, {Op::Level, key, down}) == DataNode::RES_OK);
    };
    const auto poll = [&](uint64_t ms) {
        const auto before = events.buttons;
        assert(dp::Button::notify(caller, {Op::Poll, Key::Both, false, false, ms}) == DataNode::RES_OK);
        assert(events.buttons == before + 1);
        return events.button;
    };
    level(true);
    assert(!poll(100).power_held && !poll(119).power_held);
    assert(poll(120).power_held);
    level(false);
    assert(poll(130).power_held && poll(149).power_held);
    assert(!poll(150).power_held);
    assert(poll(400).power == Action::None);
    assert(poll(401).power == Action::Single);
    assert(poll(450).power == Action::None);

    /** @brief 回退时间钳制到最后一次采样，不能让无符号减法提前完成去抖。 */
    level(true);
    assert(!poll(500).power_held && !poll(1).power_held);
    assert(!poll(519).power_held && poll(520).power_held);
    level(false);
    assert(poll(530).power_held && poll(2).power_held);
    assert(!poll(550).power_held);
    assert(poll(801).power == Action::Single);

    level(true); poll(1000); poll(1020);
    level(false); poll(1040); poll(1060);
    level(true); poll(1080); poll(1100);
    level(false); poll(1120);
    assert(poll(1140).power == Action::Double);
    assert(poll(1500).power == Action::None);

    level(true); poll(1600); poll(1620);
    assert(dp::Button::notify(caller, {Op::Cancel, Key::Power}) == DataNode::RES_OK);
    assert(poll(1640).power_held);
    level(false); poll(1660); poll(1680);
    assert(poll(2000).power == Action::None);

    level(true, Key::Function); poll(2100); poll(2120);
    level(false, Key::Function); poll(2140); poll(2160);
    assert(dp::Button::notify(caller, {Op::Cancel, Key::Power}) == DataNode::RES_OK);
    assert(poll(2411).function == Action::Single);
    level(true); poll(2500); poll(2520);
    assert(dp::Button::notify(caller, {Op::Cancel, Key::Both, false, true}) == DataNode::RES_OK);
    const auto cleared = poll(3000);
    assert(!cleared.power_held && !cleared.function_held && cleared.power == Action::None);
    assert(dp::Button::notify(caller, {Op::Level, Key::Both}) == DataNode::RES_PARAM_ERROR);
    assert(dp::Button::notify(caller, {static_cast<Op>(99)}) == DataNode::RES_PARAM_ERROR);
    dp::ButtonCommand command;
    assert(caller.notify(dp::names::Button, &command, sizeof(command) - 1) == DataNode::RES_SIZE_MISMATCH);
}
void navigation(DataNode& caller, Events& events) {
    using Op = dp::PageNaviOperation;
    assert(dp::PageNavi::notify(caller, {Op::Push, 0}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {Op::Configure, 9}) == DataNode::RES_OK);
    assert(dp::PageNavi::notify(caller, {Op::Configure, 9}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {Op::Push, 9}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {Op::Replace, UINT16_MAX}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {Op::Pop, 1}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {Op::Consume}) == DataNode::RES_PARAM_ERROR);
    assert(dp::PageNavi::notify(caller, {static_cast<Op>(99), 0}) == DataNode::RES_PARAM_ERROR);
    assert(events.navigation == 0);
    constexpr dp::PageNaviCommand requests[] = {{Op::Push, 8}, {Op::Pop}, {Op::Replace, 2},
        {Op::ResetRoot, 6}, {Op::Preload, 0}, {Op::Push, 1}, {Op::Push, 2}, {Op::Push, 3}};
    for (const auto command : requests)
        assert(dp::PageNavi::notify(caller, command) == DataNode::RES_OK);
    assert(pull<dp::PageNavi>(caller).pending == 8);
    assert(dp::PageNavi::notify(caller, {Op::Push, 4}) == DataNode::RES_NO_DATA);
    for (const auto command : requests) {
        const auto state = pull<dp::PageNavi>(caller);
        assert(state.request.operation == command.operation && state.request.page == command.page);
        assert(dp::PageNavi::notify(caller, {Op::Consume}) == DataNode::RES_OK);
    }
    assert(pull<dp::PageNavi>(caller).pending == 0);
    dp::PageNaviCommand command;
    assert(caller.notify(dp::names::PageNavi, &command, sizeof(command) - 1) == DataNode::RES_SIZE_MISMATCH);
}
void power_edges(DataNode& caller, Events& events) {
    fixture::source.power = {true, false, false, hal::PowerCause::User};
    advance();
    assert(pull<dp::Power>(caller).navigation.power_on);
    const auto once = events.power;
    advance();
    assert(events.power == once);
    fixture::source.power.shutting_down = true;
    advance();
    assert(pull<dp::Power>(caller).navigation.shutdown);
    fixture::source.power = {false, false, true, hal::PowerCause::Host};
    advance();
    auto state = pull<dp::Power>(caller);
    assert(state.navigation.standby && !state.navigation.shutdown && !state.navigation.power_on);
    dp::PowerCommand acknowledge;
    acknowledge.operation = dp::PowerOperation::AcknowledgeNavigation;
    acknowledge.navigation.standby = true;
    assert(dp::Power::notify(caller, acknowledge) == DataNode::RES_OK);
    const auto consumed = events.power;
    advance(); advance();
    state = pull<dp::Power>(caller);
    assert(events.power == consumed && !state.navigation.standby);
    assert(dp::Power::notify(caller, {dp::PowerOperation::RequestOn}) == DataNode::RES_OK);
    assert(pull<dp::Power>(caller).navigation.power_on);
    acknowledge.navigation = {false, false, true};
    assert(dp::Power::notify(caller, acknowledge) == DataNode::RES_OK);
    advance();
    assert(!pull<dp::Power>(caller).navigation.power_on);
}
}
DeviceManager& system_backend::devices() { return fixture::devices(); }
int main() {
    fixture::source = {};
    assert(DataProc_Init(clock_ms));
    {
        Events events;
        DataNode caller("RoutingTest", DataProc::Broker(), &events);
        caller.setEventCallback(receive);
        assert(dp::Button::subscribe(caller) && dp::PageNavi::subscribe(caller) && dp::Power::subscribe(caller));
        buttons(caller, events);
        navigation(caller, events);
        power_edges(caller, events);
    }
    assert(DataProc_Deinit());
    assert(DataProc_Init(clock_ms));
    {
        DataNode caller("RebuiltRoutingTest", DataProc::Broker());
        assert(dp::PageNavi::subscribe(caller) && dp::Button::subscribe(caller));
        assert(!pull<dp::PageNavi>(caller).pending);
        assert(!pull<dp::Button>(caller).power_held);
        assert(dp::PageNavi::notify(caller, {dp::PageNaviOperation::Configure, 9}) == DataNode::RES_OK);
    }
    assert(DataProc_Deinit());
    std::puts("Button/PageNavi/Power：动作、电平、取消、时间回退、有界队列、边沿与重建通过。");
}
