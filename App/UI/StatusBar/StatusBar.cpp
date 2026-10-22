#include <memory>
#include "StatusBar.h"

#include "UI/Resource/ResourcePool.h"
#include "UI/Common/BatteryStyle.h"
#include "Log.h"
#include "UI/Common/SettingsStyle.h"

namespace page {

namespace {
constexpr int32_t kHeight = 26, kMargin = 6, kGap = 3;
// SD 图标上移 22 px 即完全离开 26 px 高的分组（分组裁剪子对象），在栏外完成换色。
constexpr int32_t kSdTravel = 22;
constexpr uint32_t kSdOutMs = 160, kSdInMs = 220, kSdBlinkMs = 500;

// 8-bit alpha 图标由 recolor 着色；图片对象便于后续做透明度/缩放/换图动画。
lv_obj_t* icon(lv_obj_t* parent, const char* name, lv_color_t color = style::kText) {
    lv_obj_t* image = lv_image_create(parent);
    lv_image_set_src(image, resource_pool::get_image(name));
    lv_obj_set_style_image_recolor(image, color, 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    return image;
}

// 左右两组按实际内容宽度紧凑排列并垂直居中；隐藏的子对象不占位，数值位数变化时不留空隙。
lv_obj_t* group(lv_obj_t* parent, lv_align_t align, int32_t x) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_size(row, LV_SIZE_CONTENT, kHeight);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, kGap, 0);
    lv_obj_align(row, align, x, 0);
    return row;
}

// 组内语义分隔：与两侧 kGap 合计约 10 px。
void spacer(lv_obj_t* parent) {
    lv_obj_t* gap = lv_obj_create(parent);
    lv_obj_remove_style_all(gap);
    lv_obj_set_size(gap, 4, 1);
}

// NumberFlow 隐藏的高位不再保留固定宽度，数字紧贴左右邻居。
void compact(const NumberFlow* number) {
    lv_obj_t* cont = number->get_cont();
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(cont, 0, 0);
    lv_obj_set_width(cont, LV_SIZE_CONTENT);
}
} // namespace

bool StatusBar::create(lv_obj_t* parent) {
    if (root_ || !parent)
        return false;
    account_ = std::make_unique<DataNode>("StatusBar", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !account_->subscribe("Status")) {
        account_.reset();
        return false;
    }
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, LV_HOR_RES, 26);
    lv_obj_set_y(root_, -26);
    visible_ = false;
    lv_obj_set_style_bg_color(root_, style::kBackground, 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(root_, false);

    const lv_font_t* small = resource_pool::get_font("oswaldBold_12");
    const lv_font_t* large = resource_pool::get_font("oswaldBold_18");

    // 左：卫星数 | 定位状态；中：时钟；右：SD、Wi-Fi | 电池与电量。数值统一用 18 号字。
    lv_obj_t* left = group(root_, LV_ALIGN_LEFT_MID, kMargin);
    icon(left, "ui_satellite", style::kInfo);
    satellites_ = new NumberFlow(large, 2, true);
    satellites_->create(left);
    compact(satellites_);
    spacer(left);
    position_icon_ = icon(left, "ui_location");
    position_label_ = lv_label_create(left);
    lv_obj_set_style_text_font(position_label_, small, 0);
    lv_obj_set_style_text_color(position_label_, style::kText, 0);
    lv_label_set_text(position_label_, "NONE");

    clock_ = new NumberFlowClock(large);
    clock_->create(root_);
    clock_->set_pos(LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* right = group(root_, LV_ALIGN_RIGHT_MID, -kMargin);
    sd_icon_ = icon(right, "ui_sd_card");
    lv_obj_set_user_data(sd_icon_, this);
    sd_recording_ = false;
    wifi_icon_ = icon(right, "ui_wifi");
    spacer(right);
    battery_fill_ = battery::create_slot(right, 0, 0, 20, 10);
    battery_percent_ = new NumberFlow(large, 3, true);
    battery_percent_->create(right); // 保持为组内最后一个子对象，测试据此定位电量数字。
    compact(battery_percent_);
    lv_obj_set_style_pad_left(battery_percent_->get_cont(), kGap, 0); // 电池正极凸出 1 px，数字再让开一点。

    update({});
    lv_obj_update_layout(root_); // 创建后坐标即有效，调用方可立即读取滑入起点。
    return true;
}

void StatusBar::set_y(void* obj, int32_t y) {
    lv_obj_set_y(static_cast<lv_obj_t*>(obj), static_cast<int32_t>(y));
}

bool StatusBar::on_presentation(void* owner, const DataProc::StatusBarPresentation& request) {
    auto* self = static_cast<StatusBar*>(owner);
    if (!self->root_)
        return false;
    switch (request.style) {
        case DataProc::StatusBarStyle::Default:
            lv_obj_set_style_bg_opa(self->root_, LV_OPA_COVER, 0);
            break;
        case DataProc::StatusBarStyle::Transparent:
            lv_obj_set_style_bg_opa(self->root_, LV_OPA_TRANSP, 0);
            break;
        default:
            return false;
    }
    // HC32 slide behavior; reverse from the current position, never jump or restart an identical request.
    if (self->visible_ == request.visible)
        return true;
    self->visible_ = request.visible;
    lv_anim_delete(self->root_, set_y);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, self->root_);
    lv_anim_set_exec_cb(&animation, set_y);
    lv_anim_set_values(&animation, lv_obj_get_y(self->root_), request.visible ? 0 : -26);
    lv_anim_set_duration(&animation, 500);
    lv_anim_set_path_cb(&animation, request.visible ? lv_anim_path_ease_out : lv_anim_path_overshoot);
    if (!lv_anim_start(&animation)) {
        set_y(self->root_, request.visible ? 0 : -26);
        APP_LOG_E("StatusBar", "slide allocation failed; applied endpoint without animation");
    }
    return true;
}

