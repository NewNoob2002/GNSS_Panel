#include "Startup.h"
#include <cstdio>

using namespace Page;

static void
lv_anim_arc_set_value(void* obj, int32_t value) {
    lv_anim_t* anim = lv_anim_get(obj, lv_anim_arc_set_value);
    // 注意：在动画结束销毁时 anim 可能为空，需做保护
    if (!anim) {
        return;
    }

    const auto* instance = static_cast<Startup*>(lv_anim_get_user_data(anim));
    LV_ASSERT_NULL(instance);

    lv_arc_set_value(static_cast<lv_obj_t*>(obj), value);

    // 更新百分比文字 (放在这里可以让文字跟随动画平滑变化)
    if (instance->View.ui.arc_percent) {
        lv_label_set_text_fmt(instance->View.ui.arc_percent, "%" PRIu32 "%%", value);
    }

    if (value >= 100) {
        lv_obj_set_style_arc_color(static_cast<lv_obj_t*>(obj), lv_color_hex(0x70e958), LV_PART_MAIN);

        if (!systemInfo.powerMonitor.panel_power_on) {
#if defined(LVGL_SIMULATOR) || defined(_WIN32)
            systemInfo.powerMonitor.panel_power_on = true;
            systemInfo.powerMonitor.pannel_power_on_time = lv_tick_get();
#endif
        }
    }
}

void
Startup::onCustomAttrConfig() {
    SetCustomCacheEnable(false);
    SetCustomLoadAnimType(PageManager::LOAD_ANIM_NONE);
}

void
Startup::onViewLoad() {
    Model.Init();
    View.Create(_root);
    lv_obj_fade_in(_root, 300, 0);
    lv_anim_set_exec_cb(&View.ui.arc_anim, lv_anim_arc_set_value);
    lv_anim_set_user_data(&View.ui.arc_anim, this);
    AttachEvent(View.ui.btnPress);
}

void
Startup::onViewDidLoad() {
    timer = lv_timer_create(onTimer, 5000, this);
    lv_timer_set_repeat_count(timer, 1);
}

void
Startup::onViewWillAppear() {
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lv_group_add_obj(group, View.ui.btnPress);
    lv_group_focus_obj(View.ui.btnPress);
}

void
Startup::onViewDidAppear() {}

void
Startup::onViewWillDisappear() {
    if (timer != nullptr) {
        lv_timer_del(timer);
        timer = nullptr;
    }
    Model.SetStatusBarAppear(false, false);
}

void
Startup::onViewDidDisappear() {}

void
Startup::onViewUnload() {
    View.Delete();
    Model.Deinit();
    Model.SetEncoderEnable(false);
}

void
Startup::onViewDidUnload() {}

void
Startup::onLanguageChanged() {
    View.ApplyLanguage();
}

void
Startup::AttachEvent(lv_obj_t* obj) {
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void
Startup::onTimer(lv_timer_t* timer) {
    const auto* instance = static_cast<Startup*>(timer->user_data);
    instance->Model.SetStatusBarAppear(true, true);
    instance->timer = nullptr;
}

void
Startup::onEvent(lv_event_t* event) {
    auto* instance = static_cast<Startup*>(lv_event_get_user_data(event));
    LV_ASSERT_NULL(instance);
    if (systemInfo.powerMonitor.panel_power_on) {
        return;
    }

    if (const lv_obj_t* obj = lv_event_get_current_target(event); obj != instance->View.ui.btnPress) {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) {
        return;
    }

    // 获取当前进度，用于断点续传或回退
    const int16_t current_value = lv_arc_get_value(instance->View.ui.arc);

    // === 按下瞬间：开始充能 ===
    if (code == LV_EVENT_PRESSED) {
        // 如果已经触发了完成，就不再处理

        // 停止之前的任何动画（防止冲突）
        lv_anim_del(instance->View.ui.arc, lv_anim_arc_set_value);

        // 设置动画：从当前值走到 100
        lv_anim_set_values(&instance->View.ui.arc_anim, current_value, 100);

        // 计算剩余时间：如果总共需要2000ms，根据当前进度计算剩余时间
        // 这样体验更自然，不会因为进度一半按下还需要等完整的2秒
        uint32_t boot_time_ms = 2000; // 设定长按开机时间为2秒
        uint32_t remaining_time = boot_time_ms * (100 - current_value) / 100;

        lv_anim_set_time(&instance->View.ui.arc_anim, remaining_time);
        lv_anim_set_path_cb(&instance->View.ui.arc_anim, lv_anim_path_linear);
        lv_anim_start(&instance->View.ui.arc_anim);
    }
    // === 松开或焦点丢失：回退清零 ===
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {

        // 停止充能动画
        lv_anim_del(instance->View.ui.arc, lv_anim_arc_set_value);

        // 如果还没满，开始回退动画：从当前值回到 0
        if (current_value < 100) {
            lv_anim_set_values(&instance->View.ui.arc_anim, current_value, 0);

            // 回退速度可以快一点，比如固定 300ms 或者 500ms
            lv_anim_set_time(&instance->View.ui.arc_anim, 300);
            lv_anim_set_path_cb(&instance->View.ui.arc_anim, lv_anim_path_ease_out);
            lv_anim_start(&instance->View.ui.arc_anim);
        }
    }
}
