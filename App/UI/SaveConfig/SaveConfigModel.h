#pragma once

#include "UI/Common/Model.h"
#include "Def/DP_Power.h"

namespace page {
/** @brief 关机/重启倒计时页面状态。 */
struct SaveConfigState {
    bool shutting_down = false;
    hal::PowerCause power_cause{};
    hal::PowerCommand power_action{};
    bool power_pending = false;
    uint64_t power_token = 0;
};

/** @brief 随页面加载创建、卸载销毁；只订阅 Power，通知 StatusBar。 */
class SaveConfigModel final : public Model<SaveConfigState> {
  public:
    SaveConfigModel() : Model("SaveConfigModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    bool finish_power();
    bool cancel_power();

    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::Power::State& value);
    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
};
} // namespace page
