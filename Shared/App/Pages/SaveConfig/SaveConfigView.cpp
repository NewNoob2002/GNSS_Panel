#include "SaveConfigView.h"
#include "Utils/SettingsStyle.h"

using namespace page;
using i18n::TextId;

void SaveConfigView::create(lv_obj_t* root) {
    const auto* small = resource_pool::get_font("oswaldBold_12");
    const auto* large = resource_pool::get_font("oswaldBold_18");
    ring_ = style::ring(root, 14, 34, 84, 5, style::kDanger);
    lv_arc_set_range(ring_, 0, 1000);
    icon_ = settings_style::icon(ring_, "ui_power", 34, 14, style::kDanger);
    seconds_ = style::label(ring_, resource_pool::get_font("rajdhaniBold_40"), 10, 40, 64);
    lv_obj_set_style_text_align(seconds_, LV_TEXT_ALIGN_CENTER, 0);
    shutdown_icon_ = settings_style::icon(ring_, "ui_power_large", 26, 26, style::kDanger);
    settings_style::set_visible(shutdown_icon_, false);
    lv_obj_set_user_data(ring_, this);
    set_progress(ring_, 0);

    column_ = style::box(root, 114, 28, 174, 96);
    lv_obj_set_flex_flow(column_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(column_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(column_, 2, 0);
    action_ = style::label(column_, small, 0, 0, 174);
    headline_ = style::label(column_, large, 0, 0, 174);
    lv_obj_set_style_text_line_space(headline_, -4, 0);
    subtitle_ = style::label(column_, small, 0, 0, 174);
    lv_obj_set_style_text_color(subtitle_, style::kMuted, 0);
    lv_obj_set_style_pad_top(subtitle_, 2, 0);

    lv_anim_init(&progress_animation_);
    lv_anim_set_var(&progress_animation_, ring_);
    lv_anim_set_exec_cb(&progress_animation_, set_progress);
    lv_anim_set_values(&progress_animation_, 0, 1000);
    lv_anim_set_time(&progress_animation_, 8000);
    lv_anim_set_path_cb(&progress_animation_, lv_anim_path_linear);
}

void SaveConfigView::stop() const {
    lv_anim_del(ring_, set_progress);
    lv_anim_del(ring_, rotate_spinner);
    for (auto* obj : {ring_, column_})
        lv_anim_del(obj, settings_style::move_in);
}

void SaveConfigView::appear() {
    stop();
    shutting_down_ = false;
    lv_arc_set_rotation(ring_, 270);
    lv_arc_set_angles(ring_, 0, 0);
    set_progress(ring_, 0);
    settings_style::set_visible(icon_, true);
    settings_style::set_visible(seconds_, true);
    settings_style::set_visible(shutdown_icon_, false);
    settings_style::reveal(ring_);
    settings_style::reveal(column_, 40);
}

void SaveConfigView::update(const DataProc::SystemState& state) {
    if (state.shutting_down && !shutting_down_) {
        shutting_down_ = true;
        lv_anim_del(ring_, set_progress);
        lv_arc_set_angles(ring_, 0, 90);
        settings_style::set_visible(icon_, false);
        settings_style::set_visible(seconds_, false);
        settings_style::set_visible(shutdown_icon_, true);
        lv_anim_t animation;
        lv_anim_init(&animation);
        lv_anim_set_var(&animation, ring_);
        lv_anim_set_exec_cb(&animation, rotate_spinner);
        lv_anim_set_values(&animation, 270, 630);
        lv_anim_set_time(&animation, 1200);
        lv_anim_set_path_cb(&animation, lv_anim_path_linear);
        lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&animation);
    }
    const bool restart = !shutting_down_ && state.power_action == DataProc::PowerAction::Reboot;
    const auto color = restart ? style::kAccent : style::kDanger;
    if (lv_obj_get_style_arc_color(ring_, LV_PART_INDICATOR).full != color.full)
        lv_obj_set_style_arc_color(ring_, color, LV_PART_INDICATOR);
    settings_style::set_recolor(icon_, color);
    settings_style::set_text_color(action_, color);
    settings_style::set_text(action_,
                             i18n::text(restart ? TextId::SaveConfigRestarting : TextId::SaveConfigPoweringOff));
    auto headline = TextId::SaveConfigWarning;
    if (shutting_down_) {
        headline = state.power_cause == DataProc::PowerCause::Host         ? TextId::SaveConfigPowerOffBoard
                   : state.power_cause == DataProc::PowerCause::LowBattery ? TextId::SaveConfigPowerOffLowBattery
                   : state.power_cause == DataProc::PowerCause::User       ? TextId::SaveConfigPowerOffPushKey
                                                                           : TextId::SaveConfigPowerOffForced;
    }
    settings_style::set_text(headline_, i18n::text(headline));
    settings_style::set_text(subtitle_,
                             i18n::text(state.power_pending && !shutting_down_ ? TextId::SaveConfigCancelHint
                                                                               : TextId::SaveConfigKeepPower));
}

void SaveConfigView::set_progress(void* obj, int32_t progress) {
    auto* view = static_cast<SaveConfigView*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    progress = progress < 0 ? 0 : progress > 1000 ? 1000 : progress;
    if (lv_arc_get_value(view->ring_) != progress)
        lv_arc_set_value(view->ring_, static_cast<int16_t>(progress));
    const auto seconds = ((1000 - progress) * 8 + 999) / 1000;
    char text[2];
    lv_snprintf(text, sizeof(text), "%ld", static_cast<long>(seconds));
    settings_style::set_text(view->seconds_, text);
}

void SaveConfigView::rotate_spinner(void* obj, int32_t angle) {
    lv_arc_set_rotation(static_cast<lv_obj_t*>(obj), static_cast<uint16_t>(angle % 360));
}
