/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "App.h"
#include "Common/DataProc/DataProc.h"
#include "HAL/HAL.h"
#include "Pages/AppFactory.h"
#include "Pages/BatteryWarning/BatteryWarning.h"
#include "Pages/StatusBar/StatusBar.h"
#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "Utils/PageManager/PageManager.h"
#include "mcu_define.h"
#include "src/hal/lv_hal_tick.h"
#if defined(LVGL_SIMULATOR) || defined(_WIN32)
#else
#include "lv_port.h"
#endif

#define ACCOUNT_SEND_CMD(ACT, CMD)                                                                                     \
    do {                                                                                                               \
        DataProc::ACT##_Info_t info;                                                                                   \
        DATA_PROC_INIT_STRUCT(info);                                                                                   \
        info.cmd = DataProc::CMD;                                                                                      \
        DataProc::Center()->AccountMain.Notify(#ACT, &info, sizeof(info));                                             \
    } while (0)

#define PAGE_DIALPLATE_INDEX 2

static AppFactory factory;
static PageManager manager(&factory);
static bool Shutdown_pushed = false;

void
App_Init() {
    /* Make sure the default group exists */
    if (!lv_group_get_default()) {
        lv_group_t* group = lv_group_create();
        lv_group_set_default(group);
    }

    /* Initialize the data processing node */
    DataProc_Init();

    /* Set screen style */
    lv_disp_t* disp_p = lv_disp_get_default();
    // lv_theme_t *theme = lv_theme_basic_init(disp_p);
    lv_theme_t* theme = lv_theme_default_init(disp_p, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp_p, theme);

    /* Set root default style */
    static lv_style_t rootStyle;
    lv_style_init(&rootStyle);
    lv_style_set_width(&rootStyle, LV_HOR_RES);
    lv_style_set_height(&rootStyle, LV_VER_RES);
    lv_style_set_bg_color(&rootStyle, lv_color_black());
    lv_style_set_pad_all(&rootStyle, 0);
    lv_style_set_border_width(&rootStyle, 0);
    lv_style_set_radius(&rootStyle, 0);
    manager.SetRootDefaultStyle(&rootStyle);

    /* Initialize resource pool */
    ResourcePool::Init();

    /* Initialize status bar */
    Page::StatusBar_Create(lv_layer_top());
#if PRODUCT_PAGE_BATTERY_WARNING
    Page::BatteryWarning_Create(lv_layer_top());
#endif

    /* Initialize pages */
    manager.Install("Startup", "Pages/Startup");
    manager.Install("HardwareCheck", "Pages/HardwareCheck");
    manager.Install("Dialplate", "Pages/Dialplate");
    manager.Install("RecordConfig", "Pages/RecordConfig");
    manager.Install("WorkSettings", "Pages/WorkSettings");
    manager.Install("SystemInfos", "Pages/SystemInfos");
    manager.Install("Shutdown", "Pages/Shutdown");
    manager.Install("SaveConfig", "Pages/SaveConfig");
#if PRODUCT_PAGE_STAR_MAP
    manager.Install("StarMap", "Pages/StarMap");
#endif

    manager.SetGlobalLoadAnimType(PageManager::LOAD_ANIM_OVER_TOP);

    manager.Push("Pages/Startup");
}

bool
App_SetLanguage(const I18n::Language language) {
    if (!I18n::SetLanguage(language)) {
        return false;
    }

    manager.NotifyLanguageChanged();
    Page::StatusBar_ApplyLanguage();
#if PRODUCT_PAGE_BATTERY_WARNING
    Page::BatteryWarning_ApplyLanguage();
#endif
    return true;
}

I18n::Language
App_GetLanguage() {
    return I18n::GetLanguage();
}

void
App_Deinit() {
    manager.Clear();
}

void
App_Update() {
#if PRODUCT_PAGE_BATTERY_WARNING
    Page::BatteryWarning_Update();
#endif

    PageBase* current = manager.GetCurrentPage();
    const bool onStartup = current && strcmp(current->pageName, "Pages/Startup") == 0;
    if (systemInfo.powerMonitor.panel_power_on && onStartup) {
        manager.Push("Pages/HardwareCheck");
    }
    // 外部供电关机驻留：回到栈底的待机页 Startup，等待长按电源键开机（开机由 HAL 判定）。
    static bool standby_shown = false;
    if (!systemInfo.powerMonitor.ShutdownComplete) {
        standby_shown = false;
    } else if (!standby_shown && (onStartup || manager.BackHome())) {
        standby_shown = true;
        Shutdown_pushed = false;
#if !defined(LVGL_SIMULATOR) && !defined(_WIN32)
        // 关机页在动画结束时禁用了编码器，待机页需要按键事件驱动开机进度。
        HAL::Encoder_SetEnable(true);
#endif
    }
    if (systemInfo.powerMonitor.Force_ShutDown || systemInfo.powerMonitor.LowBatteryPowerOff
        || systemInfo.powerMonitor.LinuxPowerOff || systemInfo.powerMonitor.ShutdownReq
        || systemInfo.powerMonitor.ShutdownGoing) {
        if (!Shutdown_pushed) {
            Shutdown_pushed =
                (current && strcmp(current->pageName, "Pages/SaveConfig") == 0) || manager.Push("Pages/SaveConfig");
        }
    }
#if defined(LVGL_SIMULATOR) || defined(_WIN32)
    static uint32_t lastTick = 0;
    if (lv_tick_get() - lastTick > 1000) {
        lastTick = lv_tick_get();
        systemInfo.powerMonitor.batteryInfo.Temp_f++;
    }
    if (systemInfo.powerMonitor.batteryInfo.Temp_f >= 80) {
        systemInfo.powerMonitor.batteryInfo.Temp_f = 10;
    }
#else
    // if (systemInfo.powerMonitor.ExternalPowerChange) {
    //     systemInfo.powerMonitor.ExternalPowerChange = 0;
    //     NVIC_SystemReset();
    // }
#endif
}
