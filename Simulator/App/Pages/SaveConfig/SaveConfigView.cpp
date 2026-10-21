#include "SaveConfigView.h"
#include "Utils/SettingsStyle.h"

using namespace page;
using i18n::TextId;

void SaveConfigView::create(lv_obj_t* root) {
    const auto* small = resource_pool::get_font("oswaldBold_12");
    const auto* large = resource_pool::get_font("oswaldBold_18");
    ring_ = style::ring(root, 14, 34, 84, 5, style::kDanger);
    icon_ = settings_style::icon(ring_, "ui_power_large", 26, 14, style::kDanger);
    percent_ = style::label(ring_, small, 10, 50, 64);
    lv_obj_set_style_text_align(percent_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(percent_, style::kMuted, 0);
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
    lv_anim_set_values(&progress_animation_, 0, 100);
    lv_anim_set_time(&progress_animation_, 8000);
}

void SaveConfigView::stop() const {
    lv_anim_del(ring_, set_progress);
    for (auto* obj : {ring_, column_})
        lv_anim_del(obj, settings_style::move_in);
}

void SaveConfigView::appear() {
    stop();
    set_progress(ring_, 0);
    settings_style::reveal(ring_);
    settings_style::reveal(column_, 40);
}

void SaveConfigView::update(const DataProc::SystemState& state) const {
    const bool restart = !state.shutting_down && state.power_action == DataProc::PowerAction::Reboot;
    const auto color = restart ? style::kAccent : style::kDanger;
    if (lv_obj_get_style_arc_color(ring_, LV_PART_INDICATOR).full != color.full)
        lv_obj_set_style_arc_color(ring_, color, LV_PART_INDICATOR);
    settings_style::set_recolor(icon_, color);
    settings_style::set_text_color(action_, color);
    settings_style::set_text(action_,
                             i18n::text(restart ? TextId::SaveConfigRestarting : TextId::SaveConfigPoweringOff));
    auto headline = TextId::SaveConfigWarning;
    if (state.shutting_down) {
        headline = state.power_cause == DataProc::PowerCause::Host         ? TextId::SaveConfigPowerOffBoard
                   : state.power_cause == DataProc::PowerCause::LowBattery ? TextId::SaveConfigPowerOffLowBattery
                   : state.power_cause == DataProc::PowerCause::User       ? TextId::SaveConfigPowerOffPushKey
                                                                           : TextId::SaveConfigPowerOffForced;
    }
    settings_style::set_text(headline_, i18n::text(headline));
    settings_style::set_text(subtitle_,
                             i18n::text(state.power_pending && !state.shutting_down ? TextId::SaveConfigCancelHint
                                                                                    : TextId::SaveConfigKeepPower));
}

void SaveConfigView::set_progress(void* obj, int32_t percent) {
    auto* view = static_cast<SaveConfigView*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    percent = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    if (lv_arc_get_value(view->ring_) != percent)
        lv_arc_set_value(view->ring_, static_cast<int16_t>(percent));
    char text[8];
    lv_snprintf(text, sizeof(text), "%ld%%", static_cast<long>(percent));
    settings_style::set_text(view->percent_, text);
}
