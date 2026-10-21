#include "DialplateView.h"
#include "Resource/ResourcePool.h"
#include "Utils/PageStyle.h"
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

    anim_start();
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
    if (anim_running_)
        return;
    lv_anim_init(&anim_loop_);
    lv_anim_set_var(&anim_loop_, this);
    lv_anim_set_exec_cb(&anim_loop_, anim_step_exec);
    lv_anim_set_values(&anim_loop_, 0, 1200);
    lv_anim_set_time(&anim_loop_, 1200);
    lv_anim_set_repeat_count(&anim_loop_, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&anim_loop_);
    anim_running_ = true;
}

void DialplateView::anim_stop() {
    if (anim_running_) {
        lv_anim_del(this, anim_step_exec);
        anim_running_ = false;
    }
}

void DialplateView::anim_step_exec(void* var, int32_t value) {
    auto* self = static_cast<DialplateView*>(var);
    if (!self)
        return;
    self->anim_phase_ = static_cast<uint32_t>(value);
    if (self->ui.top_info.radio_card)
        lv_obj_invalidate(self->ui.top_info.radio_card);
    if (self->ui.top_info.mode_card)
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
    lv_obj_t* icon_satellite = btn_create(cont, resource_pool::get_image("satellite_big"), 6, 10, sat_tf);
    ui.top_info.icon_satellite = icon_satellite;

    const lv_font_t* font_large = resource_pool::get_font("rajdhaniBold_40");
    const lv_font_t* font_small = resource_pool::get_font("rajdhaniBold_20");
    const lv_font_t* font_label = resource_pool::get_font("oswaldBold_12");
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

    // 电台状态卡片（图片 + 手绘动态波形）
    lv_obj_t* radio_card = lv_obj_create(cont);
    lv_obj_remove_style_all(radio_card);
    lv_obj_clear_flag(radio_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(radio_card, 54, 44);
    lv_obj_set_pos(radio_card, 162, 5);
    lv_obj_set_style_bg_color(radio_card, style::kCard, 0);
    lv_obj_set_style_bg_opa(radio_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(radio_card, style::kBorder, 0);
    lv_obj_set_style_border_width(radio_card, 1, 0);
    lv_obj_set_style_radius(radio_card, 8, 0);
    ui.top_info.radio_card = radio_card;
    ui.top_info.icon_radio = radio_card;

    lv_obj_t* radio_img = lv_img_create(radio_card);
    lv_img_set_src(radio_img, resource_pool::get_image("radio"));
    lv_obj_set_pos(radio_img, 19, 5);
    lv_obj_set_style_img_recolor(radio_img, lv_color_hex(0x606060), 0);
    lv_obj_set_style_img_recolor_opa(radio_img, LV_OPA_COVER, 0);
    ui.top_info.radio_img = radio_img;

    lv_obj_t* radio_label = lv_label_create(radio_card);
    lv_obj_remove_style_all(radio_label);
    lv_obj_set_style_text_font(radio_label, font_label, 0);
    lv_obj_set_style_text_color(radio_label, lv_color_hex(0x606060), 0);
    lv_label_set_text(radio_label, "OFF");
    lv_obj_align(radio_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    ui.top_info.radio_label = radio_label;

    lv_obj_add_event_cb(radio_card, radio_draw_event_cb, LV_EVENT_DRAW_POST, this);

    // 工作模式卡片（图片 + 手绘信标/追踪动效）
    lv_obj_t* mode_card = lv_obj_create(cont);
    lv_obj_remove_style_all(mode_card);
    lv_obj_clear_flag(mode_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(mode_card, 54, 44);
    lv_obj_set_pos(mode_card, 222, 5);
    lv_obj_set_style_bg_color(mode_card, style::kCard, 0);
    lv_obj_set_style_bg_opa(mode_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(mode_card, style::kBorder, 0);
    lv_obj_set_style_border_width(mode_card, 1, 0);
    lv_obj_set_style_radius(mode_card, 8, 0);
    ui.top_info.mode_card = mode_card;
    ui.top_info.icon_mode = mode_card;

    lv_obj_t* mode_img = lv_img_create(mode_card);
    lv_img_set_src(mode_img, resource_pool::get_image("rover"));
    lv_obj_set_pos(mode_img, 19, 5);
    lv_obj_set_style_img_recolor(mode_img, lv_color_hex(0x28c76f), 0);
    lv_obj_set_style_img_recolor_opa(mode_img, LV_OPA_COVER, 0);
    ui.top_info.mode_img = mode_img;

    lv_obj_t* mode_label = lv_label_create(mode_card);
    lv_obj_remove_style_all(mode_label);
    lv_obj_set_style_text_font(mode_label, font_label, 0);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0x28c76f), 0);
    lv_label_set_text(mode_label, "ROVER");
    lv_obj_align(mode_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    ui.top_info.mode_label = mode_label;

    lv_obj_add_event_cb(mode_card, mode_draw_event_cb, LV_EVENT_DRAW_POST, this);
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
    if (x < 0 && y == 0) {
        lv_obj_align(obj, LV_ALIGN_CENTER, x, 0);
    } else if (x > 0 && y == 0) {
        lv_obj_align(obj, LV_ALIGN_CENTER, x, 0);
    } else {
        lv_obj_set_pos(obj, x, y);
    }
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
    last_device_ = device;

    ui.top_info.satellite_used->set_value(device.satellites_valid ? device.satellites_used : 0U);
    ui.top_info.satellite_tracked->set_value(device.satellites_valid ? device.satellites_tracked : 0U);
    for (auto* count : {ui.top_info.satellite_used->get_cont(), ui.top_info.satellite_tracked->get_cont(),
                        ui.top_info.satellite_separator}) {
        if (device.satellites_valid)
            lv_obj_clear_flag(count, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(count, LV_OBJ_FLAG_HIDDEN);
    }
    if (device.satellites_valid)
        lv_obj_add_flag(ui.top_info.satellite_missing, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(ui.top_info.satellite_missing, LV_OBJ_FLAG_HIDDEN);

    const bool rover = device.work_valid && device.work_mode == DataProc::WorkMode::Rover;
    const bool radio_on = device.work_valid && device.radio_on;

    // 工作模式与电台共用状态色；无有效数据时明确显示未知。
    const auto active_color = rover ? style::kSuccess : style::kInfo;
    const auto mode_color = device.work_valid ? active_color : style::kMuted;
    lv_img_set_src(ui.top_info.mode_img, resource_pool::get_image(rover ? "rover" : "base"));
    lv_obj_set_style_img_recolor(ui.top_info.mode_img, mode_color, 0);
    lv_obj_set_style_img_recolor_opa(ui.top_info.mode_img, LV_OPA_COVER, 0);
    lv_label_set_text(ui.top_info.mode_label, device.work_valid ? (rover ? "ROVER" : "BASE") : "--");
    lv_obj_set_style_text_color(ui.top_info.mode_label, mode_color, 0);
    lv_obj_set_style_border_color(
        ui.top_info.mode_card, device.work_valid ? lv_color_mix(active_color, style::kCard, LV_OPA_30) : style::kBorder,
        0);

    const auto radio_color = radio_on ? active_color : style::kMuted;
    lv_obj_set_style_img_recolor(ui.top_info.radio_img, radio_color, 0);
    lv_obj_set_style_img_recolor_opa(ui.top_info.radio_img, LV_OPA_COVER, 0);
    lv_label_set_text(ui.top_info.radio_label, !device.work_valid ? "--" : radio_on ? (rover ? "RX" : "TX") : "OFF");
    lv_obj_set_style_text_color(ui.top_info.radio_label, radio_color, 0);
    lv_obj_set_style_border_color(ui.top_info.radio_card,
                                  radio_on ? lv_color_mix(active_color, style::kCard, LV_OPA_30) : style::kBorder, 0);

    // 首页始终是记录管理入口；红色边线只提示记录状态。
    const bool recording = device.record_valid && device.recording;
    lv_obj_set_style_border_color(ui.btn_cont.btn_rec, recording ? style::kDanger : style::kBorder, 0);
}

void DialplateView::radio_draw_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_DRAW_POST)
        return;
    auto* self = static_cast<DialplateView*>(lv_event_get_user_data(e));
    if (!self)
        return;

    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
    lv_obj_t* obj = lv_event_get_target(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    // 卡片尺寸 54x44，天线顶点中心位于 (cx, cy)
    const lv_coord_t cx = coords.x1 + 27;
    const lv_coord_t cy = coords.y1 + 10;
    const lv_point_t center = {cx, cy};

    const bool radio_on = self->last_device_.work_valid && self->last_device_.radio_on;
    const bool rover = self->last_device_.work_valid && self->last_device_.work_mode == DataProc::WorkMode::Rover;
    const lv_color_t color = !radio_on ? style::kBorder : rover ? style::kSuccess : style::kInfo;

    constexpr uint16_t radii[3] = {6, 10, 14};

    for (int i = 0; i < 3; ++i) {
        lv_opa_t opa = LV_OPA_MIN;
        if (radio_on) {
            // TX(基站)：外向扩散波；RX(移动站)：内向接收波
            const uint32_t shift = rover ? static_cast<uint32_t>((2 - i) * 300) : static_cast<uint32_t>(i * 300);
            const uint32_t p = (self->anim_phase_ + shift) % 900;
            if (p < 450) {
                opa = (p < 225) ? static_cast<lv_opa_t>(p * 220 / 225 + 35)
                                : static_cast<lv_opa_t>((450 - p) * 220 / 225 + 35);
            } else {
                opa = LV_OPA_20;
            }
        } else {
            opa = LV_OPA_10;
        }

        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = color;
        arc_dsc.width = 2;
        arc_dsc.opa = opa;
        arc_dsc.rounded = 1;

        // 左弧（235°~305°）与右弧（55°~125°）
        lv_draw_arc(draw_ctx, &arc_dsc, &center, radii[i], 235, 305);
        lv_draw_arc(draw_ctx, &arc_dsc, &center, radii[i], 55, 125);
    }
}

void DialplateView::mode_draw_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_DRAW_POST)
        return;
    auto* self = static_cast<DialplateView*>(lv_event_get_user_data(e));
    if (!self)
        return;

    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
    lv_obj_t* obj = lv_event_get_target(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    if (!self->last_device_.work_valid)
        return;

    const bool rover = self->last_device_.work_mode == DataProc::WorkMode::Rover;
    const lv_coord_t cx = coords.x1 + 27;
    const lv_coord_t cy = coords.y1 + (rover ? 7 : 8);
    const lv_point_t center = {cx, cy};

    if (!rover) {
        // 基准站：天线顶部周期性信标广播脉冲
        const uint32_t p = self->anim_phase_ % 1200;
        const uint16_t r = static_cast<uint16_t>(3 + p * 8 / 1200);
        const lv_opa_t opa = static_cast<lv_opa_t>((1200 - p) * 200 / 1200);

        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = style::kInfo;
        arc_dsc.width = 1;
        arc_dsc.opa = opa;
        arc_dsc.rounded = 1;
        lv_draw_arc(draw_ctx, &arc_dsc, &center, r, 0, 360);

        // 信标中心核心光点
        lv_draw_rect_dsc_t dot_dsc;
        lv_draw_rect_dsc_init(&dot_dsc);
        dot_dsc.bg_color = style::kInfo;
        dot_dsc.radius = LV_RADIUS_CIRCLE;
        const uint32_t breath = (p < 600) ? p : (1200 - p);
        dot_dsc.bg_opa = static_cast<lv_opa_t>(140 + breath * 110 / 600);
        const lv_area_t dot_area = {static_cast<lv_coord_t>(cx - 2), static_cast<lv_coord_t>(cy - 2),
                                    static_cast<lv_coord_t>(cx + 2), static_cast<lv_coord_t>(cy + 2)};
        lv_draw_rect(draw_ctx, &dot_dsc, &dot_area);
    } else {
        // 移动站：RTK 卫星追踪锁定动态十字环
        const uint32_t p = self->anim_phase_ % 1200;
        const uint32_t breath = (p < 600) ? p : (1200 - p);
        const lv_opa_t opa = static_cast<lv_opa_t>(90 + breath * 150 / 600);

        lv_draw_arc_dsc_t arc_dsc;
        lv_draw_arc_dsc_init(&arc_dsc);
        arc_dsc.color = style::kSuccess;
        arc_dsc.width = 1;
        arc_dsc.opa = opa;
        arc_dsc.rounded = 1;

        const uint16_t rot = static_cast<uint16_t>(p * 360 / 1200);
        lv_draw_arc(draw_ctx, &arc_dsc, &center, 6, (rot + 30) % 360, (rot + 90) % 360);
        lv_draw_arc(draw_ctx, &arc_dsc, &center, 6, (rot + 210) % 360, (rot + 270) % 360);

        // 锁定中心光点
        lv_draw_rect_dsc_t dot_dsc;
        lv_draw_rect_dsc_init(&dot_dsc);
        dot_dsc.bg_color = style::kSuccess;
        dot_dsc.radius = LV_RADIUS_CIRCLE;
        dot_dsc.bg_opa = opa;
        const lv_area_t dot_area = {static_cast<lv_coord_t>(cx - 1), static_cast<lv_coord_t>(cy - 1),
                                    static_cast<lv_coord_t>(cx + 1), static_cast<lv_coord_t>(cy + 1)};
        lv_draw_rect(draw_ctx, &dot_dsc, &dot_area);
    }
}
