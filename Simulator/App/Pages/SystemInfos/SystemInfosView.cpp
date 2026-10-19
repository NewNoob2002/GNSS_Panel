#include "SystemInfosView.h"

using namespace Page;

#define ITEM_HEIGHT_MIN 100
#define ITEM_PAD        ((LV_VER_RES - ITEM_HEIGHT_MIN) / 2)

void
SystemInfosView::Create(lv_obj_t* root) {
    lv_obj_set_style_pad_ver(root, ITEM_PAD, 0);

    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    Style_Init();

    /* Item workmode */
    Item_Create(&ui.work, root, I18n::Text(I18n::TextId::SystemWorkTitle), "workmode",
                I18n::Text(I18n::TextId::SystemWorkInfo));

    /* Item GPS */
    Item_Create(&ui.gps, root, I18n::Text(I18n::TextId::SystemGpsTitle), "map_location",
                I18n::Text(I18n::TextId::SystemGpsInfo));

    /* Item Wi-Fi */
    Item_Create(&ui.wifi, root, I18n::Text(I18n::TextId::SystemWifiTitle), "wifi",
                I18n::Text(I18n::TextId::SystemWifiInfo));

    /* Item Battery */
    Item_Create(&ui.battery, root, I18n::Text(I18n::TextId::SystemBatteryTitle), "battery_info",
                I18n::Text(I18n::TextId::SystemBatteryInfo));

    /* Item Storage */
    Item_Create(&ui.storage, root, I18n::Text(I18n::TextId::SystemStorageTitle), "storage",
                I18n::Text(I18n::TextId::SystemStorageInfo));

    /* Item System */
    Item_Create(&ui.system, root, I18n::Text(I18n::TextId::SystemTitle), "system_info",
                I18n::Text(I18n::TextId::SystemInfo));

    Group_Init();
}

void
SystemInfosView::Group_Init() {
    lv_group_t* group = lv_group_get_default();
    lv_group_set_wrap(group, true);
    lv_group_set_focus_cb(group, onFocus);

    const item_t* item_grp = reinterpret_cast<item_t*>(&ui);

    /* Reverse adding to group makes encoder operation more comfortable */
    for (int i = sizeof(ui) / sizeof(item_t) - 1; i >= 0; i--) {
        lv_group_add_obj(group, item_grp[i].icon);
    }

    lv_group_focus_obj(item_grp[0].icon);
}

void
SystemInfosView::Delete() {
    lv_group_set_focus_cb(lv_group_get_default(), nullptr);
    Style_Reset();
}

void
SystemInfosView::SetScrollToY(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en) {
    const lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
    const lv_coord_t diff = -y + scroll_y;

    lv_obj_scroll_by(obj, 0, diff, en);
}

void
SystemInfosView::onFocus(lv_group_t* g) {
    const lv_obj_t* icon = lv_group_get_focused(g);
    const lv_obj_t* cont = lv_obj_get_parent(icon);
    const lv_coord_t y = lv_obj_get_y(cont);
    lv_obj_scroll_to_y(lv_obj_get_parent(cont), y, LV_ANIM_ON);
}

void
SystemInfosView::Style_Init() {
    lv_style_init(&style.icon);
    lv_style_set_width(&style.icon, 260);
    lv_style_set_height(&style.icon, 100);
    lv_style_set_bg_color(&style.icon, lv_color_black());
    lv_style_set_bg_opa(&style.icon, LV_OPA_COVER);
    lv_style_set_text_font(&style.icon, ResourcePool::GetFont("oswaldBold_18"));
    lv_style_set_text_color(&style.icon, lv_color_white());

    lv_style_init(&style.focus);
    lv_style_set_width(&style.focus, 70);
    lv_style_set_height(&style.focus, 80);
    lv_style_set_border_side(&style.focus, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style.focus, 2);
    lv_style_set_border_color(&style.focus, lv_color_hex(0xff931e));

    static constexpr lv_style_prop_t style_prop[] = {LV_STYLE_WIDTH, LV_STYLE_PROP_INV};

    static lv_style_transition_dsc_t trans;
    lv_style_transition_dsc_init(&trans, style_prop, lv_anim_path_overshoot, 200, 0, nullptr);
    lv_style_set_transition(&style.focus, &trans);
    lv_style_set_transition(&style.icon, &trans);

    lv_style_init(&style.info);
    lv_style_set_text_font(&style.info, ResourcePool::GetFont("oswaldBold_12"));
    lv_style_set_text_color(&style.info, lv_color_hex(0x999999));

    lv_style_init(&style.data);
    lv_style_set_text_font(&style.data, ResourcePool::GetFont("oswaldBold_12"));
    lv_style_set_text_color(&style.data, lv_color_white());
}

