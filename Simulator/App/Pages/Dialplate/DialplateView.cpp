#include "DialplateView.h"
#include "Resource/ResourcePool.h"
#include "Utils/PageStyle.h"
#include "Utils/SettingsStyle.h"
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
    update(DataProc::DeviceSnapshot{});

    ui.anim_timeline = lv_anim_timeline_create();

    constexpr lv_coord_t y_tar_top = 28;
    constexpr lv_coord_t h_tar_btn = 32;

    const lv_anim_timeline_wrapper_t wrapper[] = {
        {0, ui.top_info.cont, animate_y, -54, y_tar_top, 260, lv_anim_path_ease_out, true},
        {80, ui.btn_cont.btn_map, animate_height, 0, h_tar_btn, 180, lv_anim_path_ease_out, true},
        {120, ui.btn_cont.btn_rec, animate_height, 0, h_tar_btn, 180, lv_anim_path_ease_out, true},
        {160, ui.btn_cont.btn_menu, animate_height, 0, h_tar_btn, 180, lv_anim_path_ease_out, true},
        {200, ui.btn_cont.btn_shutdown, animate_height, 0, h_tar_btn, 180, lv_anim_path_ease_out, true},
        LV_ANIM_TIMELINE_WRAPPER_END};
    lv_anim_timeline_add_wrapper(ui.anim_timeline, wrapper);
}

void DialplateView::destroy() {
    anim_stop();
    if (ui.anim_timeline) {
        lv_anim_timeline_del(ui.anim_timeline);
        ui.anim_timeline = nullptr;
    }
    delete ui.top_info.satellite_used;
    ui.top_info.satellite_used = nullptr;
    delete ui.top_info.satellite_tracked;
    ui.top_info.satellite_tracked = nullptr;
}

void DialplateView::anim_start() {
    anim_enabled_ = true;
    sync_anim();
}

void DialplateView::anim_stop() {
    anim_enabled_ = false;
    sync_anim();
}

void DialplateView::sync_anim() {
    const bool needed = anim_enabled_ && last_device_.work_valid;
    if (needed == anim_running_)
        return;
    anim_running_ = needed;
    if (!needed) {
        lv_anim_del(this, anim_step_exec);
        anim_phase_ = 0;
        return;
    }
    lv_anim_init(&anim_loop_);
    lv_anim_set_var(&anim_loop_, this);
    lv_anim_set_exec_cb(&anim_loop_, anim_step_exec);
    lv_anim_set_values(&anim_loop_, 0, kAnimPeriod - 1);
    lv_anim_set_time(&anim_loop_, kAnimPeriod);
    lv_anim_set_repeat_count(&anim_loop_, LV_ANIM_REPEAT_INFINITE);
    if (!lv_anim_start(&anim_loop_))
        anim_running_ = false; // 动画分配失败时保持静态画面，下一次 update 再尝试。
}

void DialplateView::anim_step_exec(void* var, int32_t value) {
    auto* self = static_cast<DialplateView*>(var);
    // 量化到 50 ms：MCU 上每秒最多重绘 20 次两张小卡片，而不是每个刷新周期都重绘。
    const uint32_t phase = static_cast<uint32_t>(value) / kAnimStep * kAnimStep;
    if (phase == self->anim_phase_)
        return;
    self->anim_phase_ = phase;
    if (self->last_device_.radio_on)
        lv_obj_invalidate(self->ui.top_info.radio_card);
    lv_obj_invalidate(self->ui.top_info.mode_card);
}

