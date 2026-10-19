#include "SaveConfigView.h"
#include "src/misc/lv_log.h"

using namespace Page;

namespace {

constexpr lv_coord_t kProgressWidth = 184;
constexpr lv_coord_t kProgressHeight = 12;
constexpr lv_coord_t kBrandMaxWidth = 210;
constexpr lv_coord_t kBrandHeight = 20;
constexpr lv_coord_t kBrandPadX = 8;
constexpr lv_coord_t kShineLength = 12;
constexpr lv_coord_t kShineThickness = 2;
constexpr lv_color_t kAmber = LV_COLOR_MAKE(0xE1, 0xAA, 0x22);
constexpr lv_color_t kAmberBright = LV_COLOR_MAKE(0xF6, 0xD0, 0x62);
constexpr lv_color_t kLineGrey = LV_COLOR_MAKE(0x72, 0x6E, 0x63);
constexpr lv_color_t kTextWarm = LV_COLOR_MAKE(0xE8, 0xE1, 0xCF);

const char*
GetBrandName() {
#if defined(APP_BRAND_NAME)
    return APP_BRAND_NAME;
#elif defined(RGK_LOGO_USE)
    return "RGK";
#elif defined(MIDDLE_LOGO_USE)
    return "X";
#else
    return "SINGULARXYZ";
#endif
}

lv_obj_t*
CreateBrandLogo(lv_obj_t* parent, const char* brandName, const lv_font_t* font) {
    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x1D1C18), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_60, 0);
    lv_obj_set_style_border_color(cont, kLineGrey, 0);
    lv_obj_set_style_border_width(cont, 2, 0);
    lv_obj_set_style_radius(cont, 0, 0);

    lv_obj_t* label = lv_label_create(cont);
    lv_obj_remove_style_all(label);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, kAmberBright, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_label_set_text(label, brandName);
    lv_obj_update_layout(label);

    lv_coord_t labelWidth = lv_obj_get_width(label);
    lv_coord_t contWidth = labelWidth + kBrandPadX * 2;
    if (contWidth > kBrandMaxWidth) {
        contWidth = kBrandMaxWidth;
        labelWidth = kBrandMaxWidth - kBrandPadX * 2;
        lv_obj_set_width(label, labelWidth);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }
    LV_LOG_USER("BrandWidth [1] :%d", contWidth);
    lv_obj_set_size(cont, contWidth, kBrandHeight + kBrandPadX);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_update_layout(cont);

    return cont;
}

void
brand_shine_bottom_exec(void* obj, int32_t value) {
    auto* shine = static_cast<lv_obj_t*>(obj);
    if (shine == nullptr) {
        return;
    }

    lv_obj_t* cont = lv_obj_get_parent(shine);
    const lv_coord_t bottomY = lv_obj_get_height(cont) - kShineThickness;
    lv_obj_set_pos(shine, static_cast<lv_coord_t>(value), bottomY);
}

void
syncbar_progress_exec(void* obj, const int32_t width) {
    lv_obj_set_width(static_cast<lv_obj_t*>(obj), width);

    auto* view = static_cast<SaveConfigView*>(lv_obj_get_user_data(static_cast<lv_obj_t*>(obj)));
    if (view == nullptr || view->ui.sync.percentLabel == nullptr) {
        return;
    }

    const int32_t percent = width * 100 / kProgressWidth;
    lv_label_set_text_fmt(view->ui.sync.percentLabel, "%ld%%", static_cast<long>(percent));
}

} // namespace

