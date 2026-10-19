#include "Shutdown.h"

#include "App.h"
#include "HAL.h"

using namespace Page;

static bool anim_complement_callback_do = false;

static void
lv_anim_obj_set_width(void* obj, const int32_t width) {
    lv_anim_t* anim = lv_anim_get(obj, lv_anim_obj_set_width);
    if (anim == nullptr) {
        return;
    }

    const auto* instance = static_cast<Shutdown*>(lv_anim_get_user_data(anim));
    LV_ASSERT_NULL(instance);

    lv_obj_set_width(static_cast<lv_obj_t*>(obj), width);

    if (instance->View.ui.shutdown.bar.label) {
        lv_label_set_text_fmt(instance->View.ui.shutdown.bar.label, "%ld%%", static_cast<long>(width));
    }
    if (width >= 100) {
        if (!anim_complement_callback_do) {
            anim_complement_callback_do = true;
            lv_obj_fade_out(instance->View.ui.shutdown.cont, 300, 0);
            HAL::Power_Shutdown(false);
            instance->pageManager->Push("Pages/SaveConfig");
#if defined(LVGL_SIMULATOR) || defined(_WIN32)
#else
            HAL::Encoder_SetEnable(false);
#endif
        }
    }
}

void
Shutdown::onCustomAttrConfig() {
    LV_LOG_USER("Shutdown onCustomAttrConfig");
    SetCustomCacheEnable(true);
}

void
Shutdown::onViewLoad() {
    LV_LOG_USER("Shutdown onViewLoad");
    View.Create(_root);
    lv_anim_set_exec_cb(&View.ui.shutdown.bar.anim, lv_anim_obj_set_width);
    lv_anim_set_user_data(&View.ui.shutdown.bar.anim, this);

    AttachEvent(View.ui.shutdown.btnPress);
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    AttachEvent(View.ui.shutdown.btnWifi);
#else
    // 主控不支持由面板开关 Wi-Fi 的产品隐藏该按钮。
    lv_obj_add_flag(View.ui.shutdown.btnWifi, LV_OBJ_FLAG_HIDDEN);
#endif
    AttachEvent(View.ui.shutdown.btnLanguage);
}

void
Shutdown::onViewDidLoad() {
    LV_LOG_USER("onViewDidLoad");
}

void
Shutdown::onViewWillAppear() {
    LV_LOG_USER("onViewWillAppear");
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lv_group_add_obj(group, View.ui.shutdown.btnPress);
#if PRODUCT_SHUTDOWN_WIFI_BUTTON
    lv_group_add_obj(group, View.ui.shutdown.btnWifi);
#endif
    lv_group_add_obj(group, View.ui.shutdown.btnLanguage);
    lv_group_focus_obj(View.ui.shutdown.btnPress);

    Update();
    View.SetWifiLoading(wifiChangePending, wifiLoadingStep);
    if (timer == nullptr) {
        timer = lv_timer_create(onTimerUpdate, 500, this);
    } else {
        lv_timer_resume(timer);
    }
}

void
Shutdown::onViewDidAppear() {
    LV_LOG_USER("onViewDidAppear");
}

void
Shutdown::onViewWillDisappear() {
    LV_LOG_USER("onViewWillDisappear");
    lv_group_remove_obj(View.ui.shutdown.btnPress);
    lv_group_remove_obj(View.ui.shutdown.btnWifi);
    lv_group_remove_obj(View.ui.shutdown.btnLanguage);
    if (timer) {
        lv_timer_pause(timer);
    }
}

void
Shutdown::onViewDidDisappear() {
    LV_LOG_USER("onViewDidDisappear");
}

void
Shutdown::onViewUnload() {
    LV_LOG_USER("onViewUnload");
    anim_complement_callback_do = false;
    if (timer) {
        lv_timer_del(timer);
        timer = nullptr;
    }
    View.Delete();
}

void
Shutdown::onViewDidUnload() {
    LV_LOG_USER("onViewDidUnload");
}

void
Shutdown::onLanguageChanged() {
    View.ApplyLanguage();
}