int StatusBar::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<StatusBar*>(account->getUserData());
    if (!event)
        return DataNode::RES_PARAM_ERROR;
    if (event->event == DataNode::EVENT_NOTIFY) {
        DataProc::StatusBarPresentation request;
        const int result = DataProc::ReadPayload(event, request);
        if (result != DataNode::RES_OK)
            return result;
        return on_presentation(self, request) ? DataNode::RES_OK : DataNode::RES_PARAM_ERROR;
    }
    if (event->event != DataNode::EVENT_PUBLISH || !event->tran || std::strcmp(event->tran->getID(), "Status") != 0)
        return DataNode::RES_UNSUPPORTED_REQUEST;
    DataProc::StatusSnapshot snapshot;
    const int result = DataProc::ReadPayload(event, snapshot);
    if (result != DataNode::RES_OK)
        return result;
    StatusBarState state;
    state.position = snapshot.position.data();
    state.position_color = lv_color_hex(snapshot.position_color_rgb);
    state.satellites = snapshot.satellites;
    state.satellites_valid = snapshot.satellites_valid;
    state.battery_percent = snapshot.battery_percent;
    state.battery_valid = snapshot.battery_valid;
    state.charging = snapshot.charging;
    state.battery_voltage = snapshot.battery_voltage;
    state.hour = snapshot.hour;
    state.minute = snapshot.minute;
    state.second = snapshot.second;
    state.clock_valid = snapshot.clock_valid;
    state.wifi = snapshot.wifi;
    state.recording = snapshot.recording;
    self->update(state);
    return DataNode::RES_OK;
}

void StatusBar::update(const StatusBarState& state) {
    // 状态约 5 Hz 发布：只在显示内容变化时触碰 LVGL，避免状态栏持续重绘。
    if (state.satellites_valid)
        satellites_->set_value(state.satellites > 99 ? 99 : state.satellites);
    settings_style::set_visible(satellites_->get_cont(), state.satellites_valid);
    settings_style::set_text(position_label_, state.position ? state.position : "DEMO");
    settings_style::set_recolor(position_icon_, state.position_color);
    if (state.clock_valid)
        clock_->set_time(state.hour % 24, state.minute % 60, state.second % 60);
    settings_style::set_visible(clock_->get_cont(), state.clock_valid);
    sd_show(state.recording);
    settings_style::set_recolor(wifi_icon_, state.wifi ? style::kInfo : style::kText);
    const unsigned percent = state.battery_percent > 100 ? 100 : state.battery_percent;
    battery::update_fill(battery_fill_, percent, state.charging, state.battery_valid);
    if (state.battery_valid)
        battery_percent_->set_value(percent);
    settings_style::set_visible(battery_percent_->get_cont(), state.battery_valid);
}

