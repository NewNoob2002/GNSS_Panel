#include "DialplateView.h"
#include "Resource/ResourcePool.h"
#include "Utils/lv_ext/lv_anim_timeline_wrapper.h"

using namespace page;

static void animate_y(void* object, int32_t value) {
    lv_obj_set_y(static_cast<lv_obj_t*>(object), static_cast<lv_coord_t>(value));
}

static void animate_height(void* object, int32_t value) {
    lv_obj_set_height(static_cast<lv_obj_t*>(object), static_cast<lv_coord_t>(value));
}

void DialplateView::create(lv_obj_t* root) {
    top_info_create(root);
    btn_cont_create(root);

    ui.anim_timeline = lv_anim_timeline_create();

    const lv_coord_t y_tar_top = lv_obj_get_y(ui.top_info.cont);
    const lv_coord_t h_tar_btn = lv_obj_get_height(ui.btn_cont.btn_rec);

    const lv_anim_timeline_wrapper_t wrapper[] = {
        {0, ui.top_info.cont, animate_y, -lv_obj_get_height(ui.top_info.cont), y_tar_top, 500, lv_anim_path_ease_out,
         true},
        {500, ui.btn_cont.btn_map, animate_height, 0, h_tar_btn, 500, lv_anim_path_ease_out, true},
        {600, ui.btn_cont.btn_rec, animate_height, 0, h_tar_btn, 500, lv_anim_path_ease_out, true},
        {700, ui.btn_cont.btn_menu, animate_height, 0, h_tar_btn, 500, lv_anim_path_ease_out, true},
        {800, ui.btn_cont.btn_shutdown, animate_height, 0, h_tar_btn, 500, lv_anim_path_ease_out, true},
        LV_ANIM_TIMELINE_WRAPPER_END};
    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
}

void DialplateView::destroy() {
    if (ui.anim_timeline) {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
    delete ui.top_info.satellite_used;
    ui.top_info.satellite_used = nullptr;
    delete ui.top_info.satellite_tacked;
    ui.top_info.satellite_tacked = nullptr;
}

void DialplateView::top_info_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 60);

    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), 0);

    lv_obj_set_style_radius(cont, 27, 0);
    lv_obj_set_y(cont, 26);
    ui.top_info.cont = cont;

    TransformInfo_t transform = {45, 35, 50, 30};
    lv_obj_t* icon_satellite = btn_create(cont, resource_pool::get_image("satellite_big"), -110, transform);
    ui.top_info.icon_satellite = icon_satellite;

    const lv_font_t* font_large = resource_pool::get_font("rajdhaniBold_40");
    const lv_font_t* font_small = resource_pool::get_font("rajdhaniBold_20");

    ui.top_info.satellite_used = new NumberFlow(font_large, 2);
    ui.top_info.satellite_used->create(cont);
    ui.top_info.satellite_used->set_value(0);
    ui.top_info.satellite_used->set_align_to(icon_satellite, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    lv_obj_t* separator = lv_label_create(cont);
    lv_obj_remove_style_all(separator);
    lv_obj_set_style_text_font(separator, resource_pool::get_font("oswaldBold_18"), 0);
    lv_label_set_text(separator, "/");
    lv_obj_align_to(separator, ui.top_info.satellite_used->get_cont(), LV_ALIGN_OUT_RIGHT_MID, 0, 10);

    ui.top_info.satellite_tacked = new NumberFlow(font_small, 2);
    ui.top_info.satellite_tacked->create(cont);
    ui.top_info.satellite_tacked->set_value(0);
    ui.top_info.satellite_tacked->set_align_to(separator, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_obj_t* icon_radio = lv_label_create(cont);
    lv_obj_set_style_text_font(icon_radio, resource_pool::get_font("dialplate"), 0);
    lv_obj_set_style_text_color(icon_radio, lv_color_white(), 0);
    lv_label_set_text(icon_radio, CUSTOM_SYMBOL_RADIO);
    lv_obj_align(icon_radio, LV_ALIGN_RIGHT_MID, -80, 0);
    ui.top_info.icon_radio = icon_radio;

    lv_obj_t* icon_mode = lv_label_create(cont);
    lv_obj_set_style_text_font(icon_mode, resource_pool::get_font("dialplate"), 0);
    lv_obj_set_style_text_color(icon_mode, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_label_set_text(icon_mode, CUSTOM_SYMBOL_BASE);
    lv_obj_align(icon_mode, LV_ALIGN_RIGHT_MID, -20, 0);
    ui.top_info.icon_mode = icon_mode;
}

void DialplateView::btn_cont_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, 40);
    lv_obj_align_to(cont, ui.top_info.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    ui.btn_cont.cont = cont;
    TransformInfo_t transform = {40, 31, 45, 25};

    ui.btn_cont.btn_map = btn_create(cont, resource_pool::get_image("settings"), -110, transform);
    ui.btn_cont.btn_rec = btn_create(cont, resource_pool::get_image("start"), -40, transform);
    ui.btn_cont.btn_menu = btn_create(cont, resource_pool::get_image("menu"), 40, transform);
    ui.btn_cont.btn_shutdown = btn_create(cont, resource_pool::get_image("shutdown"), 110, transform);
}

lv_obj_t* DialplateView::btn_create(lv_obj_t* par, const void* img_src, const lv_coord_t x_ofs,
                                    const TransformInfo_t& transform) {
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, transform.width_default, transform.height_default);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(obj, LV_ALIGN_CENTER, x_ofs, 0);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_width(obj, transform.transform_width, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, transform.transform_height, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x666666), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xbbbbbb), LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff931e), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(obj, 9, 0);

    static lv_style_transition_dsc_t tran;
    static constexpr lv_style_prop_t prop[] = {LV_STYLE_WIDTH, LV_STYLE_HEIGHT, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&tran, prop, lv_anim_path_ease_out, 200, 0, nullptr);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &tran, LV_STATE_FOCUSED);

    lv_obj_update_layout(obj);

    return obj;
}

void DialplateView::appear_anim_start(const bool reverse) const {
    lv_anim_timeline_set_reverse(ui.anim_timeline, reverse);
    lv_anim_timeline_start(ui.anim_timeline);
}
