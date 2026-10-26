#include "SystemLoadingView.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "UI/Common/PageStyle.h"
#include "UI/Resource/LogoParts.h"
#include "UI/Resource/DeviceNameLayout.h"
#include "oem_config.h"

using namespace page;

namespace {
constexpr int32_t kTrackWidth = 246;
constexpr lv_color_t kOrange = style::kAccent;
constexpr lv_color_t kGreen = style::kSuccess;
constexpr lv_color_t kRed = style::kDanger;
constexpr lv_color_t kGrey = style::kMuted;
constexpr lv_color_t kTrack = style::kBorder;

void set_opa(void* obj, int32_t opa) {
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(opa), 0);
}
void hide_after_fade(lv_anim_t* anim) {
    lv_obj_set_hidden(static_cast<lv_obj_t*>(anim->var), true);
}
void draw_check(lv_event_t* event) {
    auto* obj = lv_event_get_target_obj(event);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    const lv_point_t points[] = {{static_cast<int32_t>(area.x1 + 1), static_cast<int32_t>(area.y1 + 11)},
                                 {static_cast<int32_t>(area.x1 + 8), static_cast<int32_t>(area.y1 + 18)},
                                 {static_cast<int32_t>(area.x1 + 24), static_cast<int32_t>(area.y1 + 2)}};
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    lv_obj_init_draw_line_dsc(obj, LV_PART_MAIN, &line);
    auto* layer = lv_event_get_layer(event);
    for (unsigned i = 0; i < 2; ++i) {
        line.p1 = {static_cast<lv_value_precise_t>(points[i].x), static_cast<lv_value_precise_t>(points[i].y)};
        line.p2 = {static_cast<lv_value_precise_t>(points[i + 1].x), static_cast<lv_value_precise_t>(points[i + 1].y)};
        lv_draw_line(layer, &line);
    }
}
void rotate_gear(void* obj, int32_t angle) {
    lv_image_set_rotation(static_cast<lv_obj_t*>(obj), angle);
}
// Checks take a variable time, so the gear turns until Ready instead of spanning a fixed duration.
void spin_gear(lv_obj_t* gear) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, gear);
    lv_anim_set_values(&anim, 0, 3600);
    lv_anim_set_duration(&anim, 1600);
    lv_anim_set_exec_cb(&anim, rotate_gear);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    if (!lv_anim_start(&anim))
        LV_LOG_WARN("SystemLoading: gear animation allocation failed");
}
void animate(lv_obj_t* obj, lv_anim_exec_xcb_t exec, int32_t from, int32_t to, uint32_t time_ms,
             lv_anim_completed_cb_t finished = nullptr) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_values(&anim, from, to);
    lv_anim_set_duration(&anim, time_ms);
    lv_anim_set_exec_cb(&anim, exec);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_set_completed_cb(&anim, finished);
    if (!lv_anim_start(&anim)) {
        exec(obj, to);
        if (finished)
            finished(&anim);
        LV_LOG_ERROR("SystemLoading: animation allocation failed; applied endpoint");
    }
}
} // namespace

