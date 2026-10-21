#pragma once
#include <cstdint>

/** Single/double clicks are exclusive. Poll with monotonic milliseconds, on the LVGL thread. */
class ButtonGesture {
  public:
    enum class Action { None, Single, Double };
    Action sample(bool pressed, std::uint64_t now_ms);
    /** Debounced held state; a cancelled key stays inactive until released. */
    bool pressed() const {
        return stable_ && !suppressed_;
    }
    /** Debounced key level, including a key already held when gestures were cancelled (e.g. the Power key that
     * powered the board on). For holds, which must count from the actual press; clicks still use pressed().
     */
    bool held() const {
        return stable_;
    }
    /** Discard pending gestures; a held key must release before another click is accepted. */
    void cancel(bool pressed);

  private:
    bool raw_ = false, stable_ = false, pending_click_ = false, double_armed_ = false, suppressed_ = false;
    std::uint64_t changed_ms_ = 0, first_release_ms_ = 0;
};
