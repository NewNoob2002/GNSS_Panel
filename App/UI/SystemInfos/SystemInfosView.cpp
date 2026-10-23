#include "FixedFormat.h"
#include "Utils/lv_ext/lv_label_format.h"
#include "SystemInfosView.h"
#include <cstdio>
#include <cstring>
#include "UI/Common/PageStyle.h"

using namespace page;

#define ITEM_HEIGHT_MIN (LV_VER_RES - 8)
#define ITEM_PAD        4

void SystemInfosView::create(lv_obj_t* root) {
    root_ = root;
    lv_obj_set_style_outline_width(root, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(root, 0, LV_STATE_FOCUSED);
    lv_obj_set_scrollable(root, true);
    lv_obj_set_scroll_dir(root, LV_DIR_VER);
    lv_obj_set_style_width(root, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(root, style::kMuted, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(root, LV_OPA_50, LV_PART_SCROLLBAR);
    lv_obj_set_style_pad_ver(root, ITEM_PAD, 0);
    lv_obj_set_style_pad_row(root, 2, 0);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    style_init();

    /* Item workmode */
    item_create(&ui.work, root, i18n::text(i18n::TextId::SystemWorkTitle), "workmode",
                i18n::text(i18n::TextId::SystemWorkInfo));

    /* Item GPS */
    item_create(&ui.gps, root, i18n::text(i18n::TextId::SystemGpsTitle), "map_location",
                i18n::text(i18n::TextId::SystemGpsInfo));

    /* Item Wi-Fi */
    item_create(&ui.wifi, root, i18n::text(i18n::TextId::SystemWifiTitle), "wifi",
                i18n::text(i18n::TextId::SystemWifiInfo));

    /* Item Battery */
    item_create(&ui.battery, root, i18n::text(i18n::TextId::SystemBatteryTitle), "battery_info",
                i18n::text(i18n::TextId::SystemBatteryInfo));

    /* Item Storage */
    item_create(&ui.storage, root, i18n::text(i18n::TextId::SystemStorageTitle), "storage",
                i18n::text(i18n::TextId::SystemStorageInfo));

    /* Item System */
    item_create(&ui.system, root, i18n::text(i18n::TextId::SystemTitle), "system_info",
                i18n::text(i18n::TextId::SystemInfo));
    apply_language();
}

std::array<lv_obj_t*, 6> SystemInfosView::controls() const {
    return {ui.work.icon, ui.gps.icon, ui.wifi.icon, ui.battery.icon, ui.storage.icon, ui.system.icon};
}

void SystemInfosView::group_init() {
    lv_group_t* group = lv_group_get_default();
    lv_group_set_wrap(group, true);
    lv_group_set_focus_cb(group, on_focus);

    // 根页面是进入时的焦点，其后按可见顺序翻阅六个信息项。
    lv_group_add_obj(group, root_);
    for (auto* control : controls())
        lv_group_add_obj(group, control);
    lv_group_focus_obj(root_);
    lv_obj_scroll_to_y(root_, 0, LV_ANIM_OFF);
}

void SystemInfosView::destroy() {
    unbind();
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    style_reset();
}

bool SystemInfosView::bind(lv_subject_t* subject) {
    unbind();
    observer_ = lv_subject_add_observer(subject, on_state, this);
    return observer_ != nullptr;
}

void SystemInfosView::unbind() {
    if (observer_) {
        lv_observer_delete(observer_);
        observer_ = nullptr;
    }
}

void SystemInfosView::on_state(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<SystemInfosView*>(lv_observer_get_user_data(observer));
    self->update(*static_cast<const SystemInfosState*>(lv_subject_get_pointer(subject)));
}

void SystemInfosView::set_scroll_to_y(lv_obj_t* obj, int32_t y, lv_anim_enable_t en) {
    const int32_t scroll_y = lv_obj_get_scroll_y(obj);
    const int32_t diff = static_cast<int32_t>(-y + scroll_y);

    lv_obj_scroll_by(obj, 0, diff, en);
}

void SystemInfosView::on_focus(lv_group_t* g) {
    auto* focused = lv_group_get_focused(g);
    if (lv_obj_get_parent(focused) == lv_screen_active())
        lv_obj_scroll_to_y(focused, 0, LV_ANIM_ON);
    else
        lv_obj_scroll_to_view(lv_obj_get_parent(focused), LV_ANIM_ON);
}

void SystemInfosView::style_init() {
    lv_style_init(&style_.icon);
    lv_style_set_width(&style_.icon, 260);
    lv_style_set_height(&style_.icon, ITEM_HEIGHT_MIN);
    lv_style_set_bg_color(&style_.icon, style::kCard);
    lv_style_set_bg_opa(&style_.icon, LV_OPA_COVER);
    lv_style_set_text_font(&style_.icon, resource_pool::get_font("oswaldBold_18"));
    lv_style_set_text_color(&style_.icon, style::kText);

    lv_style_init(&style_.focus);
    lv_style_set_width(&style_.focus, 70);
    lv_style_set_height(&style_.focus, ITEM_HEIGHT_MIN - 16);
    lv_style_set_border_side(&style_.focus, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style_.focus, 2);
    lv_style_set_border_color(&style_.focus, style::kAccent);

    static constexpr lv_style_prop_t style_prop[] = {LV_STYLE_WIDTH, LV_STYLE_PROP_INV};

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(&trans, style_prop, lv_anim_path_overshoot, 200, 0, nullptr);
    lv_style_set_transition(&style_.focus, &trans);
    lv_style_set_transition(&style_.icon, &trans);

    lv_style_init(&style_.info);
    lv_style_set_text_font(&style_.info, resource_pool::get_font("oswaldBold_12"));
    lv_style_set_text_color(&style_.info, style::kMuted);

    lv_style_init(&style_.data);
    lv_style_set_text_font(&style_.data, resource_pool::get_font("oswaldBold_12"));
    lv_style_set_text_color(&style_.data, style::kText);
}

void SystemInfosView::style_reset() {
    lv_style_reset(&style_.icon);
    lv_style_reset(&style_.info);
    lv_style_reset(&style_.data);
    lv_style_reset(&style_.focus);
}

void SystemInfosView::item_create(item_t* item, lv_obj_t* par, const char* name, const char* img_src,
                                  const char* infos) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, ITEM_HEIGHT_MIN);
    style::panel(cont);

    lv_obj_set_scrollable(cont, false);
    lv_obj_set_event_bubble(cont, true);
    item->cont = cont;

    /* icon */
    lv_obj_t* icon = lv_obj_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(icon);
    lv_obj_set_scrollable(icon, false);
    lv_obj_set_event_bubble(icon, true);

    lv_obj_add_style(icon, &style_.icon, 0);
    lv_obj_add_style(icon, &style_.focus, LV_STATE_FOCUSED);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(icon, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* img = lv_image_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_image_set_src(img, resource_pool::get_image(img_src));

    lv_obj_t* label = lv_label_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, name);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    item->label_name = label;
    item->icon = icon;

    /* infos */
    lv_obj_t* info_label = lv_label_create(cont);
    lv_obj_remove_style_all(info_label);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(info_label, infos);
    lv_obj_add_style(info_label, &style_.info, 0);
    lv_obj_align(info_label, LV_ALIGN_LEFT_MID, 85, 0);
    item->label_info = info_label;

    /* datas */
    lv_obj_t* data_label = lv_label_create(cont);
    lv_obj_remove_style_all(data_label);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(data_label, "");
    lv_obj_set_width(data_label, 111);
    lv_obj_add_style(data_label, &style_.data, 0);
    lv_obj_align(data_label, LV_ALIGN_LEFT_MID, 175, 0);
    item->label_data = data_label;

    auto* demo = page::style::mark_demo(lv_label_create(cont));
    lv_obj_add_style(demo, &style_.info, 0);
    lv_label_set_text(demo, "DEMO");
    // Clear the root's vertical scrollbar (drawn over the full-width cards' right edge).
    lv_obj_align(demo, LV_ALIGN_TOP_RIGHT, -15, 4);

    lv_obj_move_to_index(icon, -1);
    lv_obj_move_to_index(demo, -1);
    lv_obj_enable_style_refresh(true);

    /* get real max height */
    lv_obj_update_layout(item->label_info);
    int32_t height = lv_obj_get_height(item->label_info);
    height = LV_MAX(height, ITEM_HEIGHT_MIN);
    lv_obj_set_height(cont, height);
    lv_obj_set_height(icon, height);
}

void SystemInfosView::apply_language() const {
    lv_label_set_text(ui.work.label_name, i18n::text(i18n::TextId::SystemWorkTitle));
    lv_label_set_text(ui.work.label_info, i18n::text(i18n::TextId::SystemWorkInfo));
    lv_label_set_text(ui.gps.label_name, i18n::text(i18n::TextId::SystemGpsTitle));
    lv_label_set_text(ui.gps.label_info, i18n::text(i18n::TextId::SystemGpsInfo));
    lv_label_set_text(ui.wifi.label_name, i18n::text(i18n::TextId::SystemWifiTitle));
    lv_label_set_text(ui.wifi.label_info, i18n::text(i18n::TextId::SystemWifiInfo));
    lv_label_set_text(ui.battery.label_name, i18n::text(i18n::TextId::SystemBatteryTitle));
    lv_label_set_text(ui.battery.label_info, i18n::text(i18n::TextId::SystemBatteryInfo));
    lv_label_set_text(ui.storage.label_name, i18n::text(i18n::TextId::SystemStorageTitle));
    lv_label_set_text(ui.storage.label_info, i18n::text(i18n::TextId::SystemStorageInfo));
    lv_label_set_text(ui.system.label_name, i18n::text(i18n::TextId::SystemTitle));
    lv_label_set_text(ui.system.label_info, i18n::text(i18n::TextId::SystemInfo));
}

void SystemInfosView::update(const SystemInfosState& state) const {
    using i18n::TextId;
    const char* na = i18n::text(TextId::NotAvailable);
    const auto on_off = [](bool on) {
        return i18n::text(on ? TextId::StatusOn : TextId::StatusOff);
    };
    if (state.work.valid) {
        const char* ntrip = state.work.ntrip == hal::NtripRole::Server   ? i18n::text(TextId::NtripServer)
                            : state.work.ntrip == hal::NtripRole::Client ? i18n::text(TextId::NtripClient)
                                                                         : na;
        app_label_set_text_fmt(
            ui.work.label_data, "%s\n%s\n%s",
            i18n::text(state.work.mode == hal::WorkMode::Rover ? TextId::WorkModeRover : TextId::WorkModeBase),
            on_off(state.work.radio_on), ntrip);
    } else {
        app_label_set_text_fmt(ui.work.label_data, "%s\n%s\n%s", na, na, na);
    }
    if (state.gnss.position_valid)
        app_label_set_text_fmt(ui.gps.label_data, "%s\n%s\n%s m", app::Fixed(state.gnss.latitude, 6).c_str(),
                              app::Fixed(state.gnss.longitude, 6).c_str(), app::Fixed(state.gnss.altitude, 2).c_str());
    else
        app_label_set_text_fmt(ui.gps.label_data, "%s\n%s\n%s", na, na, na);
    const char* wifi_state = state.wifi.pending ? "..." : on_off(state.wifi.on);
    if (state.wifi.valid)
        app_label_set_text_fmt(ui.wifi.label_data, "%s-%s\n%s\n%u.%u.%u.%u", state.wifi.station ? "STA" : "AP",
                              state.wifi.ssid.data(), wifi_state, state.wifi.ip[0], state.wifi.ip[1],
                              state.wifi.ip[2], state.wifi.ip[3]);
    else
        app_label_set_text_fmt(ui.wifi.label_data, "%s\n%s\n%s", na, wifi_state, na);
    if (state.battery.valid) {
        const TextId charge = state.battery.external_power                         ? TextId::BatteryExternalPower
                              : state.battery.charge == hal::ChargeState::NotCharging ? TextId::BatteryNotCharge
                              : state.battery.charge == hal::ChargeState::Fast        ? TextId::BatteryFastCharge
                                                                                    : TextId::BatteryNormalCharge;
        app_label_set_text_fmt(ui.battery.label_data, "%s%%\n%s V\n%s C\n%s",
                              app::Fixed(state.battery.gauge_percent, 1).c_str(), app::Fixed(state.battery.voltage, 2).c_str(),
                              app::Fixed(state.battery.temperature, 1).c_str(), i18n::text(charge));
    } else {
        app_label_set_text_fmt(ui.battery.label_data, "%s\n%s\n%s\n%s", na, na, na, na);
    }
    if (state.recorder.valid && state.recorder.recording) {
        char type[16], interval[16];
        option_text(TextId::RecordTypeOptions, state.recorder.type, type, sizeof(type));
        option_text(TextId::RecordIntervalOptions, state.recorder.interval, interval, sizeof(interval));
        app_label_set_text_fmt(ui.storage.label_data, "%s\n%s\n%s MB\n%s\n%s", on_off(true), state.recorder.name.data(),
                              app::Fixed(state.recorder.space_mb, 1).c_str(), type, interval);
    } else {
        app_label_set_text_fmt(ui.storage.label_data, "%s\n-\n-\n-\n-", state.recorder.valid ? on_off(false) : na);
    }
    unsigned errors = 0;
    for (const auto& result : state.init.results)
        errors += result.status == DataProc::InitializationStatus::Failed;
    const auto uptime = state.version.uptime_s;
    app_label_set_text_fmt(ui.system.label_data, "%s\n%02u:%02u:%02u\n%u\n%s",
                          state.version.firmware[0] ? state.version.firmware.data() : na, static_cast<unsigned>(uptime / 3600U),
                          static_cast<unsigned>(uptime / 60U % 60U), static_cast<unsigned>(uptime % 60U), errors,
                          state.version.build[0] ? state.version.build.data() : na);
}
