#pragma once

#include "UI/Common/Model.h"
#include "UI/Common/PageStyle.h"
#include "Def/DP_Battery.h"
#include "Def/DP_Clock.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Settings.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Work.h"

namespace page {
/** @brief 状态栏显示值；position 只引用程序内静态翻译文本，不借用发布参数。 */
struct StatusBarState {
    unsigned satellites = 0;
    bool satellites_valid = false;
    const char* position = "DEMO";
    lv_color_t position_color = style::kDanger;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
    bool clock_valid = false;
    bool recording = false;
    bool wifi = false;
    unsigned battery_percent = 0;
    bool battery_valid = false;
    bool charging = false;
    float battery_voltage = 7.60f;
    bool battery_temperature_valid = false;
    float battery_temperature = 25.0f;
};

/** @brief 自主订阅状态栏实际依赖；外观命令沿用 StatusBar 节点协议。 */
class StatusBarModel final : public Model<StatusBarState> {
  public:
    using Presentation = bool (*)(void*, const DataProc::StatusBarPresentation&);
    StatusBarModel(void* owner, Presentation presentation) : Model("StatusBar"), owner_(owner), presentation_(presentation) {}
    bool init();
    int onEvent(EventParam_t* event) override;

  private:
    void* owner_;
    Presentation presentation_;
    hal::FixStatus fix_{};
    hal::WorkMode mode_{};
    bool simulated_ = false;
    void commit(StatusBarState next);
    void apply(const dp::Battery::State& value);
    void apply(const dp::Clock::State& value);
    void apply(const dp::GNSS::State& value);
    void apply(const dp::Recorder::State& value);
    void apply(const dp::Settings::State& value);
    void apply(const dp::Version::State& value);
    void apply(const dp::WiFi::State& value);
    void apply(const dp::Work::State& value);
    template <class Domain> bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
    template <class Domain> int receive(EventParam_t* event) {
        typename Domain::State value;
        const int result = Domain::receive(event, value);
        if (result == RES_OK)
            apply(value);
        return result;
    }
};
} // namespace page
