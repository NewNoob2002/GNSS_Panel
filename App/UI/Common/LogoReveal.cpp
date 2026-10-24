#include "LogoReveal.h"
#include <algorithm>
#include "PageStyle.h"

namespace app {
namespace {
constexpr int32_t kHighlightWidth = 22;
constexpr int32_t kWipeEdge = 16;
/** @brief 使用 LVGL 原生 ease-out 的端点与曲线，避免增添逐字动画分配。 */
int32_t ease(uint32_t elapsed, uint32_t duration) {
    if (!duration || elapsed >= duration)
        return 255;
    lv_anim_t curve;
    lv_anim_init(&curve);
    curve.start_value = 0;
    curve.end_value = 255;
    curve.duration = duration;
    curve.act_time = static_cast<int32_t>(elapsed);
    return lv_anim_path_ease_out(&curve);
}
} // namespace

LogoReveal::~LogoReveal() { destroy(); }

void LogoReveal::create(lv_obj_t* parent, const logo_parts_t& logo) {
    destroy();
    logo_ = &logo;
    timeline_ = logo.effect;
    container_ = page::style::box(parent, 0, 0, 294, 126);
    lv_obj_set_style_bg_color(container_, lv_color_hex(logo.layout.background), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);
    for (unsigned i = 0; i < logo.part_count; ++i) {
        const auto& part = logo.parts[i];
        auto* image = lv_image_create(container_);
        lv_image_set_src(image, part.image);
        lv_obj_set_pos(image, logo.layout.x + part.x, logo.layout.y + part.y);
        if (logo.recolor) {
            lv_obj_set_style_image_recolor(image, lv_color_hex(logo.layout.foreground), 0);
            lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
        }
        lv_obj_set_style_image_opa(image, LV_OPA_TRANSP, 0);
        lv_obj_set_style_translate_y(image, timeline_.rise_px, 0);
    }
    lv_obj_add_event_cb(container_, draw_highlight, LV_EVENT_DRAW_POST, this);
    lv_obj_add_event_cb(container_, [](lv_event_t* event) {
        auto* self = static_cast<LogoReveal*>(lv_event_get_user_data(event));
        self->stop();
        self->container_ = nullptr;
        self->logo_ = nullptr;
    }, LV_EVENT_DELETE, this);
}

void LogoReveal::play() {
    if (!container_)
        return;
    stop();
    lv_obj_set_hidden(container_, false);
    lv_obj_set_style_opa(container_, LV_OPA_COVER, 0);
    apply(0);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, this);
    lv_anim_set_exec_cb(&animation, tick);
    lv_anim_set_values(&animation, 0, static_cast<int32_t>(timeline_.total_ms));
    lv_anim_set_duration(&animation, timeline_.total_ms);
    /** @brief 直接使用实际毫秒，避免线性路径的量化把 1200 ms 高光清理推迟一帧。 */
    lv_anim_set_path_cb(&animation, [](const lv_anim_t* anim) { return anim->act_time; });
    if (!lv_anim_start(&animation)) {
        apply(timeline_.total_ms);
        LV_LOG_ERROR("LogoReveal: animation allocation failed; applied endpoint");
    }
}

void LogoReveal::stop() { lv_anim_delete(this, tick); }

void LogoReveal::destroy() {
    stop();
    if (container_) {
        lv_obj_delete(container_);
        container_ = nullptr;
    }
    logo_ = nullptr;
}

void LogoReveal::tick(void* self, int32_t ms) {
    static_cast<LogoReveal*>(self)->apply(static_cast<uint32_t>(ms));
}

