#pragma once

#include "UI/BatteryWarning/BatteryWarning.h"
#include "UI/StatusBar/StatusBar.h"
#include "PageManager.h"

#include <memory>

namespace page { class AppNavigator; }

/** @brief 应用上下文：组装服务与 UI，管理平台公开接口及销毁顺序。 */
class App {
  public:
    App();
    /** Product options; defaults match the RK3506 build. */
    struct Options {
        bool battery_warning = false; // Over-temperature banner (needs battery_temperature in the status).
    };
    bool init() {
        return init(Options{});
    }
    bool init(const Options& options);
    ~App();
    /** Run application-service timers before the LVGL handler on the same thread. */
    void update();
    enum class InputAction { None, NextFocus, PreviousFocus, Confirm, Back, Press, Release, Commit };
    enum class Key { Power, Function };
    /** Normalized two-button input; Key names are logical roles, not keyboard letters or evdev codes.
     * GPIO adapters call on_key(role, level == active_level); the test board maps V+/V- here.
     * No GPIO line or polarity is assumed. Call on the UI thread.
     */
    void on_key(Key key, bool pressed);
    /** Queued transition with its own monotonic timestamp (e.g. evdev). Samples the previous level up to
     * event_ms, applies the new level and samples again, so a press and release drained together still
     * produce a click. Call in event order; then poll_keys() with the current time.
     */
    void on_key_at(Key key, bool pressed, std::uint64_t event_ms);
    /** Process sampled key states and page Power holds (Startup, power confirmation) with monotonic milliseconds on
     * the UI thread.
     * Earlier timestamps than a previous poll are treated as that poll's time.
     */
    void poll_keys(std::uint64_t now_ms);
    /** Cancel held controls and partial gestures on device/window focus loss; never confirm on cancellation. */
    void cancel_input();
    /** Direct host actions: no production debounce or double-click timing. */
    void on_input(InputAction action);
    /** 显示已注册的页面，供无按键的静态画面预览使用。 */
    bool show_page(const char* name);
    bool back();
    const char* current_page() const;
    lv_obj_t* focused() const;

  private:
    lv_group_t* group_ = nullptr;
    lv_style_t root_style_{};
    bool style_ready_ = false;
    bool data_ready_ = false;
    std::unique_ptr<page::AppNavigator> navigator_;
    page::StatusBar status_bar_;
    page::BatteryWarning battery_warning_;
    std::unique_ptr<PageManager> manager_;
};
