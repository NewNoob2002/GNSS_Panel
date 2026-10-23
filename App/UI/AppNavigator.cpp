#include "AppNavigator.h"

using namespace page;

AppNavigator::AppNavigator(PageManager& manager, lv_group_t* group, BatteryWarning& warning)
    : DataNode("Application", DataProc::Broker()), manager_(manager), warning_(warning), input_(manager, group, *this) {}

AppNavigator::~AppNavigator() {
    stop();
}

bool AppNavigator::init() {
    dp::SettingsState settings;
    if (!isRegistered() || !dp::Power::subscribe(*this) || !dp::Settings::subscribe(*this)
        || !dp::Button::subscribe(*this) || !dp::PageNavi::subscribe(*this)
        || dp::Settings::pull(*this, settings) != RES_OK
        || dp::PageNavi::notify(*this, {dp::PageNaviOperation::Configure, static_cast<uint16_t>(PageId::Count)}) != RES_OK)
        return false;
    language_ = settings.language;
    timer_ = lv_timer_create(on_navigation, 20, this);
    return timer_ && manager_.push(name(PageId::Startup)) == PageManager::RES_TYPE::OK;
}

void AppNavigator::stop() {
    if (timer_) {
        lv_timer_delete(timer_);
        timer_ = nullptr;
    }
}

int AppNavigator::onEvent(EventParam_t* event) {
    dp::SettingsState settings;
    if (dp::Settings::receive(event, settings) == RES_OK) {
        if (settings.language != language_) {
            language_ = settings.language;
            manager_.notifyLanguageChanged();
            warning_.apply_language();
        }
        return RES_OK;
    }
    dp::ButtonState button;
    if (dp::Button::receive(event, button) == RES_OK) {
        input_.receive(button);
        return RES_OK;
    }
    dp::PowerState power;
    if (dp::Power::receive(event, power) == RES_OK)
        return RES_OK;
    dp::PageNaviState navigation;
    return dp::PageNavi::receive(event, navigation);
}

void AppNavigator::on_navigation(lv_timer_t* timer) {
    static_cast<AppNavigator*>(lv_timer_get_user_data(timer))->navigate();
}

void AppNavigator::acknowledge(dp::PowerNavigation consumed) {
    dp::PowerCommand command;
    command.operation = dp::PowerOperation::AcknowledgeNavigation;
    command.navigation = consumed;
    (void)dp::Power::notify(*this, command);
}

void AppNavigator::navigate() {
    if (manager_.isSwitching())
        return;
    const auto current = id(input_.current_page());
    dp::PowerState power;
    if (dp::Power::pull(*this, power) != RES_OK)
        return;
    if (power.navigation.standby) {
        if (manager_.resetRoot(name(PageId::Startup)) == PageManager::RES_TYPE::OK) {
            acknowledge({true, false, false});
            input_.cancel_input();
        }
        return;
    }
    if (power.navigation.shutdown) {
        if (current == PageId::SaveConfig) {
            acknowledge({false, true, false});
        } else if (manager_.push(name(PageId::SaveConfig)) == PageManager::RES_TYPE::OK) {
            acknowledge({false, true, false});
            input_.cancel_input();
        }
        return;
    }
    if (power.navigation.power_on) {
        if (current != PageId::Startup
            || manager_.replace(name(PageId::SystemLoading)) == PageManager::RES_TYPE::OK)
            acknowledge({false, false, true});
        return;
    }
    if (input_.run_pending())
        return;
    dp::PageNaviState navigation;
    if (dp::PageNavi::pull(*this, navigation) == RES_OK && navigation.pending) {
        /** @brief 执行前消费，页面生命周期的重入请求只会追加到下一节拍。 */
        (void)dp::PageNavi::notify(*this, {dp::PageNaviOperation::Consume});
        if (execute(navigation.request))
            input_.cancel_keys();
        return;
    }
    if (current == PageId::SystemLoading)
        preload_next();
    else
        preload_index_ = 0;
}

bool AppNavigator::execute(const dp::PageNaviCommand& command) {
    const auto* route = name(static_cast<PageId>(command.page));
    using Op = dp::PageNaviOperation;
    using Result = PageManager::RES_TYPE;
    switch (command.operation) {
    case Op::Push: return manager_.push(route) == Result::OK;
    case Op::Pop: return manager_.pop() == Result::OK;
    case Op::Replace: return manager_.replace(route) == Result::OK;
    case Op::ResetRoot: return manager_.resetRoot(route) == Result::OK;
    case Op::Preload: return manager_.preload(route) == Result::OK;
    default: return false;
    }
}

void AppNavigator::preload_next() {
    /** @brief 96 KiB 池仅预建紧随启动的两页；已加载页可跳过，每节拍最多新建一页。 */
    static constexpr PageId pages[] = {PageId::Dialplate, PageId::SaveConfig};
    while (preload_index_ < sizeof(pages) / sizeof(pages[0])) {
        if (manager_.preload(name(pages[preload_index_++])) == PageManager::RES_TYPE::OK)
            return;
    }
}

bool AppNavigator::show_page(PageId target) {
    if (target == PageId::Invalid)
        return false;
    input_.cancel_keys();
    return id(input_.current_page()) == target || manager_.push(name(target)) == PageManager::RES_TYPE::OK;
}
