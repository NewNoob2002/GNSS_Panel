#include "App.h"
#include <cstdio>
#include "SystemService.h"
#include "Log.h"

#include "Pages/Dialplate/Dialplate.h"
#include "Pages/RecordConfig/RecordConfig.h"
#include "Pages/SaveConfig/SaveConfig.h"
#include "Pages/StarMap/StarMap.h"
#include "Pages/Startup/Startup.h"
#include "Pages/SystemDash/SystemDash.h"
#include "Pages/SystemInfos/SystemInfos.h"
#include "Pages/SystemLoading/SystemLoading.h"
#include "Pages/WorkSettings/WorkSettings.h"
#include "Resource/ResourcePool.h"
#include "Utils/PageStyle.h"

#include <cstring>

bool App::init(const Options& options) {
    resource_pool::init();
    if (!DataProc_Init(lv_tick_get))
        return false;
    data_ready_ = true;
    group_ = lv_group_create();
    if (!group_)
        return false;
    lv_group_set_default(group_);
    lv_style_init(&root_style_);
    style_ready_ = true;
    lv_style_set_width(&root_style_, LV_HOR_RES);
    lv_style_set_height(&root_style_, LV_VER_RES);
    lv_style_set_bg_color(&root_style_, page::style::kBackground);
    lv_style_set_bg_opa(&root_style_, LV_OPA_COVER);
    lv_style_set_text_color(&root_style_, page::style::kText);
    lv_style_set_pad_all(&root_style_, 0);
    lv_style_set_border_width(&root_style_, 0);
    lv_style_set_radius(&root_style_, 0);

    // Sliding page roots overflow the screen during transitions; the screen itself never scrolls, so it must not draw
    // a scrollbar over every transition frame.
    lv_obj_set_scrollable(lv_screen_active(), false);
    lv_obj_set_scrollbar_mode(lv_screen_active(), LV_SCROLLBAR_MODE_OFF);

    manager_ = std::make_unique<PageManager>();
    manager_->setRootDefaultStyle(&root_style_);
    // Default forward navigation is a smooth horizontal push; pages override this for boot/modal/context changes.
    manager_->setGlobalLoadAnim(PAGE_ANIM::MOVE_LEFT, page::style::kPageTransitionMs,
                                        lv_anim_path_ease_out);
    auto register_page = [this](PageBase* page, const char* name) {
        if (manager_->registerPage(page, name) != PageManager::RES_TYPE::OK) {
            delete page;
            return false;
        }
        page->onInstalled();
        return true;
    };
    if (!register_page(new page::Dialplate, "Pages/Dialplate")
        || !register_page(new page::WorkSettings, "Pages/WorkSettings")
        || !register_page(new page::RecordConfig, "Pages/RecordConfig")
        || !register_page(new page::SystemInfos, "Pages/SystemInfos")
        || !register_page(new page::SystemDash, "Pages/SystemDash")
        || (SystemService::snapshot().star_map && !register_page(new page::StarMap, "Pages/StarMap"))
        || !register_page(new page::Startup, "Pages/Startup")
        || !register_page(new page::SystemLoading, "Pages/SystemLoading")
        || !register_page(new page::SaveConfig, "Pages/SaveConfig"))
        return false;
    if (!status_bar_.create(lv_layer_top()))
        return false;
    if (options.battery_warning && !battery_warning_.create(lv_layer_top()))
        return false;
    language_ = i18n::get_language();
    account_ = std::make_unique<DataNode>("Application", DataProc::Broker(), this);
    account_->setEventCallback(on_system_event);
    if (!account_->isRegistered() || !account_->subscribe("System"))
        return false;
    navigation_timer_ = lv_timer_create(on_navigation, 20, this);
    if (!navigation_timer_)
        return false;
    return manager_->push("Pages/Startup") == PageManager::RES_TYPE::OK;
}

void App::update() {
    (void)DataProc_RunLoop();
}

