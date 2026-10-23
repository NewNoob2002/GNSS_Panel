#pragma once

#include "UI/Common/Model.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Version.h"
#include "Def/DP_Work.h"

namespace page {
/** @brief 首页实际显示的值；不包含聚合快照或借用的回调指针。 */
struct DialplateState {
    bool satellites_valid = false;
    unsigned satellites_used = 0, satellites_tracked = 0;
    bool work_valid = false, radio_on = false;
    hal::WorkMode work_mode{};
    bool record_valid = false, recording = false;
    bool star_map = false;
};

/** @brief 随页面加载创建、卸载销毁；只订阅 GNSS、Work、Recorder、Version。 */
class DialplateModel final : public Model<DialplateState> {
  public:
    DialplateModel() : Model("DialplateModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});
    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::GNSS::State& value);
    void apply(const dp::Work::State& value);
    void apply(const dp::Recorder::State& value);
    void apply(const dp::Version::State& value);
    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
};
} // namespace page
