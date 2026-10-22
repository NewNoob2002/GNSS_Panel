#include "Utils/lv_ext/lv_label_format.h"
#include "BatteryWarning.h"
#include "Resource/ResourcePool.h"
#include "I18n.h"
#include "Log.h"

namespace page {
namespace {
constexpr int32_t kStatusBarHeight = 26;
constexpr int32_t kBannerHeight = 28;
constexpr int32_t kBannerY = kStatusBarHeight + 1;
constexpr int32_t kShineWidth = 42;

void set_bg_opa(void* obj, int32_t value) {
    lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(value), 0);
}
void set_text_opa(void* obj, int32_t value) {
    lv_obj_set_style_text_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(value), 0);
}
void set_x(void* obj, int32_t value) {
    lv_obj_set_x(static_cast<lv_obj_t*>(obj), static_cast<int32_t>(value));
}
void set_y(void* obj, int32_t value) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), static_cast<int32_t>(value));
}
void start(lv_obj_t* obj, lv_anim_exec_xcb_t exec, int32_t from, int32_t to, uint32_t time, uint32_t playback,
           uint32_t repeat_delay, lv_anim_path_cb_t path, bool repeat) {
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, obj);
    lv_anim_set_exec_cb(&animation, exec);
    lv_anim_set_values(&animation, from, to);
    lv_anim_set_duration(&animation, time);
    lv_anim_set_reverse_duration(&animation, playback);
    lv_anim_set_repeat_delay(&animation, repeat_delay);
    lv_anim_set_repeat_count(&animation, repeat ? LV_ANIM_REPEAT_INFINITE : 1);
    lv_anim_set_path_cb(&animation, path);
    if (!lv_anim_start(&animation))
        APP_LOG_W("BatteryWarning", "effect allocation failed; banner stays static");
}
} // namespace

bool BatteryWarning::create(lv_obj_t* parent) {
    if (banner_ || !parent)
        return false;
    account_ = std::make_unique<DataNode>("BatteryWarning", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !account_->subscribe("Status")) {
        account_.reset();
        return false;
    }
    banner_ = lv_obj_create(parent);
    lv_obj_remove_style_all(banner_);
    lv_obj_set_size(banner_, LV_HOR_RES - 8, kBannerHeight);
    lv_obj_set_pos(banner_, 4, kBannerY);
    lv_obj_set_clickable(banner_, false); lv_obj_set_scrollable(banner_, false);
    lv_obj_set_style_bg_color(banner_, lv_color_hex(0x8F241E), 0);
    lv_obj_set_style_bg_grad_color(banner_, lv_color_hex(0xD9583E), 0);
    lv_obj_set_style_bg_grad_dir(banner_, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(banner_, LV_OPA_90, 0);
    lv_obj_set_style_border_color(banner_, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_border_width(banner_, 1, 0);
    lv_obj_set_style_radius(banner_, 3, 0);
    lv_obj_set_style_shadow_color(banner_, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_shadow_width(banner_, 5, 0);
    lv_obj_set_style_shadow_opa(banner_, LV_OPA_40, 0);

    marker_ = lv_label_create(banner_);
    lv_obj_remove_style_all(marker_);
    lv_label_set_text(marker_, "!");
    lv_obj_set_style_text_color(marker_, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_text_font(marker_, resource_pool::get_font("oswaldBold_18"), 0);
    lv_obj_align(marker_, LV_ALIGN_LEFT_MID, 8, -1);

    label_ = lv_label_create(banner_);
    lv_obj_remove_style_all(label_);
    lv_obj_set_width(label_, LV_HOR_RES - 48);
    lv_label_set_long_mode(label_, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_, resource_pool::get_font("oswaldBold_12"), 0);
    lv_obj_align(label_, LV_ALIGN_CENTER, 9, 0);

    shine_ = lv_obj_create(banner_);
    lv_obj_remove_style_all(shine_);
    lv_obj_set_size(shine_, kShineWidth, 2);
    lv_obj_set_pos(shine_, -kShineWidth, kBannerHeight - 3);
    lv_obj_set_style_bg_color(shine_, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_bg_grad_color(shine_, lv_color_white(), 0);
    lv_obj_set_style_bg_grad_dir(shine_, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(shine_, LV_OPA_80, 0);

    lv_obj_set_hidden(banner_, true);
    DataProc::StatusSnapshot status;
    if (account_->pull("Status", &status, sizeof(status)) == DataNode::RES_OK)
        update(status.battery_temperature_valid, status.battery_temperature);
    return true;
}

void BatteryWarning::destroy() {
    account_.reset(); // Unregister before deleting the objects its callback touches.
    if (banner_) {
        stop_effects();
        lv_obj_delete(banner_);
    }
    banner_ = label_ = marker_ = shine_ = nullptr;
    active_ = false;
}

void BatteryWarning::apply_language() {
    if (active_)
        update_text();
}

int BatteryWarning::on_event(DataNode* account, DataNode::EventParam_t* event) {
    if (!event || event->event != DataNode::EVENT_PUBLISH)
        return DataNode::RES_UNSUPPORTED_REQUEST;
    DataProc::StatusSnapshot status;
    const int result = DataProc::ReadPayload(event, status);
    if (result == DataNode::RES_OK)
        static_cast<BatteryWarning*>(account->getUserData())
            ->update(status.battery_temperature_valid, status.battery_temperature);
    return result;
}

void BatteryWarning::update(bool valid, float temperature) {
    if (!banner_)
        return;
    // Hysteresis: stays visible until the temperature drops below the clear threshold.
    const bool active = valid && temperature >= (active_ ? kClearC : kCriticalC);
    temperature_ = temperature;
    if (active == active_) {
        if (active_)
            update_text();
        return;
    }
    active_ = active;
    if (active_) {
        update_text();
        lv_obj_set_hidden(banner_, false);
        lv_obj_move_to_index(banner_, -1);
        start_effects();
    } else {
        stop_effects();
        lv_obj_set_hidden(banner_, true);
    }
}

void BatteryWarning::update_text() {
    app_label_set_text_fmt(label_, i18n::text(i18n::TextId::BatteryTemperatureCritical),
                          static_cast<double>(temperature_));
}

void BatteryWarning::start_effects() {
    start(banner_, set_bg_opa, LV_OPA_80, LV_OPA_COVER, 650, 650, 0, lv_anim_path_ease_in_out, true);
    start(marker_, set_text_opa, LV_OPA_50, LV_OPA_COVER, 350, 350, 0, lv_anim_path_ease_in_out, true);
    start(shine_, set_x, -kShineWidth, LV_HOR_RES, 1400, 0, 500, lv_anim_path_linear, true);
    start(banner_, set_y, kStatusBarHeight - kBannerHeight, kBannerY, 220, 0, 0, lv_anim_path_ease_out, false);
}

void BatteryWarning::stop_effects() {
    lv_anim_delete(banner_, set_bg_opa);
    lv_anim_delete(banner_, set_y);
    lv_anim_delete(marker_, set_text_opa);
    lv_anim_delete(shine_, set_x);
    lv_obj_set_y(banner_, kBannerY);
    lv_obj_set_style_bg_opa(banner_, LV_OPA_90, 0);
    lv_obj_set_style_text_opa(marker_, LV_OPA_COVER, 0);
    lv_obj_set_x(shine_, -kShineWidth);
}
} // namespace page
