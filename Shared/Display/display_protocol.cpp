#include "display_protocol.h"
#include <cmath>
#include <cstring>
#include "core_debug.h"
#include "mcu_define.h"
#include "product_config.h"

/* 响应编码时的字段快照：完整读出后与当前值比较，期间有新变化则保留变更标志。 */
static struct {
    RadioInfo_t radio;
    WorkMode_t workMode;
    NtripInfo_t ntrip;
    RecordInfo_t record;
    WifiInfo_t wifi;
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
    systemInfo.messageDecode.Info1_count++;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 0], systemInfo.hardware_version, strnlen(systemInfo.hardware_version, 8));
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 8], systemInfo.software_version, strnlen(systemInfo.software_version, 10));
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 18], &systemInfo.powerMonitor.batteryInfo.Percent, 2);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 20], &systemInfo.powerMonitor.batteryInfo.Temp, 2);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 22], &systemInfo.powerMonitor.batteryInfo.Voltage, 2);
}

static void
fill_info2_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    systemInfo.messageDecode.Info2_count++;
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.powerMonitor.reset_flag;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = systemInfo.powerMonitor.poweroff_flag;
    msg[NM_PROTOCOL_HEADER_LEN + 2] = systemInfo.recordInfo.record_status;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = systemInfo.recordInfo.record_op;
    msg[NM_PROTOCOL_HEADER_LEN + 4] = systemInfo.powerMonitor.batteryInfo.chargeStatus != notCharge;
    response_snapshot.poweroffFlag = systemInfo.powerMonitor.poweroff_flag;
}

// 主控完整读到关机请求即视为确认（旧版 X1Pro/XBase 语义），与 0x05 确认二选一均可。
static void
info2_read_complete() {
    if (response_snapshot.poweroffFlag) {
        systemInfo.powerMonitor.poweroff_flag = 0;
        systemInfo.powerMonitor.ShutdownEnsure = true;
        CORE_DEBUG_PRINTF("Shutdown Sync");
    }
}

static void
fill_info3_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    systemInfo.messageDecode.Info3_count++;
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.work_mode;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = systemInfo.positionInfo.satellite_number_used;
    msg[NM_PROTOCOL_HEADER_LEN + 2] = systemInfo.positionInfo.coordinate_status;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = systemInfo.ntripInfo.gprs_status;
    msg[NM_PROTOCOL_HEADER_LEN + 4] = systemInfo.ntripInfo.NtripServer_status;
    msg[NM_PROTOCOL_HEADER_LEN + 5] = systemInfo.ntripInfo.NtripClient_status;
    msg[NM_PROTOCOL_HEADER_LEN + 6] = systemInfo.radioInfo.radio_status;
    msg[NM_PROTOCOL_HEADER_LEN + 11] = systemInfo.radioInfo.radio_change_flag;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 12], &systemInfo.ntripInfo.NtripServer_IP, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 16], &systemInfo.ntripInfo.NtripServer_Mountpoint, 32);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 48], &systemInfo.ntripInfo.NtripClient_IP, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 52], &systemInfo.ntripInfo.NtripClient_Mountpoint, 32);
    msg[NM_PROTOCOL_HEADER_LEN + 84] = systemInfo.radioInfo.radio_mode;
    msg[NM_PROTOCOL_HEADER_LEN + 85] = systemInfo.radioInfo.radio_protocol;
    msg[NM_PROTOCOL_HEADER_LEN + 86] = systemInfo.radioInfo.radio_channel;
    response_snapshot.radio = systemInfo.radioInfo;
    response_snapshot.workMode = systemInfo.work_mode;
    response_snapshot.ntrip = systemInfo.ntripInfo;
}

static void
info3_read_complete() {
    if (memcmp(&systemInfo.radioInfo, &response_snapshot.radio, sizeof(RadioInfo_t)) == 0
        && systemInfo.work_mode == response_snapshot.workMode
        && memcmp(&systemInfo.ntripInfo, &response_snapshot.ntrip, sizeof(NtripInfo_t)) == 0) {
        systemInfo.radioInfo.radio_change_flag = 0;
    }
}