void
Shutdown::AttachEvent(lv_obj_t* obj) {
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void
Shutdown::Update() {
    View.SetWifiStatus(systemInfo.wifiInfo.wifi_status);
    if (wifiChangePending && systemInfo.wifiInfo.wifi_status == systemInfo.wifiInfo.wifi_on_off_set) {
        wifiChangePending = false;
        wifiLoadingStep = 0;
        View.SetWifiLoading(false);
    } else if (wifiChangePending) {
        View.SetWifiLoading(true, wifiLoadingStep++);
    }
}

void
Shutdown::onTimerUpdate(lv_timer_t* timer) {
    auto* instance = static_cast<Shutdown*>(timer->user_data);
    LV_ASSERT_NULL(instance);
    instance->Update();
}

void
Shutdown::onEvent(lv_event_t* event) {
    auto* instance = static_cast<Shutdown*>(lv_event_get_user_data(event));
    LV_ASSERT_NULL(instance);
    const lv_obj_t* obj = lv_event_get_target(event);
    const lv_event_code_t code = lv_event_get_code(event);

    if (obj == instance->View.ui.shutdown.btnWifi) {
        if (code == LV_EVENT_SHORT_CLICKED && !instance->wifiChangePending) {
            systemInfo.wifiInfo.wifi_on_off_set =
                systemInfo.wifiInfo.wifi_status == On_Off_Status_ON ? On_Off_Status_OFF : On_Off_Status_ON;
            systemInfo.wifiInfo.wifi_change_flag = 1;
            instance->wifiMessageCount = systemInfo.messageDecode.InfoWifi_count;
            instance->wifiChangePending = true;
            instance->wifiLoadingStep = 0;
            instance->View.SetWifiLoading(true, instance->wifiLoadingStep++);
            LV_LOG_USER("Shutdown btnWifi clicked, set wifi_on_off_set to %d", systemInfo.wifiInfo.wifi_on_off_set);
        } else if (code == LV_EVENT_SHORT_CLICKED && instance->wifiChangePending) {
            LV_LOG_USER("Shutdown btnWifi event pending, rev code: %d", code);
        }
        return;
    }

    if (obj == instance->View.ui.shutdown.btnLanguage) {
        if (code == LV_EVENT_SHORT_CLICKED) {
            const I18n::Language nextLanguage =
                App_GetLanguage() == I18n::Language::Russian ? I18n::Language::English : I18n::Language::Russian;
            App_SetLanguage(nextLanguage);
        }
        return;
    }

    if (obj != instance->View.ui.shutdown.btnPress) {
        return;
    }

    if (code == LV_EVENT_SHORT_CLICKED) {
        instance->pageManager->Pop();
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESSED) {
#if defined(HC32F460)
        if (!systemInfo.online_device.eg25_board && !systemInfo.eg25_overtime) {
            return;
        }
#endif
        lv_anim_t anim = instance->View.ui.shutdown.bar.anim;
        const int16_t current_width = lv_obj_get_width(instance->View.ui.shutdown.bar.obj);
        constexpr uint32_t boot_time_ms = 2000; // 设定长按关机时间为2秒
        const uint32_t remaining_time = boot_time_ms * (100 - current_width) / 100;
        switch (code) {
            case LV_EVENT_PRESSED: {
                lv_anim_del(instance->View.ui.shutdown.bar.obj, lv_anim_obj_set_width);
                lv_anim_set_path_cb(&anim, lv_anim_path_linear);
                lv_anim_set_values(&anim, current_width, 100);
                lv_anim_set_time(&anim, remaining_time);
                lv_anim_start(&anim);
                break;
            }
            case LV_EVENT_RELEASED: {
                if (current_width < 100) {
                    lv_anim_del(instance->View.ui.shutdown.bar.obj, lv_anim_obj_set_width);

                    lv_anim_set_values(&anim, current_width, 0);
                    lv_anim_set_time(&anim, 300);
                    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
                    lv_anim_start(&anim);
                }
                break;
            }
            default:;
        }
    }
}
