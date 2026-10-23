#pragma once

#include "UI/Common/Model.h"
#include "UI/Common/RequestFeedback.h"
#include "Def/DP_Work.h"

namespace page {
/** @brief 工作设置页面显示与交互值。 */
struct WorkSettingsState {
    hal::WorkState work{};
    hal::WorkTarget target = hal::WorkTarget::Rover;
    unsigned protocol = 0;
    unsigned channel = 0;
    i18n::TextId feedback = i18n::TextId::Count;
};

/** @brief 随页面加载创建、卸载销毁；只订阅 Work，通知 StatusBar。 */
class WorkSettingsModel final : public Model<WorkSettingsState> {
  public:
    WorkSettingsModel() : Model("WorkSettingsModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    void sync_initial(const hal::WorkState& work);
    void cycle_target();
    void cycle_protocol();
    void cycle_channel();
    void clear_feedback();
    void select_channel_feedback();
    void poll_feedback();

    bool apply();

    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::Work::State& value);
    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
    RequestFeedback feedback_{};
};
} // namespace page
