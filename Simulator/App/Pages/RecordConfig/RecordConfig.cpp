//
// Created by gtc on 2026/5/9.
//

#include "RecordConfig.h"

using namespace Page;

const uint8_t RecordConfigType[Redcord_Type_MAX] = {0x01, 0x02};
const uint8_t RecordConfigInterval[Redcord_Interval_MAX] = {0xff, 0x00, 0x01, 0x02, 0x04, 0x18};

void
RecordConfig::onCustomAttrConfig() {
    SetCustomCacheEnable(true);
    SetCustomLoadAnimType(PageManager::LOAD_ANIM_NONE);
}

void
RecordConfig::onViewLoad() {
    PageBase::onViewLoad();
    Model.Init();
    View.Create(_root);
    //lv_obj_fade_in(_root, 300, 0);
#if PRODUCT_RECORD_SETTINGS
    AttachEvent(View.ui.roller.left_roller.btnUp);
    AttachEvent(View.ui.roller.left_roller.btnDown);
    AttachEvent(View.ui.roller.right_roller.btnUp);
    AttachEvent(View.ui.roller.right_roller.btnDown);
#else
    // 录制类型/间隔不可调的产品只显示当前值。
    lv_obj_clear_flag(View.ui.roller.left_roller.btnUp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(View.ui.roller.left_roller.btnDown, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(View.ui.roller.right_roller.btnUp, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(View.ui.roller.right_roller.btnDown, LV_OBJ_FLAG_CLICKABLE);
#endif

    AttachEvent(View.ui.btnCont.btnRecord);
    AttachEvent(View.ui.btnCont.btnReturn);
}

void
RecordConfig::onViewDidLoad() {
    PageBase::onViewDidLoad();
}

void
RecordConfig::onViewWillAppear() {
    PageBase::onViewWillAppear();

    lv_indev_wait_release(lv_indev_get_act());
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);

    lv_group_set_wrap(group, true);

#if PRODUCT_RECORD_SETTINGS
    lv_group_add_obj(group, View.ui.roller.left_roller.btnUp);
    lv_group_add_obj(group, View.ui.roller.left_roller.btnDown);
    lv_group_add_obj(group, View.ui.roller.right_roller.btnUp);
    lv_group_add_obj(group, View.ui.roller.right_roller.btnDown);
#endif

    lv_group_add_obj(group, View.ui.btnCont.btnRecord);
    lv_group_add_obj(group, View.ui.btnCont.btnReturn);

    if (lastFocus) {
        lv_group_focus_obj(lastFocus);
    } else {
        lv_group_focus_obj(View.ui.btnCont.btnReturn);
    }
    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_TRANSP);

    View.AppearAnimStart();
}

void
RecordConfig::onViewDidAppear() {
    PageBase::onViewDidAppear();
    if (timerRecord) {
        PM_LOG_INFO("RecordConfig::timerRecord Resume");
        lv_timer_resume(timerRecord);
    } else {
        PM_LOG_INFO("RecordConfig::timerRecord Create");
        timerRecord = lv_timer_create(onTimerRecordUpdate, 1000, this);
        lv_timer_ready(timerRecord);
    }
    if (timerRoller) {
        PM_LOG_INFO("RecordConfig::timerRoller Resume");
        lv_timer_resume(timerRoller);
        lv_timer_ready(timerRoller);
    } else {
        PM_LOG_INFO("RecordConfig::timerRecord Create");
        timerRoller = lv_timer_create(onTimerRollerUpdate, 20000, this);
        lv_timer_ready(timerRoller);
    }
}

void
RecordConfig::onViewWillDisappear() {
    PageBase::onViewWillDisappear();
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    lastFocus = lv_group_get_focused(group);
    lv_group_remove_all_objs(group);
    if (timerRecord) {
        PM_LOG_INFO("RecordConfig::timerRecord Pause");
        lv_timer_pause(timerRecord);
    }
    if (timerRoller) {
        PM_LOG_INFO("RecordConfig::timerRecord Pause");
        lv_timer_pause(timerRoller);
    }
}

void
RecordConfig::onViewDidDisappear() {
    PageBase::onViewDidDisappear();
}

void
RecordConfig::onViewUnload() {
    PageBase::onViewUnload();
    if (timerRecord) {
        lv_timer_del(timerRecord);
        timerRecord = nullptr;
    }
    if (timerRoller) {
        lv_timer_del(timerRoller);
        timerRoller = nullptr;
    }
    Model.Deinit();
    View.Delete();
    if (lastFocus) {
        lastFocus = nullptr;
    }
}

void
RecordConfig::onViewDidUnload() {
    PageBase::onViewDidUnload();
}

void
RecordConfig::onLanguageChanged() {
    View.ApplyLanguage();
}

void
RecordConfig::onBtnClicked(const lv_obj_t* btn, const lv_event_code_t& code) {
    const bool isRecording = recState == RECORD_STATE_START;
    if (code == LV_EVENT_SHORT_CLICKED) {
        if (btn == View.ui.btnCont.btnRecord) {
            onRecord(false);
        } else if (btn == View.ui.btnCont.btnReturn) {
            pageManager->Pop();
        } else if (btn == View.ui.roller.left_roller.btnUp && !isRecording) {
            View.Roller_up(View.ui.roller.left_roller.label);
        } else if (btn == View.ui.roller.left_roller.btnDown && !isRecording) {
            View.Roller_down(View.ui.roller.left_roller.label);
        } else if (btn == View.ui.roller.right_roller.btnUp && !isRecording) {
            View.Roller_up(View.ui.roller.right_roller.label);
        } else if (btn == View.ui.roller.right_roller.btnDown && !isRecording) {
            View.Roller_down(View.ui.roller.right_roller.label);
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        if (btn == View.ui.btnCont.btnRecord) {
            const uint8_t RecordType_index =
                RecordConfigType[RecordConfigView::Roller_GetIndex(View.ui.roller.left_roller.label)];
            const uint8_t RecordInterval_index =
                RecordConfigInterval[RecordConfigView::Roller_GetIndex(View.ui.roller.right_roller.label)];
            systemInfo.recordInfo.record_type = RecordType_index;
            systemInfo.recordInfo.record_interval = RecordInterval_index;
            PM_LOG_INFO("RecordConfig::RollerUpdate, RecordType_index:%d, RecordInterval_index:%d", RecordType_index,
                        RecordInterval_index);
            onRecord(true);
        }
    }
}

void
RecordConfig::onRecord(const bool longPress) {
    switch (recState) {
        case RECORD_STATE_START:
            if (!longPress) {
                Model.RecorderCommand(RecordConfigModel::REC_STOP);
                SetBtnRecImgSrc("start");
                recState = RECORD_STATE_STOP;
            }
            break;
        case RECORD_STATE_STOP:
            if (longPress) {
                Model.RecorderCommand(RecordConfigModel::REC_START);
                SetBtnRecImgSrc("stop");
                recState = RECORD_STATE_START;
            }
            break;
        default: break;
    }
}

void
RecordConfig::SetBtnRecImgSrc(const char* srcName) const {
    lv_obj_set_style_bg_img_src(View.ui.btnCont.btnRecord, ResourcePool::GetImage(srcName), 0);
}

void
RecordConfig::AttachEvent(lv_obj_t* obj) {
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void
RecordConfig::RecordUpdate() {
    if (systemInfo.recordInfo.record_status == On_Off_Status_OFF && recState == RECORD_STATE_START) {
        onRecord(false);
    } else if (systemInfo.recordInfo.record_status == On_Off_Status_ON && recState == RECORD_STATE_STOP) {
        onRecord(true);
    }
}

void
RecordConfig::RollerUpdate() const {
    View.Update();
}

void
RecordConfig::onTimerRecordUpdate(lv_timer_t* timer) {
    auto* instance = static_cast<RecordConfig*>(timer->user_data);
    instance->RecordUpdate();
    PM_LOG_INFO("RecordConfig::RecordUpdate");
}

void
RecordConfig::onTimerRollerUpdate(lv_timer_t* timer) {
    auto* instance = static_cast<RecordConfig*>(timer->user_data);
    instance->RollerUpdate();
    PM_LOG_INFO("RecordConfig::RollerUpdate");
}

void
RecordConfig::onEvent(lv_event_t* event) {
    auto* instance = static_cast<RecordConfig*>(lv_event_get_user_data(event));
    LV_ASSERT_NULL(instance);

    const lv_obj_t* obj = lv_event_get_current_target(event);
    const lv_event_code_t code = lv_event_get_code(event);

    instance->onBtnClicked(obj, code);
}
