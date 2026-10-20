#include "ButtonGesture.h"

void ButtonGesture::cancel(bool pressed) {
    raw_ = stable_ = pressed;
    suppressed_ = pressed;
    pending_click_ = double_armed_ = false;
}
ButtonGesture::Action ButtonGesture::sample(bool pressed, std::uint64_t now_ms) {
    constexpr std::uint64_t debounce_ms = 20, double_click_ms = 250;
    if (pressed != raw_) {
        raw_ = pressed;
        changed_ms_ = now_ms;
    }
    if (raw_ != stable_ && now_ms - changed_ms_ >= debounce_ms) {
        stable_ = raw_;
        if (suppressed_) {
            if (!stable_)
                suppressed_ = false;
            return Action::None;
        }
        if (stable_) {
            double_armed_ = pending_click_ && changed_ms_ - first_release_ms_ <= double_click_ms;
        } else {
            if (double_armed_) {
                pending_click_ = double_armed_ = false;
                return Action::Double;
            }
            pending_click_ = true;
            first_release_ms_ = now_ms;
        }
    }
    // A second press begun within the window stays double, even if debounce/release crosses its end.
    if (pending_click_ && !double_armed_ && now_ms - first_release_ms_ > double_click_ms
        && (!raw_ || changed_ms_ - first_release_ms_ > double_click_ms)) {
        pending_click_ = false;
        return Action::Single;
    }
    return Action::None;
}
