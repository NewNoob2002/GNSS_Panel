#include <memory>
#include "StatusBar.h"

#include "Resource/ResourcePool.h"
#include "Utils/BatteryStyle.h"
#include "Utils/Log/Log.h"

namespace page {

bool StatusBar::create(lv_obj_t* parent) {
    if (root_ || !parent)
        return false;
    account_ = std::make_unique<Account>("StatusBar", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !account_->Subscribe("Status")) {
        account_.reset();
        return false;
    }
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, LV_HOR_RES, 26);
    lv_obj_set_y(root_, -26);
    visible_ = false;
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

    const lv_font_t* small = resource_pool::get_font("oswaldBold_12");
    const lv_font_t* large = resource_pool::get_font("oswaldBold_18");
    const lv_font_t* symbols = resource_pool::get_font("statusbar");

    lv_obj_t* sat_icon = lv_img_create(root_);
    lv_img_set_src(sat_icon, resource_pool::get_image("satellite_small"));
    lv_obj_align(sat_icon, LV_ALIGN_TOP_LEFT, 10, 5);
    satellites_ = new NumberFlow(small, 2, true);
    satellites_->create(root_);
    satellites_->set_align_to(sat_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    position_icon_ = lv_label_create(root_);
    lv_obj_set_style_text_font(position_icon_, symbols, 0);
    lv_label_set_text(position_icon_, CUSTOM_SYMBOL_LOCATION);
    lv_obj_align_to(position_icon_, satellites_->get_cont(), LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    position_label_ = lv_label_create(root_);
    lv_obj_set_style_text_font(position_label_, small, 0);
    lv_obj_set_style_text_color(position_label_, lv_color_white(), 0);
    lv_label_set_text(position_label_, "NONE");
    lv_obj_align_to(position_label_, position_icon_, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    clock_ = new NumberFlowClock(large);
    clock_->create(root_);
    clock_->set_pos(LV_ALIGN_TOP_MID, 0, 0);

    sd_icon_ = lv_label_create(root_);
    lv_obj_set_style_text_font(sd_icon_, symbols, 0);
    lv_label_set_text(sd_icon_, CUSTOM_SYMBOL_SD_CARD);
    lv_obj_align_to(sd_icon_, clock_->get_cont(), LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    wifi_icon_ = lv_label_create(root_);
    lv_obj_set_style_text_font(wifi_icon_, symbols, 0);
    lv_label_set_text(wifi_icon_, CUSTOM_SYMBOL_WIFI);
    lv_obj_align_to(wifi_icon_, sd_icon_, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    battery_fill_ = battery::create_slot(root_, LV_HOR_RES - 61, 7, 16, 8);
    auto* battery_slot = lv_obj_get_parent(battery_fill_);
    battery_percent_ = new NumberFlow(large, 3, true);
    battery_percent_->create(root_);
    battery_percent_->set_align_to(battery_slot, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    update({});
    return true;
}

void StatusBar::set_y(void* obj, int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), static_cast<lv_coord_t>(y));
}

bool StatusBar::on_presentation(void* owner, const DataProc::StatusBarPresentation& request) {
    auto* self = static_cast<StatusBar*>(owner);
    if (!self->root_)
        return false;
    switch (request.style) {
        case DataProc::StatusBarStyle::Default:
            lv_obj_set_style_bg_opa(self->root_, LV_OPA_COVER, 0);
            break;
        case DataProc::StatusBarStyle::Transparent:
            lv_obj_set_style_bg_opa(self->root_, LV_OPA_TRANSP, 0);
            break;
        default:
            return false;
    }
    // HC32 slide behavior; reverse from the current position, never jump or restart an identical request.
    if (self->visible_ == request.visible)
        return true;
    self->visible_ = request.visible;
    lv_anim_del(self->root_, set_y);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, self->root_);
    lv_anim_set_exec_cb(&animation, set_y);
    lv_anim_set_values(&animation, lv_obj_get_y(self->root_), request.visible ? 0 : -26);
    lv_anim_set_time(&animation, 500);
    lv_anim_set_path_cb(&animation, request.visible ? lv_anim_path_ease_out : lv_anim_path_overshoot);
    if (!lv_anim_start(&animation)) {
        set_y(self->root_, request.visible ? 0 : -26);
        APP_LOG_E("StatusBar", "slide allocation failed; applied endpoint without animation");
    }
    return true;
}

int StatusBar::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<StatusBar*>(account->UserData);
    if (!event)
        return Account::RES_PARAM_ERROR;
    if (event->event == Account::EVENT_NOTIFY) {
        DataProc::StatusBarPresentation request;
        const int result = DataProc::ReadPayload(event, request);
        if (result != Account::RES_OK)
            return result;
        return on_presentation(self, request) ? Account::RES_OK : Account::RES_PARAM_ERROR;
    }
    if (event->event != Account::EVENT_PUB_PUBLISH || !event->tran || std::strcmp(event->tran->ID, "Status") != 0)
        return Account::RES_UNSUPPORTED_REQUEST;
    DataProc::StatusSnapshot snapshot;
    const int result = DataProc::ReadPayload(event, snapshot);
    if (result != Account::RES_OK)
        return result;
    StatusBarState state;
    state.position = snapshot.position.data();
    state.position_color = lv_color_hex(snapshot.position_color_rgb);
    state.satellites = snapshot.satellites;
    state.satellites_valid = snapshot.satellites_valid;
    state.battery_percent = snapshot.battery_percent;
    state.battery_valid = snapshot.battery_valid;
    state.charging = snapshot.charging;
    state.battery_voltage = snapshot.battery_voltage;
    state.hour = snapshot.hour;
    state.minute = snapshot.minute;
    state.second = snapshot.second;
    state.clock_valid = snapshot.clock_valid;
    state.wifi = snapshot.wifi;
    state.recording = snapshot.recording;
    self->update(state);
    return Account::RES_OK;
}

void StatusBar::update(const StatusBarState& state) {
    if (state.satellites_valid)
        satellites_->set_value(state.satellites > 99 ? 99 : state.satellites);
    if (state.satellites_valid)
        lv_obj_clear_flag(satellites_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(satellites_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(position_label_, state.position ? state.position : "DEMO");
    lv_obj_set_style_text_color(position_icon_, state.position_color, 0);
    if (state.clock_valid)
        clock_->set_time(state.hour % 24, state.minute % 60, state.second % 60);
    if (state.clock_valid)
        lv_obj_clear_flag(clock_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(clock_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(sd_icon_, state.recording ? lv_palette_main(LV_PALETTE_BLUE) : lv_color_white(), 0);
    lv_obj_set_style_text_color(wifi_icon_, state.wifi ? lv_palette_main(LV_PALETTE_BLUE) : lv_color_white(), 0);
    const unsigned percent = state.battery_percent > 100 ? 100 : state.battery_percent;
    battery::update_fill(battery_fill_, percent, state.charging, state.battery_valid);
    if (state.battery_valid) {
        battery_percent_->set_value(percent);
        lv_obj_clear_flag(battery_percent_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(battery_percent_->get_cont(), LV_OBJ_FLAG_HIDDEN);
    }
}

void StatusBar::destroy() {
    account_.reset(); // Unregister callbacks before deleting any LVGL objects.
    battery::stop(battery_fill_);
    battery_fill_ = nullptr;
    if (root_)
        lv_anim_del(root_, set_y);
    visible_ = false;
    delete satellites_;
    delete battery_percent_;
    delete clock_;
    satellites_ = nullptr;
    battery_percent_ = nullptr;
    clock_ = nullptr;
    if (root_) {
        lv_obj_del(root_);
        root_ = nullptr;
    }
}

} // namespace page