static void
fill_info4_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    systemInfo.messageDecode.Info4_count++;
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.recordInfo.record_status;
    msg[NM_PROTOCOL_HEADER_LEN + 1] = systemInfo.recordInfo.record_op;
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 4], &systemInfo.recordInfo.record_leftspace, 4);
    memcpy(&msg[NM_PROTOCOL_HEADER_LEN + 8], &systemInfo.recordInfo.record_name, 16);
    msg[NM_PROTOCOL_HEADER_LEN + 24] = systemInfo.recordInfo.record_type;
    msg[NM_PROTOCOL_HEADER_LEN + 25] = systemInfo.recordInfo.record_interval;
    msg[NM_PROTOCOL_HEADER_LEN + 26] = systemInfo.recordInfo.record_change_flag;
    response_snapshot.record = systemInfo.recordInfo;
}

static void
info4_read_complete() {
    if (memcmp(&systemInfo.recordInfo, &response_snapshot.record, sizeof(RecordInfo_t)) == 0) {
        systemInfo.recordInfo.record_change_flag = 0;
        systemInfo.recordInfo.record_op = 0;
    }
}

#if PRODUCT_PROTOCOL_WIFI_CONTROL
static void
fill_wifi_info_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    systemInfo.messageDecode.InfoWifi_count++;
    msg[NM_PROTOCOL_HEADER_LEN + 0] = systemInfo.wifiInfo.wifi_on_off_set;
    msg[NM_PROTOCOL_HEADER_LEN + 3] = systemInfo.wifiInfo.wifi_change_flag;
    response_snapshot.wifi = systemInfo.wifiInfo;
}

static void
wifi_info_read_complete() {
    if (memcmp(&systemInfo.wifiInfo, &response_snapshot.wifi, sizeof(WifiInfo_t)) == 0) {
        systemInfo.wifiInfo.wifi_change_flag = 0;
    }
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
    systemInfo.powerMonitor.reset_flag = pending_reset;
}

static void
handle_shutdown_confirmation(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_shutdown_confirmation = parse->buffer[NM_PROTOCOL_HEADER_LEN];
    msg[NM_PROTOCOL_HEADER_LEN] = pending_shutdown_confirmation;
}

static void
shutdown_confirmation_read_complete() {
    if (pending_shutdown_confirmation) {
        systemInfo.powerMonitor.poweroff_flag = 0;
        systemInfo.powerMonitor.ShutdownEnsure = true;
        if (!systemInfo.powerMonitor.ShutdownGoing) {
            systemInfo.powerMonitor.LinuxPowerOff = true;
        }
    }
}

static bool
validate_set1(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    return payload[0] <= autobase_mode && valid_position(payload[2]) && payload[3] <= 1U && payload[4] <= 1U
           && payload[5] <= 1U && payload[6] <= 1U && payload[8] <= 1U && payload[84] <= radio_mode_bridge
           && payload[87] <= 1U;
}