void SystemLoadingView::create(lv_obj_t* root, const std::array<i18n::TextId, kStepCount>& titles) {
    titles_ = titles;
    lv_obj_set_style_bg_color(root, style::kBackground, 0);
    ui_.cont = style::box(root, 8, 2, 278, 122);
    lv_obj_set_style_bg_color(ui_.cont, style::kBackground, 0);
    lv_obj_set_style_bg_opa(ui_.cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ui_.cont, 10, 0);
    lv_obj_set_user_data(ui_.cont, this);

    ui_.ring = lv_arc_create(ui_.cont);
    lv_obj_remove_style_all(ui_.ring);
    lv_obj_set_pos(ui_.ring, 119, 4);
    lv_obj_set_size(ui_.ring, 40, 40);
    lv_obj_set_clickable(ui_.ring, false); lv_obj_set_scrollable(ui_.ring, false);
    lv_arc_set_bg_angles(ui_.ring, 0, 360);
    lv_arc_set_rotation(ui_.ring, 270);
    lv_arc_set_range(ui_.ring, 0, 100);
    lv_obj_set_style_arc_width(ui_.ring, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui_.ring, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ui_.ring, kTrack, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(ui_.ring, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(ui_.ring, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ui_.ring, LV_OPA_COVER, LV_PART_INDICATOR);
    ui_.gear = lv_image_create(ui_.cont);
    lv_image_set_src(ui_.gear, resource_pool::get_image("settings"));
    lv_obj_set_pos(ui_.gear, 131, 16);
    lv_image_set_scale(ui_.gear, 384);
    lv_obj_set_style_image_recolor(ui_.gear, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(ui_.gear, LV_OPA_COVER, 0);
    ui_.check = style::box(ui_.cont, 126, 13, 26, 23);
    lv_obj_add_event_cb(ui_.check, draw_check, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_set_style_line_color(ui_.check, kGreen, 0);
    lv_obj_set_style_line_width(ui_.check, 3, 0);
    lv_obj_set_style_line_rounded(ui_.check, true, 0);

    const auto* title_font = resource_pool::get_font("oswaldBold_18");
    const auto* small_font = resource_pool::get_font("oswaldBold_12");
    auto* demo = style::mark_demo(style::label(ui_.cont, small_font, 220, 5, 44));
    lv_label_set_text(demo, "DEMO"); // Ready only describes this preview, never hardware readiness.
    lv_obj_set_style_text_color(demo, kGrey, 0);
    lv_obj_set_style_text_align(demo, LV_TEXT_ALIGN_RIGHT, 0);
    ui_.service = style::label(ui_.cont, title_font, 16, 40, kTrackWidth);
    lv_obj_set_style_text_align(ui_.service, LV_TEXT_ALIGN_CENTER, 0);
    ui_.subtitle = style::label(ui_.cont, small_font, 16, 66, kTrackWidth);
    lv_obj_set_style_text_align(ui_.subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui_.subtitle, kGrey, 0);
    auto* track = style::box(ui_.cont, 16, 83, kTrackWidth, 8);
    lv_obj_set_style_bg_color(track, kTrack, 0);
    lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(track, 4, 0);
    ui_.bar = style::box(track, 0, 0, 0, 8);
    lv_obj_set_style_bg_opa(ui_.bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ui_.bar, 4, 0);
    lv_obj_set_user_data(ui_.bar, this);
    ui_.step = style::label(ui_.cont, small_font, 16, 91, 128);
    ui_.percent = style::label(ui_.cont, title_font, 216, 91, 48);
    lv_obj_set_style_text_align(ui_.percent, LV_TEXT_ALIGN_RIGHT, 0);
    for (int i = 0; i < 5; ++i) {
        ui_.dots[i] = style::box(ui_.cont, 139 + i * 18, 112, 8, 8);
        lv_obj_set_style_radius(ui_.dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(ui_.dots[i], 1, 0);
        lv_obj_set_style_bg_opa(ui_.dots[i], LV_OPA_COVER, 0);
        if (i < 4) {
            ui_.links[i] = style::box(ui_.cont, 147 + i * 18, 115, 10, 1);
            lv_obj_set_style_bg_opa(ui_.links[i], LV_OPA_COVER, 0);
        }
    }
    ui_.footer = style::label(ui_.cont, small_font, 16, 105, OEM_LOADING_FOOTER_WIDTH);
    lv_obj_set_style_text_color(ui_.footer, lv_color_hex(OEM_FOOTER_COLOR), 0);
    const auto* logo_parts = static_cast<const logo_parts_t*>(resource_pool::get_image("oemLogoParts"));
    logo_.create(root, *logo_parts);
    ui_.logo_container = logo_.container();
    update_text();
    prepare_logo();
}

void SystemLoadingView::prepare_logo() {
    lv_obj_set_style_bg_color(lv_obj_get_parent(ui_.logo_container), lv_color_black(), 0);
    lv_obj_set_hidden(ui_.cont, true);
    lv_obj_set_hidden(ui_.logo_container, true);
}

void SystemLoadingView::show_logo() {
    prepare_logo();
    logo_.play();
}

void SystemLoadingView::show_initialization() {
    logo_.stop();
    lv_obj_set_style_bg_color(lv_obj_get_parent(ui_.logo_container), style::kBackground, 0);
    ready_ = false;
    step_ = 1;
    waited_s_ = 0;
    failed_steps_ = 0;
    lv_obj_set_hidden(ui_.cont, false);
    lv_obj_set_hidden(ui_.check, true);
    lv_obj_set_hidden(ui_.gear, false);
    set_opa(ui_.gear, LV_OPA_COVER);
    set_opa(ui_.service, LV_OPA_COVER);
    set_opa(ui_.subtitle, LV_OPA_COVER);
    for (auto* dot : ui_.dots)
        set_opa(dot, LV_OPA_COVER); // Cached re-entry must not inherit a cancelled fade.
    set_accent(kOrange);
    update_progress(0);
    update_text();
    animate(ui_.logo_container, set_opa, LV_OPA_COVER, LV_OPA_TRANSP, kTransitionMs, hide_after_fade);
    animate(ui_.cont, set_opa, LV_OPA_TRANSP, LV_OPA_COVER, kTransitionMs);
    spin_gear(ui_.gear);
}

void SystemLoadingView::begin_step(unsigned step) {
    if (step >= kStepCount) {
        LV_LOG_ERROR("SystemLoading: invalid step");
        return;
    }
    step_ = static_cast<int>(step + 1);
    waited_s_ = 0;
    failed_steps_ &= ~(1u << step);
    update_text();
    set_accent(kOrange);
    update_progress(progress_); // Active node and step label.
    lv_obj_fade_in(ui_.service, 160, 0);
    lv_obj_fade_in(ui_.subtitle, 160, 0);
    lv_obj_fade_in(ui_.dots[step], 160, 0);
}

void SystemLoadingView::finish_step(unsigned step, bool success) {
    if (step >= kStepCount) {
        LV_LOG_ERROR("SystemLoading: invalid step");
        return;
    }
    if (!success)
        failed_steps_ |= 1u << step;
    update_text();
    set_accent(success ? kOrange : kRed);
    // Only a finished check advances the bar; progress never invents an initialization result.
    animate(ui_.bar, set_progress, progress_, static_cast<int32_t>((step + 1) * 100 / kStepCount), kProgressMs);
}

void SystemLoadingView::show_waiting(uint32_t elapsed_ms) {
    const uint32_t seconds = elapsed_ms / 1000U;
    if (seconds == waited_s_)
        return;
    waited_s_ = seconds;
    update_progress(progress_);
}

void SystemLoadingView::show_ready() {
    lv_anim_delete(ui_.bar, set_progress);
    lv_anim_delete(ui_.gear, rotate_gear);
    ready_ = true;
    step_ = 5;
    lv_obj_set_hidden(ui_.check, false);
    update_progress(100);
    update_text();
    animate(ui_.cont, set_ready_mix, 0, LV_OPA_COVER, kTransitionMs);
    lv_obj_fade_in(ui_.service, kTransitionMs, 0);
    lv_obj_fade_in(ui_.subtitle, kTransitionMs, 0);
}

void SystemLoadingView::set_progress(void* obj, int32_t percent) {
    auto* view = static_cast<SystemLoadingView*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    view->update_progress(percent);
}
void SystemLoadingView::set_ready_mix(void* obj, int32_t mix) {
    auto* view = static_cast<SystemLoadingView*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    const auto color = lv_color_mix(kGreen, kOrange, static_cast<uint8_t>(mix));
    view->set_accent(view->has_failures() ? kOrange : color);
    lv_obj_set_style_line_color(view->ui_.check, view->has_failures() ? kOrange : kGreen, 0);
    set_opa(view->ui_.check, mix);
    set_opa(view->ui_.gear, LV_OPA_COVER - mix);
    for (int i = 0; i < 5; ++i) {
        const auto node_color = view->failed_steps_ & (1u << i) ? kRed : color;
        lv_obj_set_style_bg_color(view->ui_.dots[i], node_color, 0);
        lv_obj_set_style_border_color(view->ui_.dots[i], node_color, 0);
        if (i < 4)
            lv_obj_set_style_bg_color(view->ui_.links[i], node_color, 0);
    }
}
void SystemLoadingView::update_progress(int32_t percent) {
    progress_ = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    lv_obj_set_width(ui_.bar, static_cast<int32_t>(progress_ * kTrackWidth / 100));
    lv_arc_set_value(ui_.ring, static_cast<int16_t>(progress_));
    lv_label_set_text_fmt(ui_.percent, "%ld%%", static_cast<long>(progress_));
    update_step_label();
    for (int i = 0; i < 5; ++i) {
        const bool complete = ready_ || i < step_ - 1;
        const bool active = !ready_ && i == step_ - 1;
        const bool failed = failed_steps_ & (1u << i);
        const auto color = failed ? kRed : ready_ ? kGreen : kOrange;
        lv_obj_set_style_bg_color(ui_.dots[i], complete || failed ? color : kTrack, 0);
        lv_obj_set_style_border_color(ui_.dots[i], complete || active || failed ? color : kGrey, 0);
        lv_obj_set_style_border_width(ui_.dots[i], active ? 2 : 1, 0);
        if (i < 4)
            lv_obj_set_style_bg_color(ui_.links[i], complete ? color : kTrack, 0);
    }
}

void SystemLoadingView::set_accent(lv_color_t color) {
    lv_obj_set_style_arc_color(ui_.ring, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui_.bar, color, 0);
}
void SystemLoadingView::update_text() const {
    const bool failed = failed_steps_ & (1u << (step_ - 1));
    const auto headline = ready_ ? (has_failures() ? i18n::TextId::ReadyWithWarnings : i18n::TextId::SystemReady)
                                 : titles_[step_ - 1];
    const auto subtitle = ready_   ? (has_failures() ? i18n::TextId::ServicesLimited : i18n::TextId::ServicesAvailable)
                          : failed ? i18n::TextId::InitializationFailedContinue
                                   : i18n::TextId::PreparingModules;
    lv_label_set_text(ui_.service, i18n::text(headline));
    lv_obj_set_style_text_color(ui_.service,
                                ready_   ? (has_failures() ? kOrange : kGreen)
                                : failed ? kRed
                                         : style::kText,
                                0);
    lv_label_set_text(ui_.subtitle, i18n::text(subtitle));
    lv_label_set_text(ui_.footer, i18n::text(i18n::TextId::SystemLoadingTitle));
}
void SystemLoadingView::destroy() {
    logo_.stop();
    // Includes fades and callbacks borrowing View/widget pointers; cancel before the root is destroyed.
    for (auto* obj : {ui_.cont, ui_.bar, ui_.gear, ui_.check, ui_.logo_container, ui_.service, ui_.subtitle})
        lv_anim_delete(obj, nullptr);
    for (auto* dot : ui_.dots)
        lv_anim_delete(dot, nullptr);
}
void SystemLoadingView::apply_language() const {
    update_text();
    update_step_label();
}
void SystemLoadingView::update_step_label() const {
    if (ready_) {
        lv_label_set_text(ui_.step, i18n::text(i18n::TextId::StartupCompleted));
        return;
    }
    char text[48];
    std::snprintf(text, sizeof(text), i18n::text(i18n::TextId::InitializationStep), step_);
    if (waited_s_ > 0) {
        const auto length = std::strlen(text);
        std::snprintf(text + length, sizeof(text) - length, "  %lus", static_cast<unsigned long>(waited_s_));
    }
    lv_label_set_text(ui_.step, text);
}
