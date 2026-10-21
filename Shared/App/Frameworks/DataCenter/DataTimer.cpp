#include "DataTimer.h"
#include <algorithm>
#include <limits>

DataTimer::DataTimer(DataTimerManager& manager, Callback callback, uint32_t period, void* user_data)
    : manager_(manager), callback_(callback), user_data_(user_data), period_(period), last_tick_(manager.now()) {
    if (callback && period)
        serial_ = manager_.add(this);
}
DataTimer::~DataTimer() {
    if (registered())
        manager_.remove(this);
}
void DataTimer::reset() {
    last_tick_ = manager_.now();
}

bool DataTimerManager::set_clock(Clock clock) {
    if (!clock || (clock_ != clock && std::any_of(entries_.begin(), entries_.end(), [](const Entry& entry) {
                       return entry.timer != nullptr;
                   })))
        return false;
    clock_ = clock;
    return true;
}
uint64_t DataTimerManager::add(DataTimer* timer) {
    if (!clock_ || next_serial_ == std::numeric_limits<uint64_t>::max())
        return 0;
    for (auto& entry : entries_) {
        if (!entry.timer) {
            entry = {timer, ++next_serial_};
            return entry.serial;
        }
    }
    return 0;
}
void DataTimerManager::remove(DataTimer* timer) {
    for (auto& entry : entries_)
        if (entry.timer == timer) {
            entry = {};
            return;
        }
}
bool DataTimerManager::alive(const Entry& entry) const {
    return entry.timer && std::any_of(entries_.begin(), entries_.end(), [&](const Entry& live) {
               return live.timer == entry.timer && live.serial == entry.serial;
           });
}
uint32_t DataTimerManager::handle() {
    if (!clock_)
        return UINT32_MAX;
    if (handling_)
        return 0; // Nested dispatch waits for the outer application loop.
    handling_ = true;
    const auto snapshot = entries_;
    for (const auto& entry : snapshot) {
        if (!alive(entry))
            continue;
        auto* timer = entry.timer;
        const uint32_t tick = now();
        if (!timer->paused_ && tick - timer->last_tick_ >= timer->period_) {
            timer->last_tick_ = tick;
            timer->callback_(timer); // Do not access timer afterwards: the callback may destroy it.
        }
    }
    uint32_t next = UINT32_MAX;
    const uint32_t after_callbacks = now();
    for (const auto& entry : entries_) {
        const auto* timer = entry.timer;
        if (timer && !timer->paused_) {
            const uint32_t elapsed = after_callbacks - timer->last_tick_;
            next = std::min(next, elapsed >= timer->period_ ? 0U : timer->period_ - elapsed);
        }
    }
    handling_ = false;
    return next;
}
