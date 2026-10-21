#include "WorkSettingsView.h"
#include "Utils/SettingsStyle.h"
using namespace page;
using i18n::TextId;

void WorkSettingsView::create(lv_obj_t* root) {
    const auto* font = resource_pool::get_font("oswaldBold_12");
    ui.preset = style::box(root, 8, 28, 88, 77, true);
    settings_style::hero(ui.preset);
    mode_icon_ = settings_style::icon(ui.preset, "ui_rover_large", 28, 8);
    preset_text_ = style::label(ui.preset, font, 4, 44, 80);
    lv_obj_set_style_text_align(preset_text_, LV_TEXT_ALIGN_CENTER, 0);
    status_dot_ = settings_style::dot(ui.preset, 41, 68);
    style::accent(ui.preset, 32, 3, 24);
    ui.protocol = style::box(root, 102, 28, 186, 23, true);
    ui.channel = style::box(root, 102, 55, 186, 23, true);
    ui.apply = style::box(root, 102, 82, 138, 23, true);
    ui.back = style::box(root, 246, 82, 42, 23, true);
    for (auto* control : {ui.protocol, ui.channel, ui.apply, ui.back})
        style::control(control);
    settings_style::icon(ui.protocol, "ui_protocol", 8, 3, style::kInfo);
    settings_style::icon(ui.channel, "ui_channel", 8, 3, style::kInfo);
    settings_style::icon(ui.apply, "ui_apply", 10, 3, style::kSuccess);
    settings_style::icon(ui.back, "ui_back", 13, 3);
    protocol_text_ = style::label(ui.protocol, font, 32, 2, 146);
    channel_text_ = style::label(ui.channel, font, 32, 2, 146);
    apply_text_ = style::label(ui.apply, font, 36, 2, 94);
    network_card_ = style::box(root, 102, 28, 186, 50);
    settings_style::hero(network_card_);
    settings_style::icon(network_card_, "ui_network", 24, 17, style::kInfo);
    settings_style::icon(network_card_, "ui_protocol", 86, 17, style::kAccent);
    settings_style::icon(network_card_, "ui_wifi", 146, 17, style::kInfo);
    for (int x : {46, 108}) {
        auto* link = style::box(network_card_, x, 24, 32, 2);
        lv_obj_set_style_bg_color(link, style::kBorder, 0);
        lv_obj_set_style_bg_opa(link, LV_OPA_COVER, 0);
    }
    feedback_ = style::label(root, font, 8, 108, 278);
    lv_obj_set_style_text_align(feedback_, LV_TEXT_ALIGN_CENTER, 0);
}
void WorkSettingsView::appear() const {
    settings_style::reveal(ui.preset);
    settings_style::reveal(ui.protocol, 40);
    settings_style::reveal(ui.channel, 80);
    settings_style::reveal(network_card_, 40);
    settings_style::reveal(ui.apply, 120);
    settings_style::reveal(ui.back, 160);
}
void WorkSettingsView::stop() const {
    settings_style::stop(status_dot_);
}
void WorkSettingsView::set_selection(unsigned protocol, unsigned channel) {
    if (protocol < DataProc::kRadioProtocolCount)
        protocol_ = protocol;
    if (channel < DataProc::kRadioChannelCount)
        channel_ = channel;
}
void WorkSettingsView::update(DataProc::WorkTarget target, bool pending, TextId feedback) const {
    const auto title = target == DataProc::WorkTarget::Base    ? TextId::RadioBase
                       : target == DataProc::WorkTarget::Rover ? TextId::RadioRover
                                                               : TextId::NetworkBase;
    settings_style::value(preset_text_, i18n::text(title));
    const char* image = target == DataProc::WorkTarget::Base    ? "ui_base_large"
                        : target == DataProc::WorkTarget::Rover ? "ui_rover_large"
                                                                : "ui_network_large";
    const auto color = target == DataProc::WorkTarget::Rover ? style::kSuccess : style::kInfo;
    settings_style::set_image(mode_icon_, image);
    settings_style::set_recolor(mode_icon_, color);
    settings_style::activity(status_dot_, pending, pending ? style::kAccent : color);
    char value[64];
    i18n::option_text(TextId::WorkRadioProtocolOptions, protocol_, value, sizeof(value));
    settings_style::value(protocol_text_, value);
    i18n::option_text(TextId::WorkRadioChannelOptions, channel_, value, sizeof(value));
    settings_style::value(channel_text_, value);
    settings_style::set_text(apply_text_, i18n::text(TextId::Apply));
    const bool radio = target != DataProc::WorkTarget::Ntrip;
    for (auto* control : {ui.protocol, ui.channel})
        settings_style::set_visible(control, radio);
    settings_style::set_visible(network_card_, !radio);
    for (auto* control : {ui.preset, ui.protocol, ui.channel, ui.apply}) {
        if (pending)
            lv_obj_add_state(control, LV_STATE_DISABLED);
        else
            lv_obj_clear_state(control, LV_STATE_DISABLED);
    }
    settings_style::set_text(feedback_, feedback == TextId::Count ? "" : i18n::text(feedback));
    settings_style::set_text_color(feedback_, feedback == TextId::RequestRejected || feedback == TextId::RequestTimeout
                                                      || feedback == TextId::SelectChannel
                                                  ? style::kDanger
                                              : feedback == TextId::RequestReceived ? style::kSuccess
                                                                                    : style::kMuted);
}