void LogoReveal::apply(uint32_t ms) {
    const bool final = ms >= timeline_.total_ms;
    const uint32_t bg = timeline_.background_ms;
    const auto background = static_cast<lv_opa_t>(final || !bg || ms >= bg ? 255 : ms * 255 / bg);
    /** @brief 复用 LVGL 本地样式作为上一帧缓存，避免重复失效整屏且不限制零件数。 */
    if (lv_obj_get_style_bg_opa(container_, LV_PART_MAIN) != background)
        lv_obj_set_style_bg_opa(container_, background, 0);
    for (unsigned i = 0; i < logo_->part_count; ++i) {
        const auto& part = logo_->parts[i];
        const auto* group = logo_->groups && part.group < logo_->group_count ? &logo_->groups[part.group] : nullptr;
        const uint32_t delay = group && group->delay_ms >= 0 ? static_cast<uint32_t>(group->delay_ms)
                                                                           : part.group * timeline_.group_interval_ms;
        const uint32_t duration = group && group->duration_ms ? group->duration_ms : timeline_.group_duration_ms;
        const uint32_t start = bg + (timeline_.preset == LOGO_EFFECT_CASCADE ? delay : 0);
        const uint32_t time = timeline_.preset == LOGO_EFFECT_CASCADE ? duration : timeline_.group_duration_ms;
        const int32_t mix = final ? 255 : ms <= start ? 0
                                         : timeline_.preset == LOGO_EFFECT_WIPE ? 255 : ease(ms - start, time);
        auto* image = lv_obj_get_child(container_, static_cast<int32_t>(i));
        if (lv_obj_get_style_image_opa(image, LV_PART_MAIN) != mix)
            lv_obj_set_style_image_opa(image, static_cast<lv_opa_t>(mix), 0);
        const int32_t translate = timeline_.rise_px * (255 - mix) / 255;
        if (lv_obj_get_style_translate_y(image, LV_PART_MAIN) != translate)
            lv_obj_set_style_translate_y(image, translate, 0);
    }
    bool wipe_changed = false;
    if (timeline_.preset == LOGO_EFFECT_WIPE) {
        const int32_t front = final ? logo_->width + kWipeEdge : ms <= bg ? 0
            : static_cast<int32_t>(std::min<uint32_t>(logo_->width + kWipeEdge,
                (ms - bg) * (logo_->width + kWipeEdge) / timeline_.group_duration_ms));
        wipe_changed = front != wipe_front_;
        wipe_front_ = front;
    }
    const bool was_highlighting = highlighting_;
    const int32_t old_highlight_x = highlight_x_;
    highlighting_ = !final && logo_->highlight && timeline_.highlight && timeline_.highlight_duration_ms
                    && ms >= timeline_.highlight_start_ms
                    && ms - timeline_.highlight_start_ms < timeline_.highlight_duration_ms;
    if (highlighting_)
        highlight_x_ = static_cast<int32_t>((ms - timeline_.highlight_start_ms)
                        * (logo_->width + kHighlightWidth) / timeline_.highlight_duration_ms) - kHighlightWidth;
    if (wipe_changed || highlighting_ != was_highlighting || (highlighting_ && highlight_x_ != old_highlight_x)) {
        lv_area_t area;
        lv_obj_get_coords(container_, &area);
        area.x1 += logo_->layout.x;
        area.y1 += logo_->layout.y;
        area.x2 = area.x1 + logo_->width - 1;
        area.y2 = area.y1 + logo_->height - 1;
        lv_obj_invalidate_area(container_, &area);
    }
}

/** @brief 固定零件布局；wipe 用未显现区的底色矩形及 16 列柔边，无裁剪对象或透明图层。 */
void LogoReveal::draw_highlight(lv_event_t* event) {
    auto* self = static_cast<LogoReveal*>(lv_event_get_user_data(event));
    const auto& logo = *self->logo_;
    const bool wipe = self->timeline_.preset == LOGO_EFFECT_WIPE
                      && lv_obj_get_style_bg_opa(self->container_, LV_PART_MAIN) == LV_OPA_COVER
                      && self->wipe_front_ < logo.width + kWipeEdge;
    if (!self->highlighting_ && !wipe)
        return;
    lv_area_t parent;
    lv_obj_get_coords(self->container_, &parent);
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    if (wipe) {
        rect.bg_color = lv_color_hex(logo.layout.background);
        rect.bg_opa = LV_OPA_COVER;
        if (self->wipe_front_ < logo.width) {
            lv_area_t covered = {parent.x1 + logo.layout.x + self->wipe_front_, parent.y1 + logo.layout.y,
                                 parent.x1 + logo.layout.x + logo.width - 1,
                                 parent.y1 + logo.layout.y + logo.height - 1};
            lv_draw_rect(lv_event_get_layer(event), &rect, &covered);
        }
        for (int32_t col = 0; col < kWipeEdge; ++col) {
            const int32_t x = self->wipe_front_ - kWipeEdge + col;
            if (x < 0 || x >= logo.width)
                continue;
            rect.bg_opa = static_cast<lv_opa_t>(col * 255 / (kWipeEdge - 1));
            lv_area_t stripe = {parent.x1 + logo.layout.x + x, parent.y1 + logo.layout.y,
                               parent.x1 + logo.layout.x + x, parent.y1 + logo.layout.y + logo.height - 1};
            lv_draw_rect(lv_event_get_layer(event), &rect, &stripe);
        }
    }
    if (!self->highlighting_)
        return;
    rect.bg_color = lv_color_white();
    for (int32_t col = 0; col < kHighlightWidth; ++col) {
        const int32_t x = self->highlight_x_ + col;
        if (x < 0 || x >= logo.width)
            continue;
        rect.bg_opa = static_cast<lv_opa_t>(88 * std::min(col, kHighlightWidth - 1 - col) / (kHighlightWidth / 2));
        lv_area_t stripe = {parent.x1 + logo.layout.x + x, parent.y1 + logo.layout.y,
                            parent.x1 + logo.layout.x + x, parent.y1 + logo.layout.y + logo.height - 1};
        lv_draw_rect(lv_event_get_layer(event), &rect, &stripe);
    }
}
} // namespace app
