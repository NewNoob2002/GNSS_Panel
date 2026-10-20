//
// Created by gtc on 2025/12/15.
//

#include "StatusBar.h"

#include <cstdio>

#include "../Page.h"
#include "Common/DataProc/DataProc.h"

#define BATT_USAGE_HEIGHT (8)
#define BATT_USAGE_WIDTH  (16)

#define STATUS_BAR_HEIGHT 26

static Account* actStatusBar;
static bool StatusBarAppear = false;

struct {
    lv_obj_t* cont;

    struct {
        numberFlow* satellite_num;
        lv_obj_t* position_icon;
        lv_obj_t* position_label;
    } position;

    numberFlow_clock* clock;
    lv_obj_t* sd_icon;
    lv_obj_t* wifi_icon;

    struct {
        lv_obj_t* img;
        lv_obj_t* objUsage;
        numberFlow* percent;
    } battery;

    lv_obj_t* labelRec;
} ui;

static void StatusBar_ConBattSetOpa(lv_obj_t* obj, int32_t opa);

static void StatusBar_onAnimOpaFinish(lv_anim_t* a);

static void StatusBar_AnimCreate(lv_obj_t* contBatt);

static void StatusBar_onAnimWidthFinish(lv_anim_t* a);

static void
StatusBar_ConBattSetOpa(lv_obj_t* obj, int32_t opa) {
    lv_obj_set_style_opa(obj, opa, 0);
}

static void
StatusBar_onAnimOpaFinish(lv_anim_t* a) {
    auto* obj = static_cast<lv_obj_t*>(a->var);
    StatusBar_ConBattSetOpa(obj, LV_OPA_COVER);
    StatusBar_AnimCreate(obj);
}

static void
StatusBar_onAnimWidthFinish(lv_anim_t* a) {
    lv_anim_t a_opa;
    lv_anim_init(&a_opa);
    lv_anim_set_var(&a_opa, a->var);
    lv_anim_set_exec_cb(&a_opa, reinterpret_cast<lv_anim_exec_xcb_t>(StatusBar_ConBattSetOpa));
    lv_anim_set_ready_cb(&a_opa, StatusBar_onAnimOpaFinish);
    lv_anim_set_values(&a_opa, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_early_apply(&a_opa, true);
    lv_anim_set_delay(&a_opa, 500);
    lv_anim_set_time(&a_opa, 500);
    lv_anim_start(&a_opa);
}

static void
StatusBar_AnimCreate(lv_obj_t* contBatt) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, contBatt);
    lv_anim_set_exec_cb(&a, LV_ANIM_EXEC(width));
    lv_anim_set_values(&a, 0, BATT_USAGE_WIDTH);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_ready_cb(&a, StatusBar_onAnimWidthFinish);
    lv_anim_start(&a);
}

static void
StatusBar_StyleInit(lv_obj_t* cont) {
    /* style1 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x333333), LV_STATE_DEFAULT);

    /* style2 */
    lv_obj_set_style_bg_opa(cont, LV_OPA_60, LV_STATE_USER_1);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_color(cont, lv_color_black(), LV_STATE_USER_1);
    lv_obj_set_style_shadow_width(cont, 10, LV_STATE_USER_1);

    static lv_style_transition_dsc_t tran;
    static constexpr lv_style_prop_t prop[] = {LV_STYLE_BG_COLOR, LV_STYLE_OPA, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&tran, prop, lv_anim_path_ease_out, 200, 0, nullptr);
    lv_obj_set_style_transition(cont, &tran, LV_STATE_USER_1);
}

static void
StatusBar_SetStyle(const DataProc::StatusBar_Style_t style) {
    lv_obj_t* cont = ui.cont;
    switch (style) {
        case DataProc::STATUS_BAR_STYLE_TRANSP:
            lv_obj_add_state(cont, LV_STATE_DEFAULT);
            lv_obj_clear_state(cont, LV_STATE_USER_1);
            break;
        case DataProc::STATUS_BAR_STYLE_BLACK: lv_obj_add_state(cont, LV_STATE_USER_1); break;
        default: break;
    }
}

