/*
 * MIT License
 * Copyright (c) 2021 - 2024 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "DataTimer.h"
#include <algorithm>
#include <limits>

DataTimer::DataTimer(DataTimerManager* manager, TimerFunction_t callback, uint32_t period, void* user_data)
    : manager_(manager), callback_(callback), user_data_(user_data), period_(period),
      last_tick_(manager ? manager->getTick() : 0) {
    if (manager && callback && period) {
        serial_ = manager_->add(this);
    }
}

DataTimer::~DataTimer() {
    if (isRegistered()) {
        manager_->remove(this);
    }
}

void
DataTimer::reset() {
    last_tick_ = manager_ ? manager_->getTick() : 0;
}

bool
DataTimerManager::setTickCallback(TickFunction_t clock) {
    if (!clock || (clock_ != clock && std::any_of(entries_.begin(), entries_.end(), [](const Entry& entry) {
                       return entry.timer != nullptr;
                   }))) {
        return false;
    }
    clock_ = clock;
    return true;
}

uint64_t
DataTimerManager::add(DataTimer* timer) {
    if (!clock_ || next_serial_ == std::numeric_limits<uint64_t>::max()) {
        return 0;
    }
    for (auto& entry : entries_) {
        if (!entry.timer) {
            entry = {timer, ++next_serial_};
            return entry.serial;
        }
    }
    return 0;
}

void
DataTimerManager::remove(DataTimer* timer) {
    for (auto& entry : entries_) {
        if (entry.timer == timer) {
            entry = {};
            return;
        }
    }
}

bool
DataTimerManager::alive(const Entry& entry) const {
    return entry.timer && std::any_of(entries_.begin(), entries_.end(), [&](const Entry& live) {
               return live.timer == entry.timer && live.serial == entry.serial;
           });
}

uint32_t
DataTimerManager::handler() {
    if (!clock_) {
        return UINT32_MAX;
    }
    if (handling_) {
        return 0; // 嵌套分发延后到下一轮应用循环。
    }
    handling_ = true;
    const auto snapshot = entries_;
    for (const auto& entry : snapshot) {
        if (!alive(entry)) {
            continue;
        }
        auto* timer = entry.timer;
        const uint32_t tick = getTick();
        if (!timer->paused_ && tick - timer->last_tick_ >= timer->period_) {
            timer->last_tick_ = tick;
            timer->callback_(timer); // 回调可能删除自身，此后不可访问 timer。
        }
    }
    uint32_t next = UINT32_MAX;
    const uint32_t after_callbacks = getTick();
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

/** 零周期暂停，避免忙循环；恢复时仍由调用者决定是否重置时间。 */
void
DataTimer::setPeriod(uint32_t period) {
    if (period) {
        period_ = period;
    } else {
        pause();
    }
}

/** 无符号差值支持 tick 回绕；暂停定时器不参与下次唤醒。 */
uint32_t
DataTimer::remain() const {
    if (!isRegistered() || paused_) {
        return UINT32_MAX;
    }
    const auto elapsed = manager_->getTick() - last_tick_;
    return elapsed >= period_ ? 0U : period_ - elapsed;
}
