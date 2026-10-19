#include "BatteryWarning.h"

#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "mcu_config.h"

namespace {

constexpr float kCriticalTemperatureC = 65.0f;
constexpr float kClearTemperatureC = 60.0f;
constexpr lv_coord_t kStatusBarHeight = 26;
constexpr lv_coord_t kBannerHeight = 28;
constexpr lv_coord_t kBannerY = kStatusBarHeight + 1;
constexpr lv_coord_t kShineWidth = 42;

lv_obj_t* warningBanner = nullptr;
lv_obj_t* warningLabel = nullptr;
lv_obj_t* warningMarker = nullptr;
lv_obj_t* warningShine = nullptr;
bool warningActive = false;
float displayedTemperature = -1000.0f;

void
SetBannerOpacity(void* obj, const int32_t opacity) {
    lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), opacity, 0);
}

void
SetMarkerOpacity(void* obj, const int32_t opacity) {
    lv_obj_set_style_text_opa(static_cast<lv_obj_t*>(obj), opacity, 0);
}

void
SetShinePosition(void* obj, const int32_t x) {
    lv_obj_set_x(static_cast<lv_obj_t*>(obj), x);
}

void
SetBannerPosition(void* obj, const int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), y);
}

void
UpdateWarningText() {
    if (warningLabel == nullptr) {
        return;
    }

    displayedTemperature = systemInfo.powerMonitor.batteryInfo.Temp_f;
    lv_label_set_text_fmt(warningLabel, I18n::Text(I18n::TextId::BatteryTemperatureCritical), displayedTemperature);
}

void
StartWarningEffects() {
    lv_anim_t animation;

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, warningBanner);
    lv_anim_set_exec_cb(&animation, SetBannerOpacity);
    lv_anim_set_values(&animation, LV_OPA_80, LV_OPA_COVER);
    lv_anim_set_time(&animation, 650);
    lv_anim_set_playback_time(&animation, 650);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, warningMarker);
    lv_anim_set_exec_cb(&animation, SetMarkerOpacity);
    lv_anim_set_values(&animation, LV_OPA_50, LV_OPA_COVER);
    lv_anim_set_time(&animation, 350);
    lv_anim_set_playback_time(&animation, 350);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, warningShine);
    lv_anim_set_exec_cb(&animation, SetShinePosition);
    lv_anim_set_values(&animation, -kShineWidth, LV_HOR_RES);
    lv_anim_set_time(&animation, 1400);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&animation, 500);
    lv_anim_set_path_cb(&animation, lv_anim_path_linear);
    lv_anim_start(&animation);

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, warningBanner);
    lv_anim_set_exec_cb(&animation, SetBannerPosition);
    lv_anim_set_values(&animation, kStatusBarHeight - kBannerHeight, kBannerY);
    lv_anim_set_time(&animation, 220);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_start(&animation);
}

void
StopWarningEffects() {
    lv_anim_del(warningBanner, SetBannerOpacity);
    lv_anim_del(warningBanner, SetBannerPosition);
    lv_anim_del(warningMarker, SetMarkerOpacity);
    lv_anim_del(warningShine, SetShinePosition);

    lv_obj_set_y(warningBanner, kBannerY);
    lv_obj_set_style_bg_opa(warningBanner, LV_OPA_90, 0);
    lv_obj_set_style_text_opa(warningMarker, LV_OPA_COVER, 0);
    lv_obj_set_x(warningShine, -kShineWidth);
}

} // namespace

lv_obj_t*
Page::BatteryWarning_Create(lv_obj_t* parent) {
    warningBanner = lv_obj_create(parent);
    lv_obj_remove_style_all(warningBanner);
    lv_obj_set_size(warningBanner, LV_HOR_RES - 8, kBannerHeight);
    lv_obj_set_pos(warningBanner, 4, kBannerY);
    lv_obj_clear_flag(warningBanner, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(warningBanner, lv_color_hex(0x8F241E), 0);
    lv_obj_set_style_bg_grad_color(warningBanner, lv_color_hex(0xD9583E), 0);
    lv_obj_set_style_bg_grad_dir(warningBanner, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(warningBanner, LV_OPA_90, 0);
    lv_obj_set_style_border_color(warningBanner, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_border_width(warningBanner, 1, 0);
    lv_obj_set_style_radius(warningBanner, 3, 0);
    lv_obj_set_style_shadow_color(warningBanner, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_shadow_width(warningBanner, 5, 0);
    lv_obj_set_style_shadow_opa(warningBanner, LV_OPA_40, 0);

    warningMarker = lv_label_create(warningBanner);
    lv_obj_remove_style_all(warningMarker);
    lv_obj_clear_flag(warningMarker, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_text(warningMarker, "!");
    lv_obj_set_style_text_color(warningMarker, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_text_font(warningMarker, ResourcePool::GetFont("oswaldBold_18"), 0);
    lv_obj_align(warningMarker, LV_ALIGN_LEFT_MID, 8, -1);

    warningLabel = lv_label_create(warningBanner);
    lv_obj_remove_style_all(warningLabel);
    lv_obj_set_width(warningLabel, LV_HOR_RES - 48);
    lv_obj_clear_flag(warningLabel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_label_set_long_mode(warningLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(warningLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(warningLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(warningLabel, ResourcePool::GetFont("oswaldBold_12"), 0);
    lv_obj_align(warningLabel, LV_ALIGN_CENTER, 9, 0);

    warningShine = lv_obj_create(warningBanner);
    lv_obj_remove_style_all(warningShine);
    lv_obj_set_size(warningShine, kShineWidth, 2);
    lv_obj_set_pos(warningShine, -kShineWidth, kBannerHeight - 3);
    lv_obj_clear_flag(warningShine, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(warningShine, lv_color_hex(0xF6D062), 0);
    lv_obj_set_style_bg_grad_color(warningShine, lv_color_white(), 0);
    lv_obj_set_style_bg_grad_dir(warningShine, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(warningShine, LV_OPA_80, 0);

    UpdateWarningText();
    lv_obj_add_flag(warningBanner, LV_OBJ_FLAG_HIDDEN);
    return warningBanner;
}

void
Page::BatteryWarning_Update() {
    if (warningBanner == nullptr) {
        return;
    }

    const float temperatureC = systemInfo.powerMonitor.batteryInfo.Temp_f;
    const bool shouldBeActive =
        warningActive ? temperatureC >= kClearTemperatureC : temperatureC >= kCriticalTemperatureC;

    if (shouldBeActive != warningActive) {
        warningActive = shouldBeActive;
        if (warningActive) {
            UpdateWarningText();
            lv_obj_clear_flag(warningBanner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(warningBanner);
            StartWarningEffects();
        } else {
            StopWarningEffects();
            lv_obj_add_flag(warningBanner, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    if (warningActive && temperatureC != displayedTemperature) {
        UpdateWarningText();
    }
}

void
Page::BatteryWarning_ApplyLanguage() {
    UpdateWarningText();
}
