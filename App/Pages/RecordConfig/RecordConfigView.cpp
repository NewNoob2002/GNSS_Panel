#include "RecordConfigView.h"
#include <cstdio>
#include "FixedFormat.h"
#include "Utils/SettingsStyle.h"
using namespace page;
using i18n::TextId;

void RecordConfigView::create(lv_obj_t* root) {
    const auto* font = resource_pool::get_font("oswaldBold_12");
    hero_ = style::box(root, 8, 28, 88, 77);
    settings_style::hero(hero_);
    settings_style::icon(hero_, "ui_record_large", 28, 6);
    state_ = style::label(hero_, font, 4, 40, 80);
    space_ = style::label(hero_, font, 4, 58, 80);
    lv_obj_set_style_text_color(space_, style::kMuted, 0);
    for (auto* label : {state_, space_})
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    status_dot_ = settings_style::dot(hero_, 73, 8);
    ui.type = style::box(root, 102, 28, 186, 23, true);
    ui.interval = style::box(root, 102, 55, 186, 23, true);
    ui.record = style::box(root, 102, 82, 89, 23, true);
    ui.back = style::box(root, 199, 82, 89, 23, true);
    for (auto* control : controls())
        style::control(control);
    settings_style::icon(ui.type, "ui_record", 8, 3, style::kInfo);
    settings_style::icon(ui.interval, "ui_clock", 8, 3, style::kInfo);
    record_icon_ = settings_style::icon(ui.record, "ui_play", 8, 3);
    settings_style::icon(ui.back, "ui_back", 8, 3);
    type_text_ = style::label(ui.type, font, 32, 2, 146);
    interval_text_ = style::label(ui.interval, font, 32, 2, 146);
    record_text_ = style::label(ui.record, font, 32, 2, 49);
    back_text_ = style::label(ui.back, font, 32, 2, 49);
    feedback_ = style::label(root, font, 8, 108, 278);
    lv_obj_set_style_text_align(feedback_, LV_TEXT_ALIGN_CENTER, 0);
}
void RecordConfigView::appear() const {
    settings_style::reveal(hero_);
    settings_style::reveal(ui.type, 40);
    settings_style::reveal(ui.interval, 80);
    settings_style::reveal(ui.record, 120);
    settings_style::reveal(ui.back, 160);
}
void RecordConfigView::stop() const {
    settings_style::stop(status_dot_);
}
void RecordConfigView::set_selection(unsigned type, unsigned interval) {
    if (type < DataProc::kRecordTypeCount)
        type_ = type;
    if (interval < DataProc::kRecordIntervalCount)
        interval_ = interval;
}
void RecordConfigView::update(const DataProc::DeviceSnapshot& device, bool configurable, TextId feedback) const {
    settings_style::value(state_, i18n::text(!device.record_valid ? TextId::NotAvailable
                                             : device.recording   ? TextId::Recording
                                                                  : TextId::RecordIdle));
    char space[24];
    app::format_fixed(space, sizeof(space), device.record_space_mb, 0, " MB");
    settings_style::set_text(space_, space);
    settings_style::set_text_color(state_, device.recording ? style::kDanger : style::kText);
    settings_style::activity(status_dot_, device.recording || device.record_pending,
                             device.record_pending ? style::kAccent
                             : device.recording    ? style::kDanger
                                                   : style::kMuted);
    char value[64];
    i18n::option_text(TextId::RecordTypeOptions, type_, value, sizeof(value));
    settings_style::value(type_text_, value);
    i18n::option_text(TextId::RecordIntervalOptions, interval_, value, sizeof(value));
    settings_style::value(interval_text_, value);
    settings_style::set_text(record_text_, i18n::text(device.recording ? TextId::Stop : TextId::StartupStart));
    settings_style::set_image(record_icon_, device.recording ? "ui_stop" : "ui_play");
    settings_style::set_text(back_text_, i18n::text(TextId::Back));
    const bool locked = device.recording || device.record_pending || !device.record_valid;
    for (auto* control : {ui.type, ui.interval}) {
        if (configurable && !locked)
            lv_obj_set_disabled(control, false);
        else
            lv_obj_set_disabled(control, true);
        lv_style_value_t opa{};
        const lv_opa_t disabled_opa = configurable ? LV_OPA_50 : LV_OPA_COVER;
        if (lv_obj_get_local_style_prop(control, LV_STYLE_OPA, &opa, LV_STATE_DISABLED) != LV_STYLE_RES_FOUND
            || opa.num != disabled_opa)
            lv_obj_set_style_opa(control, disabled_opa, LV_STATE_DISABLED);
        if (configurable)
            lv_obj_set_clickable(control, true);
        else
            lv_obj_set_clickable(control, false);
    }
    if (device.record_pending || !device.record_valid)
        lv_obj_set_disabled(ui.record, true);
    else
        lv_obj_set_disabled(ui.record, false);
    settings_style::set_border_color(ui.record, device.recording ? style::kDanger : style::kBorder);
    settings_style::set_text(feedback_, feedback == TextId::Count ? "" : i18n::text(feedback));
    settings_style::set_text_color(feedback_, feedback == TextId::RequestRejected || feedback == TextId::RequestTimeout
                                                  ? style::kDanger
                                              : feedback == TextId::RequestReceived ? style::kSuccess
                                                                                    : style::kMuted);
}
