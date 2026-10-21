#include <cassert>
#include <cstring>
#include <memory>
#include <vector>
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "Frameworks/DataCenter/DataTimer.h"

namespace {
uint32_t tick = 0;
uint32_t clock_ms() {
    return tick;
}
void count_timer(DataTimer* timer) {
    ++*static_cast<unsigned*>(timer->user_data());
}
struct Removal {
    unsigned calls = 0;
    std::unique_ptr<DataTimer>* self;
    std::unique_ptr<DataTimer>* other;
};
void remove_timers(DataTimer* timer) {
    auto& state = *static_cast<Removal*>(timer->user_data());
    ++state.calls;
    state.other->reset();
    state.self->reset();
}
int count_account_timer(Account* account, Account::EventParam_t* event) {
    assert(event->event == Account::EVENT_TIMER && !event->data_p);
    ++*static_cast<unsigned*>(account->UserData);
    return Account::RES_OK;
}
} // namespace

// This backend and the entire test link without LVGL, a screen, or platform headers.
system_backend::Capabilities system_backend::capabilities() {
    return {};
}
bool system_backend::request_power_on() {
    return true;
}
bool system_backend::execute_power(DataProc::PowerAction) {
    return true;
}
bool system_backend::request_wifi(bool) {
    return false;
}
DataProc::DeviceSnapshot system_backend::initial_device() {
    return {};
}
bool system_backend::execute_device(const DataProc::DeviceRequest&) {
    return false;
}

int main() {
    DataTimerManager manager;
    unsigned calls = 0;
    assert(!manager.set_clock(nullptr));
    {
        DataTimer invalid(manager, count_timer, 10, &calls);
        assert(!invalid.registered() && manager.handle() == UINT32_MAX);
    }
    assert(manager.set_clock(clock_ms));
    tick = UINT32_MAX - 9;
    {
        DataTimer timer(manager, count_timer, 20, &calls);
        assert(timer.registered() && manager.handle() == 20);
        tick += 19;
        assert(manager.handle() == 1 && calls == 0);
        ++tick;
        assert(manager.handle() == 20 && calls == 1);
        tick += 100;
        assert(manager.handle() == 20 && calls == 2); // A stalled loop emits one event, no catch-up storm.
        timer.pause();
        tick += 100;
        assert(manager.handle() == UINT32_MAX && calls == 2);
        timer.reset();
        timer.resume();
        assert(manager.handle() == 20 && calls == 2);
    }
    {
        std::unique_ptr<DataTimer> first, second;
        Removal removal{0, &first, &second};
        first = std::make_unique<DataTimer>(manager, remove_timers, 10, &removal);
        second = std::make_unique<DataTimer>(manager, count_timer, 10, &calls);
        tick += 10;
        assert(manager.handle() == UINT32_MAX && removal.calls == 1 && calls == 2);
    }
    {
        unsigned reset_calls = 0;
        DataTimer* later = nullptr;
        DataTimer first(
            manager,
            [](DataTimer* timer) {
                tick += 5; // Time may advance inside an earlier callback before it resets another timer.
                (*static_cast<DataTimer**>(timer->user_data()))->reset();
                timer->pause();
            },
            10, &later);
        DataTimer second(manager, count_timer, 10, &reset_calls);
        later = &second;
        tick += 10;
        assert(manager.handle() == 10 && reset_calls == 0);
        tick += 10;
        assert(manager.handle() == 10 && reset_calls == 1);
    }
    {
        std::vector<std::unique_ptr<DataTimer>> timers;
        for (std::size_t i = 0; i <= DataTimerManager::kMaxTimers; ++i) {
            timers.push_back(std::make_unique<DataTimer>(manager, count_timer, 10, &calls));
            assert(timers.back()->registered() == (i < DataTimerManager::kMaxTimers));
        }
    }
    assert(manager.handle() == UINT32_MAX);

    assert(!DataProc_Init(nullptr));
    assert(DataProc_Init(clock_ms));
    assert(!DataProc_Init(clock_ms));
    {
        Account startup("StartupModel", DataProc::Center());
        assert(startup.Subscribe("System"));
        DataProc::SystemRequest request{};
        request.command = DataProc::SystemCommand::RequestPowerOn;
        assert(startup.Notify("System", &request, sizeof(request)) == Account::RES_OK);
        assert(SystemService::snapshot().powered_on);
        assert(!DataProc_Deinit());
        unsigned timer_calls = 0;
        startup.UserData = &timer_calls;
        startup.SetEventCallback(count_account_timer);
        startup.SetTimerPeriod(100);
        assert(DataProc_RunLoop() == 100);
        tick += 100;
        assert(DataProc_RunLoop() == 100 && timer_calls == 1);
        startup.SetTimerEnable(false);
        tick += 100;
        assert(DataProc_RunLoop() == UINT32_MAX && timer_calls == 1);
        startup.SetTimerPeriod(0);
    }
    DataProc::StatusSnapshot status;
    std::strcpy(status.position.data(), "FIX");
    assert(StatusService::update(status));
    status.position.fill('x');
    assert(!StatusService::update(status));
    assert(DataProc_Deinit() && DataProc_RunLoop() == UINT32_MAX);
    assert(DataProc_Init(clock_ms) && DataProc_Deinit());
}