void StatusBar::sd_slide(void* obj, int32_t y) {
    lv_obj_set_style_translate_y(static_cast<lv_obj_t*>(obj), static_cast<int32_t>(y), 0);
}

void StatusBar::sd_blink(void* obj, int32_t opa) {
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(opa), 0);
}

void StatusBar::sd_show(bool recording) {
    if (recording == sd_recording_)
        return;
    sd_recording_ = recording;
    lv_anim_delete(sd_icon_, sd_blink);
    sd_blink(sd_icon_, LV_OPA_COVER);
    lv_anim_delete(sd_icon_, sd_slide); // 打断进行中的滑入/滑出，从当前位置继续滑出。
    lv_anim_t out;
    lv_anim_init(&out);
    lv_anim_set_var(&out, sd_icon_);
    lv_anim_set_exec_cb(&out, sd_slide);
    lv_anim_set_values(&out, lv_obj_get_style_translate_y(sd_icon_, LV_PART_MAIN), -kSdTravel);
    lv_anim_set_duration(&out, kSdOutMs);
    lv_anim_set_path_cb(&out, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&out, sd_hidden);
    if (!lv_anim_start(&out)) {
        sd_slide(sd_icon_, -kSdTravel);
        sd_hidden(&out); // 动画分配失败：直接换色并归位。
    }
}

void StatusBar::sd_hidden(lv_anim_t* animation) {
    auto* obj = static_cast<lv_obj_t*>(animation->var);
    auto* self = static_cast<StatusBar*>(lv_obj_get_user_data(obj));
    lv_obj_set_style_image_recolor(obj, self->sd_recording_ ? style::kInfo : style::kText, 0);
    lv_anim_t in;
    lv_anim_init(&in);
    lv_anim_set_var(&in, obj);
    lv_anim_set_exec_cb(&in, sd_slide);
    lv_anim_set_values(&in, -kSdTravel, 0);
    lv_anim_set_duration(&in, kSdInMs);
    lv_anim_set_path_cb(&in, lv_anim_path_overshoot);
    lv_anim_set_completed_cb(&in, sd_entered);
    if (!lv_anim_start(&in)) {
        sd_slide(obj, 0);
        sd_entered(&in);
    }
}

void StatusBar::sd_entered(lv_anim_t* animation) {
    auto* obj = static_cast<lv_obj_t*>(animation->var);
    const auto* self = static_cast<StatusBar*>(lv_obj_get_user_data(obj));
    if (!self->sd_recording_)
        return;
    lv_anim_t blink;
    lv_anim_init(&blink);
    lv_anim_set_var(&blink, obj);
    lv_anim_set_exec_cb(&blink, sd_blink);
    lv_anim_set_values(&blink, LV_OPA_COVER, LV_OPA_30);
    lv_anim_set_duration(&blink, kSdBlinkMs);
    lv_anim_set_reverse_duration(&blink, kSdBlinkMs);
    lv_anim_set_path_cb(&blink, lv_anim_path_ease_in_out);
    lv_anim_set_repeat_count(&blink, LV_ANIM_REPEAT_INFINITE);
    if (!lv_anim_start(&blink))
        APP_LOG_W("StatusBar", "recording blink allocation failed; SD icon stays solid");
}

void StatusBar::destroy() {
    account_.reset(); // Unregister callbacks before deleting any LVGL objects.
    battery::stop(battery_fill_);
    battery_fill_ = nullptr;
    if (root_)
        lv_anim_delete(root_, set_y);
    visible_ = false;
    delete satellites_;
    delete battery_percent_;
    delete clock_;
    satellites_ = nullptr;
    battery_percent_ = nullptr;
    clock_ = nullptr;
    if (root_) {
        lv_obj_delete(root_);
        root_ = nullptr;
    }
}

} // namespace page
