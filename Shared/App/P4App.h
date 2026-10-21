#pragma once

#include "ButtonGesture.h"
#include "Pages/BatteryWarning/BatteryWarning.h"
#include "Pages/StatusBar/StatusBar.h"
#include "Utils/PageManager/PageManager.h"

#include <memory>

/** P4 页面组合：业务状态尚未迁入，状态栏只显示调用方传入的数据。 */
class P4App {
  public:
    /** Product options; defaults match the RK3506 build. */
    struct Options {
        bool battery_warning = false; // Over-temperature banner (needs battery_temperature in the status).
    };
    bool init() {
        return init(Options{});
    }
    bool init(const Options& options);
    ~P4App();
    /** Run application-service timers before the LVGL handler on the same thread. */
    void update();
    void update_status(const page::StatusBarState& state);
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
    i18n::Language language_{};
    std::unique_ptr<Account> account_;
    static int on_system_event(Account* account, Account::EventParam_t* event);
    // Platform power events (System publications) are applied by navigation_timer_, outside the Account
    // callback chain: a page must not be unloaded while its own Notify is still running.
    static void on_navigation(lv_timer_t* timer);
    lv_timer_t* navigation_timer_ = nullptr;
    bool powered_on_ = false, shutting_down_ = false, standby_ = false;
    bool pending_power_on_ = false, pending_shutdown_ = false, pending_standby_ = false;
    page::StatusBar status_bar_;
    page::BatteryWarning battery_warning_;
    std::unique_ptr<PageManager> manager_;
    bool power_down_ = false, function_down_ = false;
    ButtonGesture power_gesture_, function_gesture_;
    bool power_hold_ = false; // The current page took the raw Power hold at the last poll.
    unsigned preload_index_ = 0;
    void preload_next();
    std::uint64_t last_poll_ms_ = 0;
    const char* key_page_ = nullptr;
    void cancel_keys();
    void dispatch_input(InputAction action);
    lv_obj_t* pressed_ = nullptr; // 按键按住期间的焦点；页面切换后绝不解引用
    const char* pressed_page_ = nullptr;
};
