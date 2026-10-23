#pragma once

#include <array>
#include "UI/Common/Model.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Version.h"

namespace page {
/** @brief 星历图页面状态。 */
struct StarMapState {
    bool constellations_valid = false;
    std::array<unsigned, 7> constellations{};
    bool simulated = false;
};

/** @brief 随页面加载创建、卸载销毁；只订阅 GNSS、Version，通知 StatusBar。 */
class StarMapModel final : public Model<StarMapState> {
  public:
    StarMapModel() : Model("StarMapModel") {}
    bool init();
    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    int onEvent(EventParam_t* event) override;

  private:
    void apply(const dp::GNSS::State& value);
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