void DialplateView::top_info_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, 282, 54);
    lv_obj_set_pos(cont, 6, 28);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(cont, style::kPanel, 0);
    lv_obj_set_style_border_color(cont, style::kBorder, 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_radius(cont, 10, 0);
    ui.top_info.cont = cont;

    // 卫星按钮（可聚焦并支持产品 StarMap 配置）
    TransformInfo_t sat_tf = {36, 34, 36, 32};
    lv_obj_t* icon_satellite = btn_create(cont, resource_pool::get_image("ui_satellite_large"), 6, 10, sat_tf);
    lv_obj_set_style_bg_img_recolor(icon_satellite, style::kInfo, 0);
    lv_obj_set_style_bg_img_recolor_opa(icon_satellite, LV_OPA_COVER, 0);
    ui.top_info.icon_satellite = icon_satellite;

    const lv_font_t* font_large = resource_pool::get_font("rajdhaniBold_40");
    const lv_font_t* font_small = resource_pool::get_font("rajdhaniBold_20");
    const lv_font_t* font_separator = resource_pool::get_font("oswaldBold_18");

    // 整组按实际位数居中，斜杠与两组数字保持同一基线。
    auto* counts = style::box(cont, 44, 12, 108, font_large->line_height);
    lv_obj_add_flag(counts, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_flex_flow(counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(counts, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(counts, 2, 0);

    ui.top_info.satellite_used = new NumberFlow(font_large, 3, true);
    ui.top_info.satellite_used->create(counts);
    ui.top_info.satellite_used->set_value(0);

    auto* separator = style::label(counts, font_separator, 0, 0, 7);
    lv_obj_set_style_translate_y(separator, font_separator->base_line, 0);
    lv_obj_set_style_text_color(separator, style::kMuted, 0);
    lv_label_set_text(separator, "/");
    ui.top_info.satellite_separator = separator;

    ui.top_info.satellite_tracked = new NumberFlow(font_small, 3, true);
    ui.top_info.satellite_tracked->create(counts);
    ui.top_info.satellite_tracked->set_value(0);
    for (auto* number : {ui.top_info.satellite_used->get_cont(), ui.top_info.satellite_tracked->get_cont()}) {
        lv_obj_set_flex_flow(number, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(number, 0, 0);
        lv_obj_set_width(number, LV_SIZE_CONTENT);
    }

    ui.top_info.satellite_missing = style::label(cont, font_separator, 44, 14, 108);
    lv_obj_set_style_text_align(ui.top_info.satellite_missing, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui.top_info.satellite_missing, style::kMuted, 0);
    lv_label_set_text(ui.top_info.satellite_missing, "-- / --");

    // 垂直分隔线
    lv_obj_t* div = lv_obj_create(cont);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 1, 38);
    lv_obj_set_pos(div, 154, 8);
    lv_obj_set_style_bg_color(div, style::kBorder, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);

    // 电台与工作模式卡片：16 px 图标居中，动效画在图标外侧，不与图标重叠。
    status_card(cont, 162, "ui_antenna", ui.top_info.radio_card, ui.top_info.radio_img, ui.top_info.radio_label);
    lv_obj_add_event_cb(ui.top_info.radio_card, radio_draw_event_cb, LV_EVENT_DRAW_POST, this);
    ui.top_info.icon_radio = ui.top_info.radio_card;
    status_card(cont, 222, "ui_rover", ui.top_info.mode_card, ui.top_info.mode_img, ui.top_info.mode_label);
    // 画在子对象之前：移动站测点涟漪位于对中杆下层，杆身始终完整可见。
    lv_obj_add_event_cb(ui.top_info.mode_card, mode_draw_event_cb, LV_EVENT_DRAW_MAIN_END, this);
    ui.top_info.icon_mode = ui.top_info.mode_card;
}

void DialplateView::status_card(lv_obj_t* par, lv_coord_t x, const char* image, lv_obj_t*& card, lv_obj_t*& img,
                                lv_obj_t*& label) {
    card = lv_obj_create(par);
    lv_obj_remove_style_all(card);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(card, kCardWidth, kCardHeight);
    lv_obj_set_pos(card, x, 5);
    lv_obj_set_style_bg_color(card, style::kCard, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, style::kBorder, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);

    img = lv_img_create(card);
    lv_img_set_src(img, resource_pool::get_image(image));
    lv_obj_set_pos(img, kIconX, kIconY);
    lv_obj_set_style_img_recolor(img, style::kMuted, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);

    label = lv_label_create(card);
    lv_obj_remove_style_all(label);
    lv_obj_set_style_text_font(label, resource_pool::get_font("oswaldBold_12"), 0);
    lv_obj_set_style_text_color(label, style::kMuted, 0);
    lv_label_set_text(label, "--");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -2);
}

void DialplateView::btn_cont_create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(cont, LV_HOR_RES, 40);
    lv_obj_align_to(cont, ui.top_info.cont, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    ui.btn_cont.cont = cont;

    TransformInfo_t transform = {56, 32, 54, 29};

    ui.btn_cont.btn_map = btn_create(cont, resource_pool::get_image("ui_work"), -105, 0, transform);
    ui.btn_cont.btn_rec = btn_create(cont, resource_pool::get_image("ui_record"), -35, 0, transform);
    ui.btn_cont.btn_menu = btn_create(cont, resource_pool::get_image("ui_info"), 35, 0, transform);
    ui.btn_cont.btn_shutdown = btn_create(cont, resource_pool::get_image("ui_settings"), 105, 0, transform);
    for (auto* control : {ui.btn_cont.btn_map, ui.btn_cont.btn_rec, ui.btn_cont.btn_menu, ui.btn_cont.btn_shutdown}) {
        lv_obj_set_style_bg_img_recolor(control, lv_color_white(), 0);
        lv_obj_set_style_bg_img_recolor_opa(control, LV_OPA_COVER, 0);
    }
}

lv_obj_t* DialplateView::btn_create(lv_obj_t* par, const void* img_src, const lv_coord_t x, const lv_coord_t y,
                                    const TransformInfo_t& transform) {
    lv_obj_t* obj = lv_obj_create(par);
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_size(obj, transform.width_default, transform.height_default);
    // y == 0 表示按钮行内按中心水平偏移排列，否则为父对象内绝对位置。
    if (y == 0)
        lv_obj_align(obj, LV_ALIGN_CENTER, x, 0);
    else
        lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_img_src(obj, img_src, 0);

    page::style::control(obj);

    lv_obj_set_style_width(obj, transform.transform_width, LV_STATE_PRESSED);
    lv_obj_set_style_height(obj, transform.transform_height, LV_STATE_PRESSED);

    lv_obj_update_layout(obj);

    return obj;
}

void DialplateView::appear_anim_start(const bool reverse) const {
    lv_anim_timeline_set_reverse(ui.anim_timeline, reverse);
    lv_anim_timeline_start(ui.anim_timeline);
}

void DialplateView::update(const DataProc::DeviceSnapshot& device) {
    // 设备快照约每秒发布一次（含运行时长），只在显示内容变化时触碰 LVGL，避免首页每秒整块重绘。
    const bool cards_changed = device.work_valid != last_device_.work_valid
                               || device.work_mode != last_device_.work_mode
                               || device.radio_on != last_device_.radio_on;
    last_device_ = device;

    ui.top_info.satellite_used->set_value(device.satellites_valid ? device.satellites_used : 0U);
    ui.top_info.satellite_tracked->set_value(device.satellites_valid ? device.satellites_tracked : 0U);
    for (auto* count : {ui.top_info.satellite_used->get_cont(), ui.top_info.satellite_tracked->get_cont(),
                        ui.top_info.satellite_separator})
        settings_style::set_visible(count, device.satellites_valid);
    settings_style::set_visible(ui.top_info.satellite_missing, !device.satellites_valid);

    const bool rover = device.work_valid && device.work_mode == DataProc::WorkMode::Rover;
    const bool radio_on = device.work_valid && device.radio_on;

    // 工作模式与电台共用状态色；无有效数据时明确显示未知。
    const auto active_color = rover ? style::kSuccess : style::kInfo;
    const auto mode_color = device.work_valid ? active_color : style::kMuted;
    settings_style::set_image(ui.top_info.mode_img, device.work_valid && !rover ? "ui_base" : "ui_rover");
    settings_style::set_recolor(ui.top_info.mode_img, mode_color);
    settings_style::set_text(ui.top_info.mode_label, device.work_valid ? (rover ? "ROVER" : "BASE") : "--");
    settings_style::set_text_color(ui.top_info.mode_label, mode_color);
    settings_style::set_border_color(ui.top_info.mode_card, device.work_valid
                                                                ? lv_color_mix(active_color, style::kCard, LV_OPA_30)
                                                                : style::kBorder);

    const auto radio_color = radio_on ? active_color : style::kMuted;
    settings_style::set_recolor(ui.top_info.radio_img, radio_color);
    settings_style::set_text(ui.top_info.radio_label, !device.work_valid ? "--"
                                                      : radio_on         ? (rover ? "RX" : "TX")
                                                                         : "OFF");
    settings_style::set_text_color(ui.top_info.radio_label, radio_color);
    settings_style::set_border_color(ui.top_info.radio_card,
                                     radio_on ? lv_color_mix(active_color, style::kCard, LV_OPA_30) : style::kBorder);

    // 首页始终是记录管理入口；红色边线只提示记录状态。
    const bool recording = device.record_valid && device.recording;
    settings_style::set_border_color(ui.btn_cont.btn_rec, recording ? style::kDanger : style::kBorder);

    // 动效只依赖 last_device_：状态变化时立即按新状态重绘，并在无动效需要时停止定时动画。
    if (cards_changed) {
        lv_obj_invalidate(ui.top_info.radio_card);
        lv_obj_invalidate(ui.top_info.mode_card);
    }
    sync_anim();
}

namespace {
// LVGL 角度 0° 指向右侧并顺时针增加；跨 0° 的弧拆成两段绘制。
void draw_arc(lv_draw_ctx_t* ctx, const lv_draw_arc_dsc_t* dsc, const lv_point_t* center, uint16_t radius, int start,
              int end) {
    if (start < 0) {
        lv_draw_arc(ctx, dsc, center, radius, static_cast<uint16_t>(start + 360), 360);
        start = 0;
    }
    lv_draw_arc(ctx, dsc, center, radius, static_cast<uint16_t>(start), static_cast<uint16_t>(end));
}

// 三角脉冲：phase 距 peak 越近越亮，窗口外保持底亮度。
lv_opa_t pulse(uint32_t phase, uint32_t peak, uint32_t period, uint32_t width, lv_opa_t floor) {
    const uint32_t distance = (phase + period - peak) % period;
    const uint32_t offset = distance < period / 2 ? distance : period - distance;
    if (offset >= width)
        return floor;
    return static_cast<lv_opa_t>(LV_OPA_COVER - (LV_OPA_COVER - floor) * offset / width);
}
} // namespace

void DialplateView::radio_draw_event_cb(lv_event_t* e) {
    auto* self = static_cast<DialplateView*>(lv_event_get_user_data(e));
    const auto& device = self->last_device_;
    if (!device.work_valid || !device.radio_on)
        return;

    lv_area_t coords;
    lv_obj_get_coords(lv_event_get_target(e), &coords);
    // 波形以天线顶端为圆心，仅画左右两侧 ±22° 的短弧；最内弧半径大于天线顶端宽度，不压到图标。
    const lv_point_t tip = {static_cast<lv_coord_t>(coords.x1 + kIconX + 8),
                            static_cast<lv_coord_t>(coords.y1 + kIconY + 4)};
    const bool rover = device.work_mode == DataProc::WorkMode::Rover;
    constexpr uint16_t radii[] = {9, 13, 17};

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = rover ? style::kSuccess : style::kInfo;
    arc.width = 2;
    arc.rounded = 1;
    for (uint32_t i = 0; i < 3; ++i) {
        // TX（基站）由内向外扩散；RX（移动站）由外向内收拢。
        const uint32_t order = rover ? 2 - i : i;
        arc.opa = pulse(self->anim_phase_, order * kAnimPeriod / 4, kAnimPeriod, kAnimPeriod / 4, LV_OPA_20);
        draw_arc(lv_event_get_draw_ctx(e), &arc, &tip, radii[i], 180 - 22, 180 + 22);
        draw_arc(lv_event_get_draw_ctx(e), &arc, &tip, radii[i], -22, 22);
    }
}

void DialplateView::mode_draw_event_cb(lv_event_t* e) {
    auto* self = static_cast<DialplateView*>(lv_event_get_user_data(e));
    const auto& device = self->last_device_;
    if (!device.work_valid)
        return;

    lv_area_t coords;
    lv_obj_get_coords(lv_event_get_target(e), &coords);
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    const uint32_t phase = self->anim_phase_;
    // 图标坐标：接收机天线罩底边在图标 y=5，对中杆尖在 y=15，水平中心 x=8。
    const lv_coord_t cx = coords.x1 + kIconX + 8;
    const lv_coord_t dome_base = coords.y1 + kIconY + 5, pole_tip = coords.y1 + kIconY + 15;

    if (device.work_mode != DataProc::WorkMode::Rover) {
        // 基准站：天线罩上方三道弧由内向外依次亮起，表示持续播发差分改正数。
        lv_draw_arc_dsc_t arc;
        lv_draw_arc_dsc_init(&arc);
        arc.color = style::kInfo;
        arc.width = 2;
        arc.rounded = 1;
        const lv_point_t center = {cx, dome_base};
        constexpr uint16_t radii[] = {6, 9, 12};
        for (uint32_t i = 0; i < 3; ++i) {
            arc.opa = pulse(phase, i * kAnimPeriod / 4, kAnimPeriod, kAnimPeriod / 4, LV_OPA_TRANSP);
            if (arc.opa > LV_OPA_MIN)
                draw_arc(ctx, &arc, &center, radii[i], 270 - 35, 270 + 35);
        }
        return;
    }

    // 移动站：信号点先落入天线罩（收到卫星/差分信号），随后杆尖地面依次扩散两圈测点涟漪。
    constexpr uint32_t kFall = kAnimPeriod * 3 / 10, kRipple = kAnimPeriod / 2;
    if (phase < kFall) {
        lv_draw_rect_dsc_t dot;
        lv_draw_rect_dsc_init(&dot);
        dot.bg_color = style::kSuccess;
        dot.bg_opa = static_cast<lv_opa_t>(LV_OPA_40 + (LV_OPA_COVER - LV_OPA_40) * phase / kFall);
        dot.radius = LV_RADIUS_CIRCLE;
        const auto y = static_cast<lv_coord_t>(coords.y1 + 1 + (dome_base - 6 - coords.y1 - 1) * phase / kFall);
        const lv_area_t area = {static_cast<lv_coord_t>(cx - 1), y, cx, static_cast<lv_coord_t>(y + 1)};
        lv_draw_rect(ctx, &dot, &area);
    }
    lv_draw_rect_dsc_t ring;
    lv_draw_rect_dsc_init(&ring);
    ring.bg_opa = LV_OPA_TRANSP;
    ring.border_color = style::kSuccess;
    ring.border_width = 1;
    ring.radius = LV_RADIUS_CIRCLE;
    for (const uint32_t start : {kFall, kFall + kAnimPeriod / 4}) {
        if (phase < start || phase >= start + kRipple)
            continue;
        const uint32_t age = phase - start;
        const auto rx = static_cast<lv_coord_t>(2 + 10 * age / kRipple),
                   ry = static_cast<lv_coord_t>(1 + 2 * age / kRipple); // 1→2 px，不压文字
        ring.border_opa = static_cast<lv_opa_t>(LV_OPA_COVER - LV_OPA_COVER * age / kRipple);
        const lv_area_t area = {static_cast<lv_coord_t>(cx - rx), static_cast<lv_coord_t>(pole_tip - ry),
                                static_cast<lv_coord_t>(cx + rx - 1), static_cast<lv_coord_t>(pole_tip + ry)};
        lv_draw_rect(ctx, &ring, &area);
    }
}
