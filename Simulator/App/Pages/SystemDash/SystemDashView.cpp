#include "SystemDashView.h"
#include <cassert>
#include "Utils/PageStyle.h"
using namespace page;
using i18n::TextId;

lv_obj_t* SystemDashView::caption(lv_obj_t* parent, TextId id, int x, int y, int width, bool small) {
    assert(caption_count_ < captions_.size());
    auto* label = style::label(parent, resource_pool::get_font(small ? "oswaldBold_12" : "oswaldBold_18"), x, y, width);
    captions_[caption_count_++] = {label, id};
    lv_label_set_text(label, i18n::text(id));
    return label;
}
void SystemDashView::create(lv_obj_t* root, bool reboot_available) {
    caption_count_ = 0;
    reboot_available_ = reboot_available;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    auto* header = style::box(root, 0, 0, 294, 28);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    caption(header, TextId::SystemMenuTitle, 10, 0, 274);
    ui.power = style::box(root, 8, 34, 88, 84, true);
    ui.settings = style::box(root, 103, 34, 88, 84, true);
    ui.back = style::box(root, 198, 34, 88, 84, true);
    const std::array<lv_obj_t*, 3> tiles{ui.power, ui.settings, ui.back};
    const std::array<TextId, 3> titles{TextId::PowerTitle, TextId::SettingsTitle, TextId::Return};
    constexpr const char* images[]{"system_dash_power", "system_dash_settings", "system_dash_return"};
    for (unsigned i = 0; i < tiles.size(); ++i) {
        auto* tile = tiles[i];
        style::card(tile);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x333333), 0);
        auto* icon = lv_img_create(tile);
        lv_img_set_src(icon, resource_pool::get_image(images[i]));
        lv_obj_set_style_img_recolor(icon, i ? lv_color_white() : lv_color_hex(0xd03c3b), 0);
        lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
        // New 40px masks render 1:1; enlarging the original 16px assets blurred the edges.
        lv_obj_set_pos(icon, 24, 8);
        auto* text = caption(tile, titles[i], 3, 51, 82);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
        style::appear(tile, i * 40);
    }
    ui.overlay = style::box(root, 0, 0, 294, 126, true);
    lv_obj_set_style_bg_color(ui.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_70, 0);
    auto* panel = style::box(ui.overlay, 8, 10, 278, 106);
    style::panel(panel);
    auto* power_icon = lv_img_create(panel);
    lv_img_set_src(power_icon, resource_pool::get_image("system_dash_power_small"));
    lv_obj_set_style_img_recolor(power_icon, lv_color_hex(0xd03c3b), 0);
    lv_obj_set_style_img_recolor_opa(power_icon, LV_OPA_COVER, 0);
    // Native 24px variant for the modal, also without image scaling.
    lv_obj_set_pos(power_icon, 12, 8);
    caption(panel, TextId::PowerTitle, 44, 6, 222);
    auto* hint = caption(panel, TextId::PowerPreviewHint, 12, 39, 254, true);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x999999), 0);
    ui.shutdown = style::box(panel, 8, 68, 82, 28, true);
    ui.reboot = style::box(panel, 98, 68, 82, 28, true);
    ui.cancel = style::box(panel, 188, 68, 82, 28, true);
    const std::array<TextId, 3> ids{TextId::ShutdownDouble, TextId::RebootDouble, TextId::Cancel};
    unsigned index = 0;
    for (auto* button : {ui.shutdown, ui.reboot, ui.cancel}) {
        style::card(button);
        lv_obj_set_style_bg_color(button, lv_color_hex(index < 2 ? 0xd03c3b : 0x666666), 0);
        auto* text = caption(button, ids[index++], 2, 4, 78, true);
        lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    }
    if (!reboot_available_) {
        // Two buttons centered in the 278px panel, 14px apart.
        lv_obj_add_flag(ui.reboot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_x(ui.shutdown, 50);
        lv_obj_set_x(ui.cancel, 146);
    }
    show_power(false);
}
void SystemDashView::apply_language() const {
    for (unsigned i = 0; i < caption_count_; ++i)
        lv_label_set_text(captions_[i].label, i18n::text(captions_[i].id));
}
void SystemDashView::show_power(bool visible) {
    power_open_ = visible;
    if (visible)
        lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(ui.overlay, LV_OBJ_FLAG_HIDDEN);
}
std::array<lv_obj_t*, 3> SystemDashView::controls() const {
    return power_open_ ? std::array<lv_obj_t*, 3>{ui.shutdown, reboot_available_ ? ui.reboot : nullptr, ui.cancel}
                       : std::array<lv_obj_t*, 3>{ui.power, ui.settings, ui.back};
}
std::array<lv_obj_t*, 6> SystemDashView::all_controls() const {
    return {ui.power, ui.settings, ui.back, ui.shutdown, ui.reboot, ui.cancel};
}