static void
handle_set1_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    systemInfo.messageDecode.Set1_count++;
    if (systemInfo.radioInfo.radio_change_flag == 0) {
        systemInfo.work_mode = static_cast<WorkMode_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
        systemInfo.positionInfo.satellite_number_used = parse->buffer[NM_PROTOCOL_HEADER_LEN + 1];
        systemInfo.positionInfo.coordinate_status =
            static_cast<PositionStatus_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 2]);
        systemInfo.ntripInfo.gprs_status = static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 3]);
        systemInfo.ntripInfo.NtripServer_status =
            static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 4]);
        systemInfo.ntripInfo.NtripClient_status =
            static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 5]);
        systemInfo.radioInfo.radio_status = static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 6]);

        systemInfo.positionInfo.satellite_number_track = parse->buffer[NM_PROTOCOL_HEADER_LEN + 7];

        const uint8_t externalPower = parse->buffer[NM_PROTOCOL_HEADER_LEN + 8];
        if (systemInfo.powerMonitor.ExternalPower && externalPower == 0) {
            systemInfo.powerMonitor.ExternalPowerChange = 1;
        }
        systemInfo.powerMonitor.ExternalPower = externalPower;
        systemInfo.radioInfo.radio_change_flag = parse->buffer[NM_PROTOCOL_HEADER_LEN + 11];
        memcpy(&systemInfo.ntripInfo.NtripServer_IP, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 12], 4);
        memcpy(&systemInfo.ntripInfo.NtripServer_Mountpoint, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 16], 32);
        memcpy(&systemInfo.ntripInfo.NtripClient_IP, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 48], 4);
        memcpy(&systemInfo.ntripInfo.NtripClient_Mountpoint, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 52], 32);
        systemInfo.radioInfo.radio_mode = static_cast<RadioMode_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 84]);
        systemInfo.radioInfo.radio_protocol = parse->buffer[NM_PROTOCOL_HEADER_LEN + 85];
        systemInfo.radioInfo.radio_channel = parse->buffer[NM_PROTOCOL_HEADER_LEN + 86];
        if (parse->buffer[NM_PROTOCOL_HEADER_LEN + 87]) {
            systemInfo.powerMonitor.poweroff_flag = 1;
            systemInfo.powerMonitor.LinuxPowerOff = true;
        }
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
    systemInfo.messageDecode.Set3_count++;
    systemInfo.positionInfo.coordinate_status =
        static_cast<PositionStatus_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
    memcpy(&systemInfo.positionInfo.coordinate_lon, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 4], 8);
    memcpy(&systemInfo.positionInfo.coordinate_lat, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 12], 8);
    memcpy(&systemInfo.positionInfo.coordinate_alt, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 20], 8);
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
    systemInfo.messageDecode.Set6_count++;
    if (systemInfo.recordInfo.record_change_flag == 0) {
        systemInfo.recordInfo.record_status = static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
        memcpy(&systemInfo.recordInfo.record_leftspace, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 4], 4);
        memcpy(&systemInfo.recordInfo.record_name, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 8], 16);
        systemInfo.recordInfo.record_type = parse->buffer[NM_PROTOCOL_HEADER_LEN + 24];
        systemInfo.recordInfo.record_interval = parse->buffer[NM_PROTOCOL_HEADER_LEN + 25];
        systemInfo.recordInfo.record_change_flag = parse->buffer[NM_PROTOCOL_HEADER_LEN + 26];
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
    systemInfo.messageDecode.SetWifi_count++;
    // 无 Wi-Fi 控制的产品不会置位 wifi_change_flag，状态总是直接同步。
    if (systemInfo.wifiInfo.wifi_change_flag == 0) {
        systemInfo.wifiInfo.wifi_status = static_cast<On_Off_Status_t>(parse->buffer[NM_PROTOCOL_HEADER_LEN + 0]);
        systemInfo.wifiInfo.wifi_mode = parse->buffer[NM_PROTOCOL_HEADER_LEN + 1];
        memcpy(&systemInfo.wifiInfo.wifi_ip, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 4], 4);
        memcpy(&systemInfo.wifiInfo.wifi_ssid, &parse->buffer[NM_PROTOCOL_HEADER_LEN + 8], 16);
    }
    panel_write_success_payload(msg);
}

#if PRODUCT_PROTOCOL_SATELLITE
static void
handle_set_satellite_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    systemInfo.messageDecode.SetSatellite_count++;
    systemInfo.starMapInfo.numberGPS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 0];
    systemInfo.starMapInfo.numberGLONASS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 1];
    systemInfo.starMapInfo.numberGALILEO = parse->buffer[NM_PROTOCOL_HEADER_LEN + 2];
    systemInfo.starMapInfo.numberBDS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 3];
    systemInfo.starMapInfo.numberSBAS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 4];
    systemInfo.starMapInfo.numberQZSS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 5];
    systemInfo.starMapInfo.numberIRNSS = parse->buffer[NM_PROTOCOL_HEADER_LEN + 6];
    systemInfo.starMapInfo.syncStatus = true;
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
