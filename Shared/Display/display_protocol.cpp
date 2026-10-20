#include "display_protocol.h"
#include <cmath>
#include <cstring>
#include "SystemState.h"
#include "core_debug.h"
#include "mcu_define.h"
#include "product_config.h"

// 写入范围：主控上报只写 State::Host()；面板请求经 State::Confirm_*() 确认；
// 电源相关通知经 State::Host_*() 交给 HAL 电源策略。

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

static void
fill_info1_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info1_count++;
    const BoardInfo_t& board = systemInfo.board;
    const BatteryInfo_t& battery = systemInfo.power.batteryInfo;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 0], board.hardware_version, strnlen(board.hardware_version, 8));
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 8], board.software_version, strnlen(board.software_version, 10));
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 18], &battery.Percent, 2);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 20], &battery.Temp, 2);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 22], &battery.Voltage, 2);
}

static void
fill_info2_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info2_count++;
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.power.reset_flag;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = systemInfo.power.poweroff_flag;
    msg[NM_PROTOCOL_HEADER_LEN + 2] = State::Effective_Record().record_status;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = systemInfo.request.record_op;
    msg[NM_PROTOCOL_HEADER_LEN + 4] = systemInfo.power.batteryInfo.chargeStatus != notCharge;
    response_snapshot.poweroffFlag = systemInfo.power.poweroff_flag;
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
    msg[NM_PROTOCOL_HEADER_LEN + 0] = work.work_mode;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = host.position.satellite_number_used;
    msg[NM_PROTOCOL_HEADER_LEN + 2] = host.position.coordinate_status;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = work.gprs_status;
    msg[NM_PROTOCOL_HEADER_LEN + 4] = work.NtripServer_status;
    msg[NM_PROTOCOL_HEADER_LEN + 5] = host.ntrip.NtripClient_status;
    msg[NM_PROTOCOL_HEADER_LEN + 6] = work.radio_status;
    msg[NM_PROTOCOL_HEADER_LEN + 11] = systemInfo.request.work_pending;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 12], &host.ntrip.NtripServer_IP, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 16], &host.ntrip.NtripServer_Mountpoint, 32);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 48], &host.ntrip.NtripClient_IP, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 52], &host.ntrip.NtripClient_Mountpoint, 32);
    msg[NM_PROTOCOL_HEADER_LEN + 84] = work.radio_mode;
    msg[NM_PROTOCOL_HEADER_LEN + 85] = work.radio_protocol;
    msg[NM_PROTOCOL_HEADER_LEN + 86] = work.radio_channel;
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
    msg[NM_PROTOCOL_HEADER_LEN + 0] = record.record_status;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = systemInfo.request.record_op;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 4], &record.record_leftspace, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 8], &record.record_name, 16);
    msg[NM_PROTOCOL_HEADER_LEN + 24] = record.record_type;
    msg[NM_PROTOCOL_HEADER_LEN + 25] = record.record_interval;
    msg[NM_PROTOCOL_HEADER_LEN + 26] = systemInfo.request.record_settings_pending;
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
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.request.wifi_on;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = systemInfo.request.wifi_pending;
    response_snapshot.wifi = State::Wifi_Revision();
}

static void
wifi_info_read_complete() {
    State::Confirm_Wifi(response_snapshot.wifi);
}
#endif

static bool
validate_flag(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    return payload[0] <= 1U;
}

static void
handle_reset_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_reset = parse->buffer[NM_PROTOCOL_HEADER_LEN];
    CORE_DEBUG_PRINTF("Reset Panel request: %u", pending_reset);
    panel_write_success_payload(msg);
}

static void
reset_read_complete() {
    State::Host_ResetRequested(pending_reset);
}

static void
handle_shutdown_confirmation(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_shutdown_confirmation = parse->buffer[NM_PROTOCOL_HEADER_LEN];
    msg[NM_PROTOCOL_HEADER_LEN] = pending_shutdown_confirmation;
}

static void
shutdown_confirmation_read_complete() {
    if (pending_shutdown_confirmation) {
        State::Host_ShutdownConfirmed();
    }
}

static bool
validate_set1(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    return payload[0] <= autobase_mode && valid_position(payload[2]) && payload[3] <= 1U && payload[4] <= 1U
           && payload[5] <= 1U && payload[6] <= 1U && payload[8] <= 1U && payload[84] <= radio_mode_bridge
           && payload[87] <= 1U;
}

// 主控上报总是写入 host；面板有待确认的工作模式请求时界面仍显示请求值（State::Effective_Work）。
static void
handle_set1_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set1_count++;
    const uint8_t* payload = &parse->buffer[NM_PROTOCOL_HEADER_LEN];
    HostReport_t& host = State::Host();
    host.work.work_mode = static_cast<WorkMode_t>(payload[0]);
    host.position.satellite_number_used = payload[1];
    host.position.coordinate_status = static_cast<PositionStatus_t>(payload[2]);
    host.work.gprs_status = static_cast<On_Off_Status_t>(payload[3]);
    host.work.NtripServer_status = static_cast<On_Off_Status_t>(payload[4]);
    host.ntrip.NtripClient_status = static_cast<On_Off_Status_t>(payload[5]);
    host.work.radio_status = static_cast<On_Off_Status_t>(payload[6]);
    host.position.satellite_number_track = payload[7];
    State::Host_ExternalPower(payload[8]);
    memcpy(&host.ntrip.NtripServer_IP, &payload[12], 4);
    memcpy(&host.ntrip.NtripServer_Mountpoint, &payload[16], 32);
    memcpy(&host.ntrip.NtripClient_IP, &payload[48], 4);
    memcpy(&host.ntrip.NtripClient_Mountpoint, &payload[52], 32);
    host.work.radio_mode = static_cast<RadioMode_t>(payload[84]);
    host.work.radio_protocol = payload[85];
    host.work.radio_channel = payload[86];
    if (payload[11]) {
        State::Echo_HostWork();
    }
    if (payload[87]) {
        State::Host_ShutdownRequested();
    }
    panel_write_success_payload(msg);
}