App::~App() {
    if (navigation_timer_)
        lv_timer_delete(navigation_timer_);
    manager_.reset();
    account_.reset();
    battery_warning_.destroy();
    status_bar_.destroy();
    if (data_ready_ && !DataProc_Deinit())
        APP_LOG_E("Application", "provider cleanup failed");
    if (group_) {
        lv_group_set_default(nullptr);
        lv_group_delete(group_);
    }
    if (style_ready_)
        lv_style_reset(&root_style_);
}

int App::on_system_event(DataNode* account, DataNode::EventParam_t* event) {
    if (!event || event->event != DataNode::EVENT_PUBLISH)
        return DataNode::RES_UNSUPPORTED_REQUEST;
    DataProc::SystemState state;
    const int result = DataProc::ReadPayload(event, state);
    if (result != DataNode::RES_OK)
        return result;
    auto* self = static_cast<App*>(account->getUserData());
    if (state.language != self->language_) {
        self->language_ = state.language;
        self->manager_->notifyLanguageChanged();
        self->battery_warning_.apply_language();
    }
    // Record rising edges only; on_navigation applies them once page switching is idle.
    if (state.standby && !self->standby_) {
        self->pending_standby_ = true;
        self->pending_power_on_ = self->pending_shutdown_ = false;
    }
    if (state.shutting_down && !self->shutting_down_)
        self->pending_shutdown_ = true;
    if (state.powered_on && !self->powered_on_)
        self->pending_power_on_ = true;
    self->standby_ = state.standby;
    self->shutting_down_ = state.shutting_down;
    self->powered_on_ = state.powered_on;
    return DataNode::RES_OK;
}

void App::on_navigation(lv_timer_t* timer) {
    auto* self = static_cast<App*>(lv_timer_get_user_data(timer));
    if (!self->manager_ || self->manager_->isSwitching())
        return;
    const char* page = self->current_page();
    if (self->pending_standby_) {
        // Power-off finished while the UI runs: a fresh standby root, no working history.
        if (self->manager_->resetRoot("Pages/Startup") == PageManager::RES_TYPE::OK) {
            self->pending_standby_ = false;
            self->cancel_input();
        }
        return;
    }
    if (self->pending_shutdown_) {
        if (page && std::strcmp(page, "Pages/SaveConfig") == 0) {
            self->pending_shutdown_ = false;
        } else if (self->manager_->push("Pages/SaveConfig") == PageManager::RES_TYPE::OK) {
            self->pending_shutdown_ = false;
            self->cancel_input();
        }
        return;
    }
    if (self->pending_power_on_) {
        self->pending_power_on_ = false;
        // Only the standby page advances; power-on confirmed elsewhere needs no navigation.
        if (page && std::strcmp(page, "Pages/Startup") == 0
            && self->manager_->replace("Pages/SystemLoading") != PageManager::RES_TYPE::OK)
            self->pending_power_on_ = true;
        return;
    }
    if (page && std::strcmp(page, "Pages/SystemLoading") == 0)
        self->preload_next();
    else
        self->preload_index_ = 0; // A later boot (after standby) preloads what was unloaded again.
}

void App::preload_next() {
    // While SystemLoading runs (often waiting for the host), build the pages entered right after it (home) and right
    // after a confirmed power-off (SaveConfig). Others are created on entry: the 96 KB LVGL pool cannot keep every
    // page resident (popped pages are unloaded), and PageManager keeps creation time out of the transition anyway.
    static constexpr const char* kPreload[] = {"Pages/Dialplate", "Pages/SaveConfig"};
    while (preload_index_ < sizeof(kPreload) / sizeof(kPreload[0])) {
        if (manager_->preload(kPreload[preload_index_++]) == PageManager::RES_TYPE::OK)
            return; // One page per tick; an already loaded one is skipped at once.
    }
}

