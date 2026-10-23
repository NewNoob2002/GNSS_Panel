#include "display_protocol.h"
#include <cmath>
#include <cstring>
#include "SystemState.h"
#include "mcu_define.h"
#include <platform_debug.h>
#include "product_config.h"

// 写入范围：主控上报只写 State::Host()；面板请求经 State::Confirm_*() 确认；
// 电源相关通知经 State::Host_*() 交给 HAL 电源策略。载荷布局见 display_protocol.h。

/* 响应编码时记下的请求 revision：完整读出后若请求未再变化，主控即已收到该请求。 */
static struct {
    uint32_t work;
    uint32_t record;
    uint32_t wifi;
    uint8_t poweroffFlag;
} response_snapshot;

static uint8_t pending_reset;
static uint8_t pending_shutdown_confirmation;

static bool
valid_position(uint8_t value) {
    return value == 0U || value == 1U || value == 4U || value == 5U || value == 7U;
}

/** 复制版本等文本字段：最多填满字段，不要求结尾 NUL（与旧报文一致）。 */
template <std::size_t N>
static void
copy_field(char (&dst)[N], const char* src, std::size_t src_size) {
    std::memcpy(dst, src, strnlen(src, src_size < N ? src_size : N));
}

static void
fill_info1_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info1_count++;
    const BoardInfo_t& board = systemInfo.board;
    const BatteryInfo_t& battery = systemInfo.power.batteryInfo;
    PanelInfoPayload info{};
    copy_field(info.hardware_version, board.hardware_version, sizeof(board.hardware_version));
    copy_field(info.software_version, board.software_version, sizeof(board.software_version));
    info.battery_percent = battery.Percent;
    info.battery_temperature = battery.Temp;
    info.battery_voltage = battery.Voltage;
    panel_write(msg, info);
}

static void
fill_info2_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info2_count++;
    PanelStatusPayload status{};
    status.reset = systemInfo.power.reset_flag;
    status.poweroff = systemInfo.power.poweroff_flag;
    status.record_status = State::Effective_Record().record_status;
    status.record_op = systemInfo.request.record_op;
    status.charging = systemInfo.power.batteryInfo.chargeStatus != notCharge;
    panel_write(msg, status);
    response_snapshot.poweroffFlag = status.poweroff;
}

// 主控完整读到关机请求即视为确认（旧版 X1Pro/XBase 语义），与 0x05 确认二选一均可。
static void
info2_read_complete() {
    if (response_snapshot.poweroffFlag) {
        State::Host_ShutdownAcknowledged();
        CORE_DEBUG_PRINTF("Shutdown Sync");
    }
}

static void
fill_info3_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info3_count++;
    const WorkConfig_t work = State::Effective_Work();
    const HostReport_t& host = systemInfo.host;
    DisplayWorkPayload payload{};
    payload.work_mode = work.work_mode;
    payload.satellite_used = host.position.satellite_number_used;
    payload.coordinate_status = host.position.coordinate_status;
    payload.gprs_status = work.gprs_status;
    payload.ntrip_server_status = work.NtripServer_status;
    payload.ntrip_client_status = host.ntrip.NtripClient_status;
    payload.radio_status = work.radio_status;
    payload.radio_change_flag = systemInfo.request.work_pending;
    std::memcpy(payload.ntrip_server_ip, host.ntrip.NtripServer_IP, sizeof(payload.ntrip_server_ip));
    std::memcpy(payload.ntrip_server_mountpoint, host.ntrip.NtripServer_Mountpoint,
                sizeof(payload.ntrip_server_mountpoint));
    std::memcpy(payload.ntrip_client_ip, host.ntrip.NtripClient_IP, sizeof(payload.ntrip_client_ip));
    std::memcpy(payload.ntrip_client_mountpoint, host.ntrip.NtripClient_Mountpoint,
                sizeof(payload.ntrip_client_mountpoint));
    payload.radio_mode = work.radio_mode;
    payload.radio_protocol = work.radio_protocol;
    payload.radio_channel = work.radio_channel;
    panel_write(msg, payload);
    response_snapshot.work = State::Work_Revision();
}

static void
info3_read_complete() {
    State::Confirm_Work(response_snapshot.work);
}

static void
fill_info4_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info4_count++;
    const RecordInfo_t record = State::Effective_Record();
    DisplayRecordPayload payload{};
    payload.record_status = record.record_status;
    payload.record_op = systemInfo.request.record_op;
    payload.record_leftspace = record.record_leftspace;
    std::memcpy(payload.record_name, record.record_name, sizeof(payload.record_name));
    payload.record_type = record.record_type;
    payload.record_interval = record.record_interval;
    payload.record_change_flag = systemInfo.request.record_settings_pending;
    panel_write(msg, payload);
    response_snapshot.record = State::Record_Revision();
}

static void
info4_read_complete() {
    State::Confirm_Record(response_snapshot.record, true);
}

