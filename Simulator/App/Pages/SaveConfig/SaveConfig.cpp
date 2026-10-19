#include "SaveConfig.h"

#include "HAL.h"

using namespace Page;

void
SaveConfig::onCustomAttrConfig() {
    LV_LOG_USER("onCustomAttrConfig");
    SetCustomCacheEnable(true);
}

void
SaveConfig::onViewLoad() {
    LV_LOG_USER("ViewLoad");
    Model.Init();
    View.Create(_root);
    Model.SetStatusBarDisappear(false);
    UpdatePowerOffCauseText();
    lv_anim_timeline_start(View.ui.anim_timeline);
    lv_anim_start(&View.ui.sync.bar.anim);
}

void
SaveConfig::onViewDidLoad() {
    LV_LOG_USER("onViewDidLoad");
}

void
SaveConfig::onViewWillAppear() {
    LV_LOG_USER("onViewWillAppear");
}

void
SaveConfig::onViewDidAppear() {
    LV_LOG_USER("onViewDidAppear");
}

void
SaveConfig::onViewWillDisappear() {
    LV_LOG_USER("onViewWillDisappear");
}

void
SaveConfig::onViewDidDisappear() {
    LV_LOG_USER("onViewDidDisappear");
}

void
SaveConfig::onViewUnload() {
    LV_LOG_USER("onViewUnload");
    View.Delete();
    Model.Deinit();
}

void
SaveConfig::onViewDidUnload() {
    LV_LOG_USER("onViewDidUnload");
}

void
SaveConfig::onLanguageChanged() {
    UpdatePowerOffCauseText();
}

void
SaveConfig::UpdatePowerOffCauseText() const {
    View.SetPowerOffCause(GetPowerOffCauseTextId());
}

I18n::TextId
SaveConfig::GetPowerOffCauseTextId() {
    if (systemInfo.powerMonitor.LinuxPowerOff) {
        return I18n::TextId::SaveConfigPowerOffBoard;
    }
    if (systemInfo.powerMonitor.LowBatteryPowerOff) {
        return I18n::TextId::SaveConfigPowerOffLowBattery;
    }
    if (systemInfo.powerMonitor.Force_ShutDown) {
        return I18n::TextId::SaveConfigPowerOffForced;
    }
    return I18n::TextId::SaveConfigPowerOffPushKey;
}