void App::update_status(const page::StatusBarState& state) {
    DataProc::StatusSnapshot snapshot;
    std::snprintf(snapshot.position.data(), snapshot.position.size(), "%s", state.position ? state.position : "DEMO");
    snapshot.position_color_rgb = lv_color_to_u32(state.position_color) & 0xffffffu;
    snapshot.satellites = state.satellites;
    snapshot.satellites_valid = state.satellites_valid;
    snapshot.battery_percent = state.battery_percent;
    snapshot.battery_valid = state.battery_valid;
    snapshot.charging = state.charging;
    snapshot.battery_voltage = state.battery_voltage;
    snapshot.battery_temperature_valid = state.battery_temperature_valid;
    snapshot.battery_temperature = state.battery_temperature;
    snapshot.hour = state.hour;
    snapshot.minute = state.minute;
    snapshot.second = state.second;
    snapshot.clock_valid = state.clock_valid;
    snapshot.recording = state.recording;
    snapshot.wifi = state.wifi;
    if (!StatusService::update(snapshot))
        APP_LOG_E("Application", "status publication rejected");
}

void App::on_input(const InputAction action) {
    if (!group_ || !manager_ || manager_->isSwitching())
        return;
    cancel_keys();
    dispatch_input(action);
}

void App::dispatch_input(const InputAction action) {
    auto* current = manager_->getCurrentPage();
    if (action == InputAction::Press && pressed_ == focused() && pressed_page_ == current_page())
        return; // Auto-repeat must not restart a hold.
    if (action == InputAction::NextFocus && current->onNextRequest())
        return;
    if (action == InputAction::Confirm && current->onEnterRequest())
        return;
    if (action == InputAction::Commit) {
        current->onCommitRequest();
        return;
    }
    const char* page = current_page();
    if (action == InputAction::Release) {
        lv_obj_t* pressed = pressed_;
        const char* pressed_page = pressed_page_;
        pressed_ = nullptr;
        pressed_page_ = nullptr;
        if (pressed && page && std::strcmp(page, pressed_page) == 0 && lv_obj_is_in_widget_tree(pressed)) {
            lv_obj_set_pressed(pressed, false);
            lv_obj_send_event(pressed, LV_EVENT_RELEASED, nullptr);
            if (lv_group_get_focused(group_) == pressed && !current->onEnterRequest())
                lv_obj_send_event(pressed, LV_EVENT_SHORT_CLICKED, nullptr);
        }
        return;
    }
    if (pressed_ && action != InputAction::None) {
        if (page && std::strcmp(page, pressed_page_) == 0 && lv_obj_is_in_widget_tree(pressed_)) {
            lv_obj_set_pressed(pressed_, false);
            lv_obj_send_event(pressed_, LV_EVENT_PRESS_LOST, nullptr);
        }
        pressed_ = nullptr;
        pressed_page_ = nullptr;
    }
    if (action == InputAction::NextFocus)
        lv_group_focus_next(group_);
    else if (action == InputAction::PreviousFocus)
        lv_group_focus_prev(group_);
    else if (action == InputAction::Confirm || action == InputAction::Press) {
        if (lv_obj_t* focused = lv_group_get_focused(group_)) {
            // Uniform input dispatch; each page decides whether Press is a hold or a normal click.
            if (action == InputAction::Press && page) {
                pressed_ = focused;
                // 删除通知指向 App 成员；相同控件只保留一份通知，避免反复按下积累事件。
                for (uint32_t i = lv_obj_get_event_count(focused); i > 0; --i) {
                    auto* event = lv_obj_get_event_dsc(focused, i - 1);
                    if (lv_event_dsc_get_user_data(event) == &pressed_)
                        lv_obj_remove_event(focused, i - 1);
                }
                lv_obj_null_on_delete(&pressed_);
                pressed_page_ = page;
                lv_obj_set_pressed(focused, true);
                lv_obj_send_event(focused, LV_EVENT_PRESSED, nullptr);
            } else {
                lv_obj_send_event(focused, LV_EVENT_SHORT_CLICKED, nullptr);
            }
        }
    } else if (action == InputAction::Back) {
        back();
    }
}