#if PRODUCT_PROTOCOL_WIFI_CONTROL
static void
fill_wifi_info_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.InfoWifi_count++;
    DisplayWifiControlPayload payload{};
    payload.wifi_on = systemInfo.request.wifi_on;
    payload.wifi_change_flag = systemInfo.request.wifi_pending;
    panel_write(msg, payload);
    response_snapshot.wifi = State::Wifi_Revision();
}

static void
wifi_info_read_complete() {
    State::Confirm_Wifi(response_snapshot.wifi);
}
#endif

static bool
validate_flag(const uint8_t* payload, uint16_t payloadLength) {
    return panel_payload<PanelFlagRequest>(payload, payloadLength).value <= 1U;
}

static void
handle_reset_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_reset = panel_request<PanelFlagRequest>(parse).value;
    CORE_DEBUG_PRINTF("Reset Panel request: %u", pending_reset);
    panel_write_success_payload(msg);
}

static void
reset_read_complete() {
    State::Host_ResetRequested(pending_reset);
}

static void
handle_shutdown_confirmation(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_shutdown_confirmation = panel_request<PanelFlagRequest>(parse).value;
    panel_write(msg, PanelSetAck{pending_shutdown_confirmation, 0}); // 回显确认值
}

static void
shutdown_confirmation_read_complete() {
    if (pending_shutdown_confirmation) {
        State::Host_ShutdownConfirmed();
    }
}

static bool
validate_set1(const uint8_t* payload, uint16_t payloadLength) {
    const auto set1 = panel_payload<DisplayWorkPayload>(payload, payloadLength);
    return set1.work_mode <= autobase_mode && valid_position(set1.coordinate_status) && set1.gprs_status <= 1U
           && set1.ntrip_server_status <= 1U && set1.ntrip_client_status <= 1U && set1.radio_status <= 1U
           && set1.external_power <= 1U && set1.radio_mode <= radio_mode_bridge && set1.poweroff_request <= 1U;
}

// 主控上报总是写入 host；面板有待确认的工作模式请求时界面仍显示请求值（State::Effective_Work）。
static void
handle_set1_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set1_count++;
    const auto set1 = panel_request<DisplayWorkPayload>(parse);
    HostReport_t& host = State::Host();
    host.work.work_mode = static_cast<WorkMode_t>(set1.work_mode);
    host.work.gprs_status = static_cast<On_Off_Status_t>(set1.gprs_status);
    host.work.NtripServer_status = static_cast<On_Off_Status_t>(set1.ntrip_server_status);
    host.work.radio_status = static_cast<On_Off_Status_t>(set1.radio_status);
    host.work.radio_mode = static_cast<RadioMode_t>(set1.radio_mode);
    host.work.radio_protocol = set1.radio_protocol;
    host.work.radio_channel = set1.radio_channel;
    host.position.satellite_number_used = set1.satellite_used;
    host.position.satellite_number_track = set1.satellite_track;
    host.position.coordinate_status = static_cast<PositionStatus_t>(set1.coordinate_status);
    host.ntrip.NtripClient_status = static_cast<On_Off_Status_t>(set1.ntrip_client_status);
    std::memcpy(host.ntrip.NtripServer_IP, set1.ntrip_server_ip, sizeof(set1.ntrip_server_ip));
    std::memcpy(host.ntrip.NtripServer_Mountpoint, set1.ntrip_server_mountpoint, sizeof(set1.ntrip_server_mountpoint));
    std::memcpy(host.ntrip.NtripClient_IP, set1.ntrip_client_ip, sizeof(set1.ntrip_client_ip));
    std::memcpy(host.ntrip.NtripClient_Mountpoint, set1.ntrip_client_mountpoint, sizeof(set1.ntrip_client_mountpoint));
    State::Host_ExternalPower(set1.external_power);
    if (set1.radio_change_flag) {
        State::Echo_HostWork();
    }
    if (set1.poweroff_request) {
        State::Host_ShutdownRequested();
    }
    panel_write_success_payload(msg);
}

static bool
validate_set3(const uint8_t* payload, uint16_t payloadLength) {
    const auto set3 = panel_payload<DisplayPositionPayload>(payload, payloadLength);
    return valid_position(set3.coordinate_status) && std::isfinite(set3.longitude) && set3.longitude >= -180.0
           && set3.longitude <= 180.0 && std::isfinite(set3.latitude) && set3.latitude >= -90.0 && set3.latitude <= 90.0
           && std::isfinite(set3.altitude);
}

static void
handle_set3_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set3_count++;
    const auto set3 = panel_request<DisplayPositionPayload>(parse);
    PositionInfo_t& position = State::Host().position;
    position.coordinate_status = static_cast<PositionStatus_t>(set3.coordinate_status);
    position.coordinate_lon = set3.longitude;
    position.coordinate_lat = set3.latitude;
    position.coordinate_alt = set3.altitude;
    panel_write_success_payload(msg);
}

