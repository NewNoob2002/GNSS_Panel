#pragma once

#include "UI/Common/Model.h"
#include "Def/DP_Battery.h"
#include "Def/DP_Settings.h"

namespace page {
/** @brief 温度告警只依赖有效温度与显示语言。 */
struct BatteryWarningState {
    bool valid = false;
    float temperature = 0;
    i18n::Language language{};
};
class BatteryWarningModel final : public Model<BatteryWarningState> {
  public:
    BatteryWarningModel() : Model("BatteryWarning") {}
    bool init();
    int onEvent(EventParam_t* event) override;
};
} // namespace page
