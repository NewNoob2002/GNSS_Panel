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
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

class DataTimerManager;

/** 应用线程独占；管理器比定时器长寿，回调允许删除任意定时器。 */
class DataTimer {
public:
  using TimerFunction_t = void (*)(DataTimer*);
  /** 注入毫秒周期与借用的用户数据；无时钟、空回调、零周期或满额时注册失败。 */
  DataTimer(DataTimerManager* manager, TimerFunction_t callback, uint32_t period, void* user_data = nullptr);
  /** 从管理器注销，不释放用户数据。 */
  ~DataTimer();
  DataTimer(const DataTimer&) = delete;
  DataTimer& operator=(const DataTimer&) = delete;

  /** 查询是否成功占用定长注册表。 */
  bool
  isRegistered() const {
      return serial_ != 0;
  }

  /** 返回调用方拥有的用户数据，不延长其生命周期。 */
  void*
  getUserData() const {
      return user_data_;
  }

  /** 以当前 tick 重置起点，不改变暂停状态。 */
  void reset();
  /** 设置非零周期；零值暂停，保留之前的周期。 */
  void setPeriod(uint32_t period);

  /** 返回当前周期，单位毫秒。 */
  uint32_t
  getPeriod() const {
      return period_;
  }

  /** 返回剩余毫秒；暂停或未注册时返回 UINT32_MAX。 */
  uint32_t remain() const;

  /** 暂停到期分发。 */
  void
  pause() {
      paused_ = true;
  }

  /** 恢复分发，不重置时间；如需重新计时先调用 reset。 */
  void
  resume() {
      paused_ = false;
  }

private:
  friend class DataTimerManager;
  DataTimerManager* manager_;
  TimerFunction_t callback_;
  void* user_data_;
  uint32_t period_, last_tick_;
  uint64_t serial_ = 0;
  bool paused_ = false;
};

/** 移植原调度器：32 项定长表和序号快照避免回调删除导致释放后使用。 */
class DataTimerManager {
public:
  using TickFunction_t = uint32_t (*)();
  static constexpr std::size_t kMaxTimers = 32;

  /** 借用单调毫秒时钟，空时钟时不分发事件。 */
  explicit DataTimerManager(TickFunction_t clock = nullptr) : clock_(clock) {}

  DataTimerManager(const DataTimerManager&) = delete;
  DataTimerManager& operator=(const DataTimerManager&) = delete;
  /** 拒绝空时钟；存在已注册定时器时禁止切换时钟源。 */
  bool setTickCallback(TickFunction_t clock);
  /** 每个到期定时器至多调用一次；回调期间新建的定时器延到下一轮。 */
  uint32_t handler();

  /** 未设置时钟时返回零，不调用空指针。 */
  uint32_t
  getTick() const {
      return clock_ ? clock_() : 0;
  }

  /** 无符号差值支持一次 tick 回绕，单位毫秒。 */
  uint32_t
  getTickElaps(uint32_t previous) const {
      return getTick() - previous;
  }

private:
  friend class DataTimer;

  struct Entry {
      DataTimer* timer = nullptr;
      uint64_t serial = 0;
  };

  TickFunction_t clock_ = nullptr;
  std::array<Entry, kMaxTimers> entries_{};
  uint64_t next_serial_ = 0;
  bool handling_ = false;
  /** 返回非零且不重复的序号，满额或无时钟返回零。 */
  uint64_t add(DataTimer* timer);
  /** 清除匹配地址的注册项，不访问已删除对象。 */
  void remove(DataTimer* timer);
  /** 对照当前表校验地址与序号，避免原地址重建误用旧快照。 */
  bool alive(const Entry& entry) const;
};