static void
StatusBar_Update(lv_timer_t* timer) {
    (void)timer;
    // HAL::GPS_Info_t gps;
    // if(actStatusBar->Pull("GPS", &gps, sizeof(gps)) == Account::RES_OK)
    // {
    //     lv_label_set_text_fmt(ui.satellite.label, "%d", gps.satellites);
    // }
    /* satellite */
    ui.position.satellite_num->setValue(systemInfo.positionInfo.satellite_number_used);
    // Position
    if (systemInfo.work_mode == base_mode || systemInfo.work_mode == autobase_mode) {
        lv_obj_set_style_text_color(ui.position.position_icon, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_label_set_text(ui.position.position_label, I18n::Text(I18n::TextId::PositionBase));
    } else {
        switch (systemInfo.positionInfo.coordinate_status) {
            case position_none: // NONE
                lv_obj_set_style_text_color(ui.position.position_icon, lv_palette_main(LV_PALETTE_RED), 0);
                lv_label_set_text(ui.position.position_label, I18n::Text(I18n::TextId::PositionNone));
                break;
            case position_single: // Single
                lv_obj_set_style_text_color(ui.position.position_icon, lv_palette_main(LV_PALETTE_YELLOW), 0);
                lv_label_set_text(ui.position.position_label, I18n::Text(I18n::TextId::PositionSingle));
                break;
            case position_fix: // FIX
                lv_obj_set_style_text_color(ui.position.position_icon, lv_palette_main(LV_PALETTE_GREEN), 0);
                lv_label_set_text(ui.position.position_label, I18n::Text(I18n::TextId::PositionFix));
                break;
            case position_float: // FLOAT
                lv_obj_set_style_text_color(ui.position.position_icon, lv_palette_main(LV_PALETTE_YELLOW), 0);
                lv_label_set_text(ui.position.position_label, I18n::Text(I18n::TextId::PositionFloat));
                break;
            default:;
        }
    }
    //
    /* record */
    // if (systemInfo.recordInfo.record_status == 1)
    //     lv_obj_set_style_text_color(ui.sd_icon, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    // else
    //     lv_obj_set_style_text_color(ui.sd_icon, lv_color_white(), LV_STATE_DEFAULT);

    /* wifi */
    if (systemInfo.wifiInfo.wifi_status == 1) {
        lv_obj_set_style_text_color(ui.wifi_icon, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(ui.wifi_icon, lv_color_white(), LV_STATE_DEFAULT);
    }

    /* clock */
    makeTime_t clock;
    MakeTime(lv_tick_get(), &clock);
    ui.clock->setTime(clock.hour, clock.minute, clock.second);

    // /* battery */
    ui.battery.percent->setValue(systemInfo.powerMonitor.batteryInfo.Percent);

    const bool Is_BattCharging = systemInfo.powerMonitor.batteryInfo.chargeStatus != notCharge;
    lv_obj_t* contBatt = ui.battery.objUsage;
    static bool Is_BattChargingAnimActive = false;
    if (Is_BattCharging) {
        if (!Is_BattChargingAnimActive) {
            lv_obj_set_style_bg_color(contBatt, lv_color_hex(0x4CAF50), 0);
            StatusBar_AnimCreate(contBatt);
            Is_BattChargingAnimActive = true;
        }
    } else {
        if (Is_BattChargingAnimActive) {
            lv_anim_del(contBatt, nullptr);
            StatusBar_ConBattSetOpa(contBatt, LV_OPA_COVER);
            Is_BattChargingAnimActive = false;
        }
        uint16_t battery_percentage = systemInfo.powerMonitor.batteryInfo.Percent;
        if (battery_percentage > 100) {
            battery_percentage = 100;
        }

        lv_color_t battery_color;
        if (battery_percentage > 50) {
            battery_color = lv_color_hex(0x4CAF50); // 绿色
        } else if (battery_percentage > 20 && battery_percentage <= 50) {
            battery_color = lv_color_hex(0xFF9800); // 橙色
        } else if (battery_percentage <= 20) {
            battery_color = lv_color_hex(0xF44336); // 红色
        }
        lv_obj_set_style_bg_color(contBatt, battery_color, 0);
        const lv_coord_t width = lv_map(battery_percentage, 0, 100, 0, BATT_USAGE_WIDTH);
        lv_obj_set_width(contBatt, width);
    }
}

lv_obj_t*
Page::StatusBar_Create(lv_obj_t* par) {
    lv_obj_t* cont = lv_obj_create(par);
    lv_obj_remove_style_all(cont);

    const lv_font_t* font = ResourcePool::GetFont("oswaldBold_18");
    const lv_font_t* font2 = ResourcePool::GetFont("oswaldBold_12");
    lv_obj_set_size(cont, LV_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_y(cont, -STATUS_BAR_HEIGHT);
    StatusBar_StyleInit(cont);
    ui.cont = cont;

    lv_obj_t* satellite_img = lv_img_create(cont);
    lv_img_set_src(satellite_img, ResourcePool::GetImage("satellite_small"));
    const auto* img_satellite_ext = reinterpret_cast<lv_img_t*>(satellite_img);
    lv_obj_set_size(satellite_img, img_satellite_ext->w, img_satellite_ext->h);
    lv_obj_align(satellite_img, LV_ALIGN_TOP_LEFT, 10, 5);

    ui.position.satellite_num = new numberFlow(font2, 2, true);
    ui.position.satellite_num->create(cont);
    ui.position.satellite_num->setAlignTo(satellite_img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    ui.position.satellite_num->setValue(0);

    const lv_font_t* statusBar_font = ResourcePool::GetFont("statusbar");
    lv_obj_t* position_icon = lv_label_create(cont);
    lv_obj_remove_style_all(position_icon);
    lv_obj_set_style_text_font(position_icon, statusBar_font, 0);
    lv_obj_set_style_text_color(position_icon, lv_palette_main(LV_PALETTE_GREY), LV_STATE_DEFAULT);
    lv_label_set_text(position_icon, CUSTOM_SYMBOL_LOCATION);
    lv_obj_align_to(position_icon, ui.position.satellite_num->getCont(), LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    ui.position.position_icon = position_icon;

    lv_obj_t* position_label = lv_label_create(cont);
    lv_obj_remove_style_all(position_label);
    lv_obj_set_style_text_font(position_label, font2, 0);
    lv_obj_set_style_text_color(position_label, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text(position_label, I18n::Text(I18n::TextId::PositionFloat));
    lv_obj_align_to(position_label, position_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    ui.position.position_label = position_label;

    ui.clock = new numberFlow_clock(font);
    ui.clock->create(cont);
    ui.clock->setPos(LV_ALIGN_TOP_MID, 0, 0);
    ui.clock->setTime(0, 0, 0);

    lv_obj_t* sd_icon = lv_label_create(cont);
    lv_obj_remove_style_all(sd_icon);
    lv_obj_set_style_text_font(sd_icon, statusBar_font, 0);
    lv_obj_set_style_text_color(sd_icon, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text(sd_icon, CUSTOM_SYMBOL_SD_CARD);
    lv_obj_align_to(sd_icon, ui.clock->getCont(), LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    ui.sd_icon = sd_icon;

    lv_obj_t* wifi_icon = lv_label_create(cont);
    lv_obj_remove_style_all(wifi_icon);
    lv_obj_set_style_text_font(wifi_icon, statusBar_font, 0);
    lv_obj_set_style_text_color(wifi_icon, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text(wifi_icon, CUSTOM_SYMBOL_WIFI);
    lv_obj_align_to(wifi_icon, sd_icon, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    ui.wifi_icon = wifi_icon;

    lv_obj_t* img = lv_img_create(cont);
    lv_img_set_src(img, ResourcePool::GetImage("battery"));
    const auto* img_ext = reinterpret_cast<lv_img_t*>(img);
    lv_obj_set_size(img, img_ext->w, img_ext->h);
    lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -40, 5);
    ui.battery.img = img;

    lv_obj_t* obj = lv_obj_create(img);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_size(obj, BATT_USAGE_WIDTH, BATT_USAGE_HEIGHT);
    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 2, 0);
    ui.battery.objUsage = obj;

    ui.battery.percent = new numberFlow(font, 3, true);
    ui.battery.percent->create(cont);
    ui.battery.percent->setAlignTo(img, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    ui.battery.percent->setValue(0);

    StatusBar_SetStyle(DataProc::STATUS_BAR_STYLE_TRANSP);

    lv_timer_t* timer = lv_timer_create(StatusBar_Update, 1000, nullptr);
    lv_timer_ready(timer);

    return ui.cont;
}

void
Page::StatusBar_ApplyLanguage() {
    if (ui.position.position_label != nullptr) {
        StatusBar_Update(nullptr);
    }
}

void
StatusBar_Appear(const bool en, const bool delay) {
    int32_t start = -STATUS_BAR_HEIGHT;
    int32_t end = 0;
    if ((en && StatusBarAppear) || (!en && !StatusBarAppear)) {
        return;
    }

    StatusBarAppear = en;
    if (!en) {
        const int32_t temp = start;
        start = end;
        end = temp;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui.cont);
    lv_anim_set_values(&a, start, end);
    lv_anim_set_time(&a, 500);
    if (delay) {
        lv_anim_set_delay(&a, 1000);
    } else {
        lv_anim_set_delay(&a, 0);
    }
    lv_anim_set_exec_cb(&a, LV_ANIM_EXEC(y));
    if (en) {
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    } else {
        lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    }
    lv_anim_set_early_apply(&a, true);
    lv_anim_start(&a);
}

static void
StatusBar_SetRecord(const bool active) {
    if (active) {
        lv_obj_set_style_text_color(ui.sd_icon, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
        systemInfo.recordInfo.record_status = On_Off_Status_ON;
        systemInfo.recordInfo.record_op = 1;
        systemInfo.recordInfo.record_change_flag = 1;
        CORE_DEBUG_PRINTF("StatusBar_SetRecord: RecordInfo: record_status:%d, op:%d, interval:%d, changeflag:%d\n",
                          systemInfo.recordInfo.record_status, systemInfo.recordInfo.record_op,
                          systemInfo.recordInfo.record_interval, systemInfo.recordInfo.record_change_flag);
    } else {
        lv_obj_set_style_text_color(ui.sd_icon, lv_color_white(), LV_STATE_DEFAULT);
        systemInfo.recordInfo.record_status = On_Off_Status_OFF;
        systemInfo.recordInfo.record_op = 1;
        systemInfo.recordInfo.record_change_flag = 1;
        CORE_DEBUG_PRINTF("StatusBar_SetRecord: RecordInfo: record_status:%d, op:%d, interval:%d, changeflag:%d\n",
                          systemInfo.recordInfo.record_status, systemInfo.recordInfo.record_op,
                          systemInfo.recordInfo.record_interval, systemInfo.recordInfo.record_change_flag);
    }
}

static int
onEvent(Account* account, Account::EventParam_t* param) {
    (void)account;
    if (param->event != Account::EVENT_NOTIFY) {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    if (param->size != sizeof(DataProc::StatusBar_Info_t)) {
        return Account::RES_SIZE_MISMATCH;
    }

    switch (const auto* info = static_cast<DataProc::StatusBar_Info_t*>(param->data_p); info->cmd) {
        case DataProc::STATUS_BAR_CMD_APPEAR: StatusBar_Appear(info->param.appear, info->param.delay); break;
        case DataProc::STATUS_BAR_CMD_SET_STYLE: StatusBar_SetStyle(info->param.style); break;
        case DataProc::STATUS_BAR_CMD_SET_LABEL_REC: StatusBar_SetRecord(info->param.record_active); break;
        default: return Account::RES_PARAM_ERROR;
    }

    return Account::RES_OK;
}

DATA_PROC_INIT_DEF(StatusBar) {
    account->SetEventCallback(onEvent);

    actStatusBar = account;
}
