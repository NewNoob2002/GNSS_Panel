#include "SystemInfos.h"

using namespace Page;

SystemInfos::SystemInfos() = default;

SystemInfos::~SystemInfos() = default;

void
SystemInfos::onCustomAttrConfig() {}

void
SystemInfos::onViewLoad() {
    Model.Init();
    View.Create(_root);
    AttachEvent(_root);
}

void
SystemInfos::onViewDidLoad() {
    const auto item_grp = reinterpret_cast<SystemInfosView::item_t*>(&View.ui);

    constexpr size_t itemCount = sizeof(View.ui) / sizeof(SystemInfosView::item_t);
    for (size_t i = 0; i < itemCount; i++) {
        AttachEvent(item_grp[i].icon);
    }
}

void
SystemInfos::onViewWillAppear() {
    Model.SetStatusBarStyle(DataProc::STATUS_BAR_STYLE_BLACK);

    timer = lv_timer_create(onTimerUpdate, 1000, this);
    lv_timer_ready(timer);

    SystemInfosView::SetScrollToY(_root, -LV_VER_RES, LV_ANIM_OFF);
    lv_obj_set_style_opa(_root, LV_OPA_TRANSP, 0);
    lv_obj_fade_in(_root, 300, 0);
}

void
SystemInfos::onViewDidAppear() {
    lv_group_t* group = lv_group_get_default();
    LV_ASSERT_NULL(group);
    SystemInfosView::onFocus(group);
}

void
SystemInfos::onViewWillDisappear() {
    PageBase::onViewWillDisappear();
    // lv_obj_fade_out(_root, 300, 0);
}

void
SystemInfos::onViewDidDisappear() {
    if (timer) {
        lv_timer_del(timer);
        timer = nullptr;
    }
}

void
SystemInfos::onViewUnload() {
    View.Delete();
    Model.Deinit();
}

void
SystemInfos::onViewDidUnload() {}

void
SystemInfos::onLanguageChanged() {
    View.ApplyLanguage();
    Update();
}

void
SystemInfos::AttachEvent(lv_obj_t* obj) {
    lv_obj_add_event_cb(obj, onEvent, LV_EVENT_ALL, this);
}

void
SystemInfos::Update() const {
    char buf[32];

    /* Sport */
    View.SetWork(systemInfo.work_mode, systemInfo.radioInfo, systemInfo.ntripInfo);

    /* GPS */
    View.SetGPS(systemInfo.positionInfo.coordinate_lat, systemInfo.positionInfo.coordinate_lon,
                systemInfo.positionInfo.coordinate_alt);

    /* WIFI */
    View.SetWifi(systemInfo.wifiInfo);

    /* Power */
    // BatteryInfo_t power;
    // Model.GetBatteryInfo(&power);
    View.SetBattery(systemInfo.powerMonitor.batteryInfo);

    /* Storage */
    // bool detect;
    // const char* type = "-";
    // Model.GetStorageInfo(&detect, &type, buf, sizeof(buf));
    View.SetStorage(systemInfo.recordInfo);

    /* System */
    MakeTimeString(lv_tick_get(), buf, sizeof(buf));
    View.SetSystem(
        SOFTWARE_VERSION,
        buf,
        systemInfo.i2c__err_count,
        SOFTWARE_BUILD_DATE " " SOFTWARE_BUILD_TIME);
}

void
SystemInfos::onTimerUpdate(lv_timer_t* timer) {
    const auto* instance = static_cast<SystemInfos*>(timer->user_data);
    LV_ASSERT_NULL(instance);
    instance->Update();
}

void
SystemInfos::onEvent(lv_event_t* event) {
    const auto* instance = static_cast<SystemInfos*>(lv_event_get_user_data(event));
    LV_ASSERT_NULL(instance);

    const lv_obj_t* obj = lv_event_get_current_target(event);
    const lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED) {
        if (lv_obj_has_state(obj, LV_STATE_FOCUSED)) {
            instance->pageManager->Pop();
        }
    }

    if (obj == instance->_root) {
        if (code == LV_EVENT_LEAVE) {
            instance->pageManager->Pop();
        }
    }
}
