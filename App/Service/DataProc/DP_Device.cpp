#include "Aggregate.h"
#include "SystemService.h"
#include "Def/DP_Battery.h"
#include "Def/DP_GNSS.h"
#include "Def/DP_Recorder.h"
#include "Def/DP_Version.h"
#include "Def/DP_WiFi.h"
#include "Def/DP_Work.h"
namespace {
class DeviceNode final : public dp::Aggregate<DataProc::DeviceSnapshot, dp::Battery, dp::GNSS, dp::Recorder,
    dp::Version, dp::WiFi, dp::Work> {
public:
    explicit DeviceNode(DataBroker* broker) : Aggregate(dp::names::Device, broker) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return Aggregate::onEvent(event);
        DataProc::DeviceRequest request;
        const int decoded = DataProc::ReadPayload(event, request);
        if (decoded != RES_OK) return decoded;
        switch (request.command) {
        case DataProc::DeviceCommand::ApplyWork:
            return dp::Work::notify(*this, {static_cast<hal::WorkTarget>(request.target), request.radio_protocol, request.radio_channel});
        case DataProc::DeviceCommand::StartRecord:
            return dp::Recorder::notify(*this, {hal::RecordAction::Start, request.record_type, request.record_interval});
        case DataProc::DeviceCommand::StopRecord:
            return dp::Recorder::notify(*this, {hal::RecordAction::Stop});
        default: return RES_UNSUPPORTED_REQUEST;
        }
    }
private:
    DataProc::DeviceSnapshot compose() const override {
        DataProc::DeviceSnapshot value;
        const auto& battery = get<dp::Battery>(); const auto& gnss = get<dp::GNSS>();
        const auto& record = get<dp::Recorder>(); const auto& version = get<dp::Version>();
        const auto& wifi = get<dp::WiFi>(); const auto& work = get<dp::Work>();
        value.position_valid = gnss.position_valid; value.latitude = gnss.latitude; value.longitude = gnss.longitude;
        value.altitude = gnss.altitude; value.satellites_valid = gnss.satellites_valid;
        value.satellites_used = gnss.satellites_used; value.satellites_tracked = gnss.satellites_tracked;
        value.constellations_valid = gnss.constellations_valid; value.constellations = gnss.constellations;
        value.work_valid = work.valid; value.work_pending = work.pending; value.work_mode = static_cast<DataProc::WorkMode>(work.mode);
        value.radio_on = work.radio_on; value.radio_protocol = work.protocol; value.radio_channel = work.channel;
        value.ntrip = static_cast<DataProc::NtripRole>(work.ntrip);
        value.record_valid = record.valid; value.recording = record.recording; value.record_pending = record.pending;
        value.record_type = record.type; value.record_interval = record.interval; value.record_name = record.name;
        value.record_space_mb = record.space_mb;
        value.wifi_valid = wifi.valid; value.wifi_station = wifi.station; value.wifi_ssid = wifi.ssid; value.wifi_ip = wifi.ip;
        value.power_valid = battery.valid; value.external_power = battery.external_power; value.battery_percent = battery.gauge_percent;
        value.battery_voltage = battery.voltage; value.battery_temperature = battery.temperature;
        value.charge = static_cast<DataProc::ChargeState>(battery.charge);
        value.firmware = version.firmware; value.hardware = version.hardware; value.build = version.build; value.uptime_s = version.uptime_s;
        return value;
    }
};
}
DP_CREATE(Device, DeviceNode)
DataProc::DeviceSnapshot DeviceService::snapshot() {
    DataProc::DeviceSnapshot result;
    auto* node = DataProc::Broker()->search(dp::names::Device);
    if (node) {
        DataNode::EventParam_t event(nullptr, node, DataNode::EVENT_PULL, &result, sizeof(result));
        (void)node->onEvent(&event);
    }
    return result;
}