void App::cancel_input() {
    power_down_ = function_down_ = false;
    cancel_keys();
    auto* pressed = pressed_;
    const auto* page = pressed_page_;
    // Pointer presses are owned by LVGL, not pressed_; reset alone does not emit PRESS_LOST.
    if (auto* focus = focused(); focus && lv_obj_is_pressed(focus)) {
        pressed = focus;
        page = current_page();
    }
    pressed_ = nullptr;
    pressed_page_ = nullptr;
    if (pressed && page && current_page() && std::strcmp(page, current_page()) == 0 && lv_obj_is_in_widget_tree(pressed)) {
        lv_obj_set_pressed(pressed, false);
        lv_obj_send_event(pressed, LV_EVENT_PRESS_LOST, nullptr);
    }
}

void App::cancel_keys() {
    power_gesture_.cancel(power_down_);
    function_gesture_.cancel(function_down_);
    key_page_ = current_page();
}
void App::on_key(Key key, bool pressed) {
    if (key == Key::Power)
        power_down_ = pressed;
    else
        function_down_ = pressed;
}
void App::on_key_at(Key key, bool pressed, std::uint64_t event_ms) {
    poll_keys(event_ms); // The previous level lasted until this transition.
    on_key(key, pressed);
    poll_keys(event_ms);
}
void App::poll_keys(std::uint64_t now_ms) {
    // ButtonGesture needs non-decreasing time; queued event stamps may precede the last loop poll.
    now_ms = now_ms < last_poll_ms_ ? last_poll_ms_ : now_ms;
    last_poll_ms_ = now_ms;
    if (!manager_ || manager_->isSwitching() || key_page_ != current_page()) {
        cancel_keys();
        return;
    }
    const auto power = power_gesture_.sample(power_down_, now_ms);
    const auto function = function_gesture_.sample(function_down_, now_ms);
    auto* current = manager_->getCurrentPage();
    const bool hold = current && current->wantsPowerHold();
    if (hold != power_hold_) {
        // Switching between hold and click handling: the press that opened or closed a hold must not also
        // complete a click (e.g. reopen the power confirmation it just dismissed).
        power_hold_ = hold;
        power_gesture_.cancel(power_down_);
        return; // power/function were sampled before the cancel; start fresh on the next poll.
    }
    if (hold) {
        // The page consumes the debounced level without cancelling it as a direct host action would. held(), not
        // pressed(): a cold boot starts with Power already held, and that hold must still count.
        dispatch_input(power_gesture_.held() ? InputAction::Press : InputAction::Release);
        // Function still moves focus / goes back, but never while Power is held (that would cancel the hold).
        if (function != ButtonGesture::Action::None && !power_gesture_.held())
            on_input(function == ButtonGesture::Action::Single ? InputAction::NextFocus : InputAction::Back);
        return;
    }
    if (power != ButtonGesture::Action::None)
        on_input(power == ButtonGesture::Action::Single ? InputAction::Confirm : InputAction::Commit);
    else if (function != ButtonGesture::Action::None)
        on_input(function == ButtonGesture::Action::Single ? InputAction::NextFocus : InputAction::Back);
}

const char* App::current_page() const {
    if (!manager_)
        return nullptr;
    PageBase* page = manager_->getCurrentPage();
    return page ? page->getName() : nullptr;
}

bool App::show_page(const char* name) {
    if (!manager_ || !name)
        return false;
    const char* current = current_page();
    cancel_keys();
    return (current && std::strcmp(current, name) == 0) || manager_->push(name) == PageManager::RES_TYPE::OK;
}

bool App::back() {
    if (!manager_ || manager_->isSwitching())
        return false;
    cancel_keys();
    if (auto* current = manager_->getCurrentPage(); current && current->onBackRequest())
        return true;
    return manager_->pop() == PageManager::RES_TYPE::OK;
}

lv_obj_t* App::focused() const {
    return group_ ? lv_group_get_focused(group_) : nullptr;
}