void
SystemInfosView::Style_Reset() {
    lv_style_reset(&style.icon);
    lv_style_reset(&style.info);
    lv_style_reset(&style.data);
    lv_style_reset(&style.focus);
}

void
SystemInfosView::Item_Create(item_t* item, lv_obj_t* par, const char* name, const char* img_src, const char* infos) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 294, 100);

    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    item->cont = cont;

    /* icon */
    lv_obj_t* icon = lv_obj_create(cont);
    lv_obj_enable_style_refresh(false);
    lv_obj_remove_style_all(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_style(icon, &style.icon, 0);
    lv_obj_add_style(icon, &style.focus, LV_STATE_FOCUSED);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 10);

    lv_obj_set_flex_flow(icon, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(icon, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* img = lv_img_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_img_set_src(img, ResourcePool::GetImage(img_src));

    lv_obj_t* label = lv_label_create(icon);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(label, name);
    item->labelName = label;
    item->icon = icon;

    /* infos */
    lv_obj_t* info_label = lv_label_create(cont);
    lv_obj_remove_style_all(info_label);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(info_label, infos);
    lv_obj_add_style(info_label, &style.info, 0);
    lv_obj_align(info_label, LV_ALIGN_LEFT_MID, 85, 12);
    item->labelInfo = info_label;

    /* datas */
    lv_obj_t* data_label = lv_label_create(cont);
    lv_obj_remove_style_all(data_label);
    lv_obj_enable_style_refresh(false);
    lv_label_set_text(data_label, "-");
    lv_obj_add_style(data_label, &style.data, 0);
    lv_obj_align_to(data_label, info_label, LV_ALIGN_TOP_RIGHT, 30, 0);
    // lv_obj_align(data_label, LV_ALIGN_LEFT_MID, 145, 10);
    item->labelData = data_label;

    lv_obj_move_foreground(icon);
    lv_obj_enable_style_refresh(true);

    /* get real max height */
    lv_obj_update_layout(item->labelInfo);
    lv_coord_t height = lv_obj_get_height(item->labelInfo);
    height = LV_MAX(height, ITEM_HEIGHT_MIN);
    lv_obj_set_height(cont, height);
    lv_obj_set_height(icon, height);
}

void
SystemInfosView::SetWork(const WorkMode_t workMode, const RadioInfo_t radioInfo, const NtripInfo_t& ntripInfo) const {
    lv_label_set_text_fmt(
        ui.work.labelData,
        "%s\n"
        "%s\n"
        "%s",
        workMode == rover_mode ? I18n::Text(I18n::TextId::WorkModeRover) : I18n::Text(I18n::TextId::WorkModeBase),
        radioInfo.radio_status ? I18n::Text(I18n::TextId::StatusOn) : I18n::Text(I18n::TextId::StatusOff),
        ntripInfo.NtripServer_status   ? I18n::Text(I18n::TextId::NtripServer)
        : ntripInfo.NtripClient_status ? I18n::Text(I18n::TextId::NtripClient)
                                       : I18n::Text(I18n::TextId::NotAvailable));
}

void
SystemInfosView::SetGPS(const double lat, const double lng, const double alt) const {
    lv_label_set_text_fmt(ui.gps.labelData,
                          "%0.6f\n"
                          "%0.6f\n"
                          "%0.6fm\n",
                          lat, lng, alt);
}

void
SystemInfosView::SetWifi(const WifiInfo_t& wifiInfo) const {
    lv_label_set_text_fmt(ui.wifi.labelData,
                          "%s-%.16s\n"
                          "%s\n"
                          "%d.%d.%d.%d",
                          wifiInfo.wifi_mode ? "STA" : "AP", wifiInfo.wifi_ssid,
                          wifiInfo.wifi_status ? I18n::Text(I18n::TextId::StatusOn)
                                               : I18n::Text(I18n::TextId::StatusOff),
                          wifiInfo.wifi_ip[0], wifiInfo.wifi_ip[1], wifiInfo.wifi_ip[2], wifiInfo.wifi_ip[3]);
}

void
SystemInfosView::SetBattery(const BatteryInfo_t& batteryInfo) const {
    lv_label_set_text_fmt(ui.battery.labelData,
                          "%0.2f%%\n"
                          "%0.2fV\n"
                          "%0.2f\n"
                          "%s",
                          batteryInfo.Percent_f, batteryInfo.Voltage_f, batteryInfo.Temp_f,
                          systemInfo.powerMonitor.ExternalPower == 1 ? I18n::Text(I18n::TextId::BatteryExternalPower)
                          : batteryInfo.chargeStatus == notCharge    ? I18n::Text(I18n::TextId::BatteryNotCharge)
                          : batteryInfo.chargeStatus == fastCharge   ? I18n::Text(I18n::TextId::BatteryFastCharge)
                                                                     : I18n::Text(I18n::TextId::BatteryNormalCharge));
}

void
SystemInfosView::SetStorage(const RecordInfo_t& recordInfo) const {
    if (recordInfo.record_status) {
        lv_label_set_text_fmt(ui.storage.labelData,
                              "ON\n"
                              "%.16s\n"
                              "%0.1f M\n"
                              "%s\n"
                              "%0.2f %s",
                              recordInfo.record_name, recordInfo.record_leftspace,
                              recordInfo.record_type == 1 ? "XYZ" : "Rinex3.02",
                              recordInfo.record_interval > 0x00 ? recordInfo.record_interval : 0.25,
                              I18n::Text(I18n::TextId::StorageHour));
    } else {
        lv_label_set_text_fmt(ui.storage.labelData, "OFF\n"
                                                    "-\n"
                                                    "-\n"
                                                    "-\n"
                                                    "-");
    }
}

void
SystemInfosView::SetSystem(const char* firmVer, const char* bootTime, const uint16_t error,
                           const char* buildTime) const {
    lv_label_set_text_fmt(ui.system.labelData,
                          "%s\n"
                          "%s\n"
                          "%04x\n"
                          "%s",
                          firmVer, bootTime, error, buildTime);
}

void
SystemInfosView::ApplyLanguage() const {
    lv_label_set_text(ui.work.labelName, I18n::Text(I18n::TextId::SystemWorkTitle));
    lv_label_set_text(ui.work.labelInfo, I18n::Text(I18n::TextId::SystemWorkInfo));
    lv_label_set_text(ui.gps.labelName, I18n::Text(I18n::TextId::SystemGpsTitle));
    lv_label_set_text(ui.gps.labelInfo, I18n::Text(I18n::TextId::SystemGpsInfo));
    lv_label_set_text(ui.wifi.labelName, I18n::Text(I18n::TextId::SystemWifiTitle));
    lv_label_set_text(ui.wifi.labelInfo, I18n::Text(I18n::TextId::SystemWifiInfo));
    lv_label_set_text(ui.battery.labelName, I18n::Text(I18n::TextId::SystemBatteryTitle));
    lv_label_set_text(ui.battery.labelInfo, I18n::Text(I18n::TextId::SystemBatteryInfo));
    lv_label_set_text(ui.storage.labelName, I18n::Text(I18n::TextId::SystemStorageTitle));
    lv_label_set_text(ui.storage.labelInfo, I18n::Text(I18n::TextId::SystemStorageInfo));
    lv_label_set_text(ui.system.labelName, I18n::Text(I18n::TextId::SystemTitle));
    lv_label_set_text(ui.system.labelInfo, I18n::Text(I18n::TextId::SystemInfo));
}