static bool
validate_set3(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    double longitude, latitude, altitude;
    memcpy(&longitude, payload + 4, sizeof(longitude));
    memcpy(&latitude, payload + 12, sizeof(latitude));
    memcpy(&altitude, payload + 20, sizeof(altitude));
    return valid_position(payload[0]) && std::isfinite(longitude) && longitude >= -180.0 && longitude <= 180.0
           && std::isfinite(latitude) && latitude >= -90.0 && latitude <= 90.0 && std::isfinite(altitude);
}

static void
handle_set3_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set3_count++;
    PositionInfo_t& position = State::Host().position;
    position.coordinate_status = static_cast<PositionStatus_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
    memcpy(&position.coordinate_lon, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 4], 8);
    memcpy(&position.coordinate_lat, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 12], 8);
    memcpy(&position.coordinate_alt, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 20], 8);
    panel_write_success_payload(msg);
}

static bool
validate_set6(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    float space;
    memcpy(&space, payload + 4, sizeof(space));
    return payload[0] <= 1U && std::isfinite(space) && space >= 0.0f;
}

static void
handle_set6_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set6_count++;
    const uint8_t* payload = &parse->buffer[NM_PROTOCOL_HEADER_LEN];
    RecordInfo_t& record = State::Host().record;
    record.record_status = static_cast<On_Off_Status_t>(payload[0]);
    memcpy(&record.record_leftspace, &payload[4], 4);
    memcpy(&record.record_name, &payload[8], 16);
    record.record_type = payload[24];
    record.record_interval = payload[25];
    if (payload[26]) {
        State::Echo_HostRecord();
    }
    panel_write_success_payload(msg);
}

static bool
validate_set_wifi(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    return payload[0] <= 1U && payload[1] <= 1U;
}

static void
handle_set_wifi_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.SetWifi_count++;
    WifiInfo_t& wifi = State::Host().wifi;
    wifi.wifi_status = static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
    wifi.wifi_mode = parse->buffer[NM_PROTOCOL_HEADER_LEN + 1];
    memcpy(&wifi.wifi_ip, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 4], 4);
    memcpy(&wifi.wifi_ssid, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 8], 16);
    panel_write_success_payload(msg);
}

#if PRODUCT_PROTOCOL_SATELLITE
static void
handle_set_satellite_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.SetSatellite_count++;
    StarMapInfo_t& stars = State::Host().starMap;
    stars.numberGPS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 0];
    stars.numberGLONASS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 1];
    stars.numberGALILEO = parse->buffer[NM_PROTOCOL_HEADER_LEN + 2];
    stars.numberBDS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 3];
    stars.numberSBAS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 4];
    stars.numberQZSS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 5];
    stars.numberIRNSS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 6];
    ++stars.sequence;
    panel_write_success_payload(msg);
}
#endif

static const PanelPacket packetTable[] = {
    {NM_PANEL_INFO1_ID, 0, NM_PROTOCOL_INFO1_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_info1_response, nullptr, nullptr},
    {NM_PANEL_INFO2_ID, 0, NM_PROTOCOL_INFO2_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_info2_response, nullptr,
     info2_read_complete},
    {NM_PANEL_INFO3_ID, 0, NM_PROTOCOL_INFO3_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_info3_response, nullptr,
     info3_read_complete},
    {NM_PANEL_INFO4_ID, 0, NM_PROTOCOL_INFO4_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_info4_response, nullptr,
     info4_read_complete},
#if PRODUCT_PROTOCOL_WIFI_CONTROL
    {NM_PANEL_WIFI_SET_ID, 0, NM_PROTOCOL_WIFI_SET_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_wifi_info_response, nullptr,
     wifi_info_read_complete},
#endif
    {NM_PANEL_SHUTDOWN_CONFIRM_ID, 1, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_shutdown_confirmation,
     validate_flag, shutdown_confirmation_read_complete},
    {NM_PANEL_RST_ID, 1, NM_PROTOCOL_RST_RESP_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_reset_request, validate_flag,
     reset_read_complete},
    {NM_PANEL_SET1_ID, 88, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_set1_request, validate_set1, nullptr},
    {NM_PANEL_SET3_ID, 28, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_set3_request, validate_set3, nullptr},
    {NM_PANEL_SET6_ID, 27, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_set6_request, validate_set6, nullptr},
    {NM_PANEL_SET_WIFI_ID, 24, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_set_wifi_request, validate_set_wifi,
     nullptr},
#if PRODUCT_PROTOCOL_SATELLITE
    {NM_PANEL_SET_SATELLITE_ID, 7, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_set_satellite_request, nullptr,
     nullptr},
#endif
};

const PanelPacket*
panel_protocol_packets(size_t* count) {
    *count = sizeof(packetTable) / sizeof(packetTable[0]);
    return packetTable;
}
