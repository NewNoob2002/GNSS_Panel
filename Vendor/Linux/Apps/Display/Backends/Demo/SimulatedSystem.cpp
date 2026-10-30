#include <cstdio>
#include "DemoModel.h"
#include "DemoStatus.h"

namespace demo {
Model&
model() {
    static Model value = [] {
        Model m;
        m.battery = {true, 85, 85.0f, 8.08f, true, 28.5f, hal::ChargeState::NotCharging, false};
        m.gnss.position_valid = true;
        m.gnss.latitude = 31.2304;
        m.gnss.longitude = 121.4737;
        m.gnss.altitude = 12.5;
        m.gnss.satellites_valid = true;
        m.gnss.satellites_used = 8;
        m.gnss.satellites_tracked = 14;
        m.gnss.constellations_valid = true;
        m.gnss.constellations = {4, 3, 1, 2, 0, 1, 0};
        m.gnss.fix = hal::FixStatus::Single;
        m.clock = {true, 12, 0, 0, hal::ClockSource::Uptime, 0};
        m.work.valid = true;
        m.work.channel = 1;
        m.recorder.valid = true;
        m.recorder.interval = 1;
        std::snprintf(m.recorder.name.data(), m.recorder.name.size(), "%s", "DEMO-001");
        m.recorder.space_mb = 1228.8f;
        m.wifi.valid = true;
        m.wifi.station = true;
        std::snprintf(m.wifi.ssid.data(), m.wifi.ssid.size(), "%s", "RK3506-DEMO");
        m.wifi.ip = {192, 168, 4, 2};
        std::snprintf(m.board.firmware.data(), m.board.firmware.size(), "%s", "1.0.0");
        std::snprintf(m.board.hardware.data(), m.board.hardware.size(), "%s", "J V1.0");
        std::snprintf(m.board.build.data(), m.board.build.size(), "%s", "DEMO-001");
        m.board.capabilities = {true, true, false, true, true};
        return m;
    }();
    return value;
}

void
refresh() {
    DemoStatus::refresh();
}
} // namespace demo
