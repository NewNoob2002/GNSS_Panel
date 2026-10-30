#include "StatusBarModel.h"
#include <tuple>

using namespace page;

bool StatusBarModel::init() {
    return ready() && load<dp::Battery>() && load<dp::Clock>() && load<dp::GNSS>() && load<dp::Recorder>()
           && load<dp::Settings>() && load<dp::Version>() && load<dp::WiFi>() && load<dp::Work>();
}

int StatusBarModel::onEvent(EventParam_t* event) {
    if (!event)
        return RES_PARAM_ERROR;
    if (event->event == EVENT_NOTIFY) {
        DataProc::StatusBarPresentation request;
        const int result = DataProc::ReadPayload(event, request);
        if (result != RES_OK)
            return result;
        return presentation_(owner_, request) ? RES_OK : RES_PARAM_ERROR;
    }
    int result = receive<dp::Battery>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::Clock>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::GNSS>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::Recorder>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::Settings>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::Version>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::WiFi>(event);
    if (result == RES_UNSUPPORTED_REQUEST) result = receive<dp::Work>(event);
    return result;
}

void StatusBarModel::commit(StatusBarState next) {
    // 定位状态色取自公共调色板：无解/演示=危险红，基准站=信息蓝，单点/浮点=进度黄，固定=正常绿。
    i18n::TextId position = i18n::TextId::PositionNone;
    lv_color_t color = style::kDanger;
    if (mode_ == hal::WorkMode::Base) {
        position = i18n::TextId::PositionBase;
        color = style::kInfo;
    } else {
        switch (fix_) {
        case hal::FixStatus::Single: position = i18n::TextId::PositionSingle; color = style::kHighlight; break;
        case hal::FixStatus::Float: position = i18n::TextId::PositionFloat; color = style::kHighlight; break;
        case hal::FixStatus::Fixed: position = i18n::TextId::PositionFix; color = style::kSuccess; break;
        default: break;
        }
    }
    next.position = simulated_ ? "DEMO" : i18n::text(position);
    next.position_color = simulated_ ? style::kDanger : color;
    const auto values = [](const StatusBarState& s) {
        return std::tie(s.satellites, s.satellites_valid, s.hour, s.minute, s.second, s.clock_valid, s.recording,
                        s.wifi, s.battery_percent, s.battery_valid, s.charging);
    };
    if (values(next) == values(state_) && std::strcmp(next.position, state_.position) == 0
        && lv_color_to_u32(next.position_color) == lv_color_to_u32(state_.position_color))
        return;
    state_ = next;
    changed();
}

void StatusBarModel::apply(const dp::Battery::State& value) {
    auto next = state_;
    next.battery_percent = value.level_percent;
    next.battery_valid = value.valid;
    next.charging = value.charge != hal::ChargeState::NotCharging;
    commit(next);
}
void StatusBarModel::apply(const dp::Clock::State& value) {
    auto next = state_;
    next.hour = value.clock.hour;
    next.minute = value.clock.minute;
    next.second = value.clock.second;
    next.clock_valid = value.clock.time_valid;
    commit(next);
}
void StatusBarModel::apply(const dp::GNSS::State& value) {
    auto next = state_;
    next.satellites = value.satellites_used;
    next.satellites_valid = value.satellites_valid;
    fix_ = value.fix;
    commit(next);
}
void StatusBarModel::apply(const dp::Recorder::State& value) {
    auto next = state_;
    /** @brief 状态栏等主控确认；首页录制入口仍显示请求后的 recording。 */
    next.recording = value.reported_recording;
    commit(next);
}
void StatusBarModel::apply(const dp::Settings::State&) { commit(state_); }
void StatusBarModel::apply(const dp::Version::State& value) {
    simulated_ = value.capabilities.simulated;
    commit(state_);
}
void StatusBarModel::apply(const dp::WiFi::State& value) {
    auto next = state_;
    next.wifi = value.on;
    commit(next);
}
void StatusBarModel::apply(const dp::Work::State& value) {
    mode_ = value.mode;
    commit(state_);
}