void
SaveConfigView::Create(lv_obj_t* root) {
    const lv_font_t* font_small = ResourcePool::GetFont("oswaldBold_12");
    const lv_font_t* font_brand = ResourcePool::GetFont("oswaldBold_18");

    lv_obj_t* main_cont = lv_obj_create(root);
    lv_obj_remove_style_all(main_cont);
    lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(main_cont);
    lv_obj_set_style_bg_opa(main_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x151513), 0);
    lv_obj_set_style_bg_grad_color(main_cont, lv_color_hex(0x2A2924), 0);
    lv_obj_set_style_bg_grad_dir(main_cont, LV_GRAD_DIR_VER, 0);

    lv_obj_t* second_cont = lv_obj_create(root);
    lv_obj_remove_style_all(second_cont);
    lv_obj_set_size(second_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(second_cont);
    ui.sync.cont = second_cont;

    lv_obj_t* top_line = lv_obj_create(second_cont);
    lv_obj_remove_style_all(top_line);
    lv_obj_set_size(top_line, 246, 2);
    lv_obj_set_style_bg_color(top_line, kAmber, 0);
    lv_obj_set_style_bg_grad_color(top_line, kLineGrey, 0);
    lv_obj_set_style_bg_grad_dir(top_line, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(top_line, LV_OPA_50, 0);
    lv_obj_align(top_line, LV_ALIGN_TOP_MID, 0, 7);

    lv_obj_t* brand_cont = CreateBrandLogo(second_cont, GetBrandName(), font_brand);
    lv_obj_align(brand_cont, LV_ALIGN_TOP_MID, 0, 15);
    ui.sync.brandCont = brand_cont;

    lv_obj_t* brand_shine = lv_obj_create(brand_cont);
    lv_obj_remove_style_all(brand_shine);
    lv_obj_clear_flag(brand_shine, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(brand_shine, kShineLength, kShineThickness);
    lv_obj_set_style_bg_color(brand_shine, kLineGrey, 0);
    lv_obj_set_style_bg_grad_color(brand_shine, kAmberBright, 0);
    lv_obj_set_style_bg_grad_dir(brand_shine, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(brand_shine, LV_OPA_90, 0);
    lv_obj_set_pos(brand_shine, 0, lv_obj_get_height(brand_cont) - kShineThickness);
    ui.sync.brandShine = brand_shine;

    lv_anim_init(&ui.sync.shineAnim);
    lv_anim_set_var(&ui.sync.shineAnim, brand_shine);
    lv_anim_set_exec_cb(&ui.sync.shineAnim, brand_shine_bottom_exec);
    const lv_coord_t brandWidth = lv_obj_get_width(brand_cont);
    LV_LOG_USER("BrandWidth [2]:%d", brandWidth);
    const lv_coord_t shineStart = kBrandPadX;
    const lv_coord_t shineEnd = brandWidth - kBrandPadX - kShineLength;
    lv_anim_set_values(&ui.sync.shineAnim, shineStart, LV_MAX(shineStart, shineEnd));
    lv_anim_set_time(&ui.sync.shineAnim, 1000);
    lv_anim_set_playback_time(&ui.sync.shineAnim, 1000);
    lv_anim_set_delay(&ui.sync.shineAnim, 500);
    lv_anim_set_repeat_delay(&ui.sync.shineAnim, 0);
    lv_anim_set_repeat_count(&ui.sync.shineAnim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&ui.sync.shineAnim, lv_anim_path_linear);
    lv_anim_start(&ui.sync.shineAnim);

    lv_obj_t* sync_label = lv_label_create(second_cont);
    lv_obj_remove_style_all(sync_label);
    lv_obj_set_width(sync_label, 260);
    lv_label_set_long_mode(sync_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(sync_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(sync_label, font_small, 0);
    lv_obj_set_style_text_color(sync_label, kTextWarm, 0);
    lv_obj_align(sync_label, LV_ALIGN_TOP_MID, 0, 54);
    ui.sync.label = sync_label;

    lv_obj_t* bar_cont = lv_obj_create(second_cont);
    lv_obj_remove_style_all(bar_cont);
    lv_obj_set_style_bg_color(bar_cont, lv_color_hex(0x272722), 0);
    lv_obj_set_style_bg_opa(bar_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar_cont, kLineGrey, 0);
    lv_obj_set_style_border_width(bar_cont, 1, 0);
    lv_obj_set_style_radius(bar_cont, kProgressHeight / 2, 0);
    lv_obj_set_size(bar_cont, kProgressWidth + 2, kProgressHeight + 2);
    lv_obj_align(bar_cont, LV_ALIGN_BOTTOM_MID, 0, -6);
    ui.sync.bar.track = bar_cont;

    lv_obj_t* sync_bar = lv_obj_create(bar_cont);
    lv_obj_remove_style_all(sync_bar);
    lv_obj_set_size(sync_bar, 0, kProgressHeight);
    lv_obj_set_style_bg_color(sync_bar, kAmber, 0);
    lv_obj_set_style_bg_grad_color(sync_bar, kAmberBright, 0);
    lv_obj_set_style_bg_grad_dir(sync_bar, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(sync_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(sync_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sync_bar, kProgressHeight / 2, 0);
    lv_obj_align(sync_bar, LV_ALIGN_LEFT_MID, 1, 0);
    lv_obj_set_user_data(sync_bar, this);
    ui.sync.bar.obj = sync_bar;

    lv_obj_t* percent_label = lv_label_create(second_cont);
    lv_obj_remove_style_all(percent_label);
    lv_obj_set_style_text_font(percent_label, font_small, 0);
    lv_obj_set_style_text_color(percent_label, kAmberBright, 0);
    lv_label_set_text(percent_label, "0%");
    lv_obj_set_opa_scale(percent_label, LV_OPA_TRANSP);
    lv_obj_align_to(percent_label, bar_cont, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    ui.sync.percentLabel = percent_label;

    for (uint8_t i = 0; i < 3; i++) {
        lv_obj_t* dot = lv_obj_create(second_cont);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 4, 4);
        lv_obj_set_style_radius(dot, 0, 0);
        lv_obj_set_style_bg_color(dot, kAmber, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_60, 0);
        lv_obj_align(dot, LV_ALIGN_BOTTOM_MID, static_cast<lv_coord_t>(-128 + i * 8), -11);
        ui.sync.dots[i] = dot;

        lv_anim_init(&ui.sync.dotAnims[i]);
        lv_anim_set_var(&ui.sync.dotAnims[i], dot);
        lv_anim_set_exec_cb(&ui.sync.dotAnims[i], LV_ANIM_EXEC(opa_scale));
        lv_anim_set_values(&ui.sync.dotAnims[i], LV_OPA_30, LV_OPA_COVER);
        lv_anim_set_time(&ui.sync.dotAnims[i], 600);
        lv_anim_set_playback_time(&ui.sync.dotAnims[i], 600);
        lv_anim_set_delay(&ui.sync.dotAnims[i], i * 180);
        lv_anim_set_repeat_count(&ui.sync.dotAnims[i], LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&ui.sync.dotAnims[i]);
    }

    lv_anim_init(&ui.sync.bar.anim);
    lv_anim_set_var(&ui.sync.bar.anim, sync_bar);
    lv_anim_set_exec_cb(&ui.sync.bar.anim, syncbar_progress_exec);
    lv_anim_set_values(&ui.sync.bar.anim, 0, kProgressWidth);
    lv_anim_set_time(&ui.sync.bar.anim, 8000);
    lv_anim_set_playback_time(&ui.sync.bar.anim, 0);
    lv_anim_set_repeat_count(&ui.sync.bar.anim, 0);

    ui.anim_timeline = lv_anim_timeline_create();
#define ANIM_DEF(start_time, obj, attr, start, end)                                                                    \
    { start_time, obj, LV_ANIM_EXEC(attr), start, end, 500, lv_anim_path_ease_out, true }

    const lv_anim_timeline_wrapper_t wrapper[] = {
        ANIM_DEF(0, top_line, width, 0, lv_obj_get_style_width(top_line, 0)),
        ANIM_DEF(100, brand_cont, opa_scale, LV_OPA_TRANSP, LV_OPA_COVER),
        ANIM_DEF(220, sync_label, y, lv_obj_get_style_height(second_cont, 0), lv_obj_get_y(sync_label)),
        ANIM_DEF(350, bar_cont, width, 0, lv_obj_get_style_width(bar_cont, 0)),
        // ANIM_DEF(450, percent_label, opa_scale, LV_OPA_TRANSP, LV_OPA_COVER),
        LV_ANIM_TIMELINE_WRAPPER_END};

    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
    ApplyLanguage();
}

void
SaveConfigView::Delete() {
    if (ui.anim_timeline) {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
    if (ui.sync.brandShine) {
        lv_anim_del(ui.sync.brandShine, brand_shine_bottom_exec);
    }
    for (auto& dot : ui.sync.dots) {
        if (dot) {
            lv_anim_del(dot, nullptr);
        }
    }
}

void
SaveConfigView::ApplyLanguage() const {
    lv_label_set_text(ui.sync.label, I18n::Text(I18n::TextId::SaveConfigWarning));
    lv_obj_align(ui.sync.label, LV_ALIGN_TOP_MID, 0, 54);
}

void
SaveConfigView::SetPowerOffCause(const I18n::TextId causeId) const {
    lv_label_set_text_fmt(ui.sync.label, I18n::Text(I18n::TextId::SaveConfigMessage), I18n::Text(causeId));
    lv_obj_align(ui.sync.label, LV_ALIGN_TOP_MID, 0, 54);
}