static bool
validate_set6(const uint8_t* payload, uint16_t payloadLength) {
    const auto set6 = panel_payload<DisplayRecordPayload>(payload, payloadLength);
    return set6.record_status <= 1U && std::isfinite(set6.record_leftspace) && set6.record_leftspace >= 0.0f;
}

static void
handle_set6_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set6_count++;
    const auto set6 = panel_request<DisplayRecordPayload>(parse);
    RecordInfo_t& record = State::Host().record;
    record.record_status = static_cast<On_Off_Status_t>(set6.record_status);
    record.record_leftspace = set6.record_leftspace;
    std::memcpy(record.record_name, set6.record_name, sizeof(set6.record_name));
    record.record_type = set6.record_type;
    record.record_interval = set6.record_interval;
    if (set6.record_change_flag) {
        State::Echo_HostRecord();
    }
    panel_write_success_payload(msg);
}

static bool
validate_set_wifi(const uint8_t* payload, uint16_t payloadLength) {
    const auto wifi = panel_payload<DisplayWifiStatusPayload>(payload, payloadLength);
    return wifi.wifi_status <= 1U && wifi.wifi_mode <= 1U;
}

static void
handle_set_wifi_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.SetWifi_count++;
    const auto request = panel_request<DisplayWifiStatusPayload>(parse);
    WifiInfo_t& wifi = State::Host().wifi;
    wifi.wifi_status = static_cast<On_Off_Status_t>(request.wifi_status);
    wifi.wifi_mode = request.wifi_mode;
    std::memcpy(wifi.wifi_ip, request.wifi_ip, sizeof(request.wifi_ip));
    std::memcpy(wifi.wifi_ssid, request.wifi_ssid, sizeof(request.wifi_ssid));
    panel_write_success_payload(msg);
}

#if PRODUCT_PROTOCOL_SATELLITE
static void
handle_set_satellite_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.SetSatellite_count++;
    const auto request = panel_request<DisplaySatellitePayload>(parse);
    StarMapInfo_t& stars = State::Host().starMap;
    stars.numberGPS = request.gps;
    stars.numberGLONASS = request.glonass;
    stars.numberGALILEO = request.galileo;
    stars.numberBDS = request.bds;
    stars.numberSBAS = request.sbas;
    stars.numberQZSS = request.qzss;
    stars.numberIRNSS = request.irnss;
    ++stars.sequence;
    panel_write_success_payload(msg);
}
#endif

static const PanelPacket packetTable[] = {
    {NM_PANEL_INFO1_ID, 0, sizeof(PanelInfoPayload), NM_MSG_QUERY_RES_TYPE, fill_info1_response, nullptr, nullptr},
    {NM_PANEL_INFO2_ID, 0, sizeof(PanelStatusPayload), NM_MSG_QUERY_RES_TYPE, fill_info2_response, nullptr,
     info2_read_complete},
    {NM_PANEL_INFO3_ID, 0, sizeof(DisplayWorkPayload), NM_MSG_QUERY_RES_TYPE, fill_info3_response, nullptr,
     info3_read_complete},
    {NM_PANEL_INFO4_ID, 0, sizeof(DisplayRecordPayload), NM_MSG_QUERY_RES_TYPE, fill_info4_response, nullptr,
     info4_read_complete},
#if PRODUCT_PROTOCOL_WIFI_CONTROL
    {NM_PANEL_WIFI_SET_ID, 0, sizeof(DisplayWifiControlPayload), NM_MSG_QUERY_RES_TYPE, fill_wifi_info_response,
     nullptr, wifi_info_read_complete},
#endif
    {NM_PANEL_SHUTDOWN_CONFIRM_ID, sizeof(PanelFlagRequest), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE,
     handle_shutdown_confirmation, validate_flag, shutdown_confirmation_read_complete},
    {NM_PANEL_RST_ID, sizeof(PanelFlagRequest), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_reset_request,
     validate_flag, reset_read_complete},
    {NM_PANEL_SET1_ID, sizeof(DisplayWorkPayload), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_set1_request,
     validate_set1, nullptr},
    {NM_PANEL_SET3_ID, sizeof(DisplayPositionPayload), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_set3_request,
     validate_set3, nullptr},
    // SET6 末字节保留，主控可只发 27 字节。
    {NM_PANEL_SET6_ID, sizeof(DisplayRecordPayload) - 1U, sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_set6_request,
     validate_set6, nullptr},
    {NM_PANEL_SET_WIFI_ID, sizeof(DisplayWifiStatusPayload), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE,
     handle_set_wifi_request, validate_set_wifi, nullptr},
#if PRODUCT_PROTOCOL_SATELLITE
    {NM_PANEL_SET_SATELLITE_ID, sizeof(DisplaySatellitePayload), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE,
     handle_set_satellite_request, nullptr, nullptr},
#endif
};

const PanelPacket*
panel_protocol_packets(size_t* count) {
    *count = sizeof(packetTable) / sizeof(packetTable[0]);
    return packetTable;
}
