#pragma once

#include "UI/Common/Model.h"
#include "UI/Common/RequestFeedback.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Version.h"

namespace page {
/** @brief 录制配置页面显示与交互值。 */
struct RecordConfigState {
    hal::RecorderState recorder{};
    bool record_settings = false;
    unsigned type = 0;
    unsigned interval = 0;
    i18n::TextId feedback = i18n::TextId::Count;
};

/** @brief 随页面加载创建、卸载销毁；只订阅 Recorder、Version，通知 StatusBar。 */
class RecordConfigModel final : public Model<RecordConfigState> {
  public:
    RecordConfigModel() : Model("RecordConfigModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    void sync_initial(const hal::RecorderState& recorder, bool record_settings);
    void cycle_type();
    void cycle_interval();
    void clear_feedback();
    void poll_feedback();

    bool toggle();

    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::Recorder::State& value);
    void apply(const dp::Version::State& value);
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
