#include "App.h"
#include "Log.h"
#include "UI/AppFactory.h"
#include "UI/AppNavigator.h"
#include "UI/Common/PageStyle.h"
#include "UI/Resource/ResourcePool.h"

App::App() = default;

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

    /** @brief 页面过渡时根控件溢出屏幕，屏幕自身不应显示滚动条。 */
    lv_obj_set_scrollable(lv_screen_active(), false);
    lv_obj_set_scrollbar_mode(lv_screen_active(), LV_SCROLLBAR_MODE_OFF);
    manager_ = std::make_unique<PageManager>();
    manager_->setRootDefaultStyle(&root_style_);
    manager_->setGlobalLoadAnim(PAGE_ANIM::MOVE_LEFT, page::style::kPageTransitionMs, lv_anim_path_ease_out);
    manager_->setScrimOpa(page::style::kPageScrimOpa);
    if (!AppFactory::installAll(*manager_) || !status_bar_.create(lv_layer_top()))
        return false;
    if (options.battery_warning && !battery_warning_.create(lv_layer_top()))
        return false;
    navigator_ = std::make_unique<page::AppNavigator>(*manager_, group_, battery_warning_);
    return navigator_->init();
}

void App::update() {
    (void)DataProc_RunLoop();
}

App::~App() {
    if (navigator_)
        navigator_->stop();
    /** @brief InputRouter 的删除通知地址必须存活到页面控件全部销毁。 */
    manager_.reset();
    navigator_.reset();
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

void App::on_input(InputAction action) {
    if (navigator_)
        navigator_->input().on_input(action);
}

void App::cancel_input() {
    if (navigator_)
        navigator_->input().cancel_input();
}

void App::on_key(Key key, bool pressed) {
    if (navigator_)
        navigator_->input().on_key(key, pressed);
}

void App::on_key_at(Key key, bool pressed, std::uint64_t event_ms) {
    if (navigator_)
        navigator_->input().on_key_at(key, pressed, event_ms);
}

void App::poll_keys(std::uint64_t now_ms) {
    if (navigator_)
        navigator_->input().poll_keys(now_ms);
}

const char* App::current_page() const {
    return navigator_ ? navigator_->input().current_page() : nullptr;
}

bool App::show_page(const char* name) {
    return navigator_ && navigator_->show_page(page::id(name));
}

bool App::back() {
    return navigator_ && navigator_->input().back();
}

lv_obj_t* App::focused() const {
    return navigator_ ? navigator_->input().focused() : nullptr;
}
