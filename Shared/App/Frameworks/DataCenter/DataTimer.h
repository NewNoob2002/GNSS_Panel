#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class DataTimerManager;

// Single application thread; the manager outlives its timers. Callbacks may remove timers.
class DataTimer {
  public:
    using Callback = void (*)(DataTimer*);
    DataTimer(DataTimerManager& manager, Callback callback, uint32_t period, void* user_data = nullptr);
    ~DataTimer();
    DataTimer(const DataTimer&) = delete;
    DataTimer& operator=(const DataTimer&) = delete;

    bool registered() const {
        return serial_ != 0;
    }
    void* user_data() const {
        return user_data_;
    }
    void reset();
    void pause() {
        paused_ = true;
    }
    void resume() {
        paused_ = false;
    }

  private:
    friend class DataTimerManager;
    DataTimerManager& manager_;
    Callback callback_;
    void* user_data_;
    uint32_t period_, last_tick_;
    uint64_t serial_ = 0;
    bool paused_ = false;
};

// X-TRACK DataTimer model: inject a monotonic millisecond clock, independently of LVGL/OS.
class DataTimerManager {
  public:
    using Clock = uint32_t (*)();
    static constexpr std::size_t kMaxTimers = 32;
    bool set_clock(Clock clock);
    uint32_t handle();
    DataTimerManager() = default;
    DataTimerManager(const DataTimerManager&) = delete;
    DataTimerManager& operator=(const DataTimerManager&) = delete;

  private:
    friend class DataTimer;
    struct Entry {
        DataTimer* timer = nullptr;
        uint64_t serial = 0;
    };
    Clock clock_ = nullptr;
    std::array<Entry, kMaxTimers> entries_{};
    uint64_t next_serial_ = 0;
    bool handling_ = false;
    uint32_t now() const {
        return clock_ ? clock_() : 0;
    }
    uint64_t add(DataTimer* timer);
    void remove(DataTimer* timer);
    bool alive(const Entry& entry) const;
};
