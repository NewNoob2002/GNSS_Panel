#pragma once

// X1Pro / XBase 带屏面板协议的消息 ID 与载荷长度。帧格式见 panel_protocol.h。

#include "panel_protocol.h"

#define NM_PANEL_INFO1_ID                 0x01
#define NM_PROTOCOL_INFO1_MSG_LEN         36 //32 bytes message length
#define NM_PROTOCOL_INFO1_MSG_PACK_LEN    60 //60 bytes package length

#define NM_PANEL_INFO2_ID                 0x02
#define NM_PROTOCOL_INFO2_MSG_LEN         8  //8 bytes message length
#define NM_PROTOCOL_PINFO2_MSG_PACK_LEN   32 //32 bytes package length

#define NM_PANEL_INFO3_ID                 0x07
#define NM_PROTOCOL_INFO3_MSG_LEN         88  //88 bytes message length
#define NM_PROTOCOL_PINFO3_MSG_PACK_LEN   112 //112 bytes package length

#define NM_PANEL_INFO4_ID                 0x0A
#define NM_PROTOCOL_INFO4_MSG_LEN         28 //28 bytes message length
#define NM_PROTOCOL_PINFO4_MSG_PACK_LEN   52 //52 bytes package length

/* 仅 PRODUCT_PROTOCOL_WIFI_CONTROL：面板请求开关 Wi-Fi */
#define NM_PANEL_WIFI_SET_ID              0x0B
#define NM_PROTOCOL_WIFI_SET_MSG_LEN      24 //24 bytes message length
#define NM_PROTOCOL_WIFI_SET_MSG_PACK_LEN 48 //48 bytes package length

#define NM_PANEL_SHUTDOWN_CONFIRM_ID      0x05
#define NM_PANEL_RST_ID                   0x04
#define NM_PROTOCOL_RST_RESP_MSG_LEN      2  //2 bytes message length
#define NM_PROTOCOL_RST_RESP_MSG_PACK_LEN 26 //26 bytes package length

#define NM_PANEL_SET1_ID                  0x06
#define NM_PANEL_SET3_ID                  0x08
#define NM_PANEL_SET6_ID                  0x09
/* Wi-Fi 状态同步；XBase 分支中称为 SET7，线缆格式相同 */
#define NM_PANEL_SET_WIFI_ID              0x0C
/* 仅 PRODUCT_PROTOCOL_SATELLITE：星图卫星数 */
#define NM_PANEL_SET_SATELLITE_ID         0x0D
#define NM_PANEL_SET3_1_ID                0x03

#ifdef __cplusplus
/* ---- 带屏产品载荷布局（通用部分见 panel_protocol.h） ---- */

/** 工作状态：INFO3 0x07 响应与 SET1 0x06 请求同一布局（88 字节）。
 * satellite_track / external_power / poweroff_request 仅 SET1 使用，INFO3 中为 0。 */
struct PANEL_PAYLOAD DisplayWorkPayload {
    uint8_t work_mode;
    uint8_t satellite_used;
    uint8_t coordinate_status;
    uint8_t gprs_status;
    uint8_t ntrip_server_status;
    uint8_t ntrip_client_status;
    uint8_t radio_status;
    uint8_t satellite_track;
    uint8_t external_power;
    uint8_t reserved9[2];
    uint8_t radio_change_flag; // 面板工作模式请求待主控确认
    uint8_t ntrip_server_ip[4];
    uint8_t ntrip_server_mountpoint[32];
    uint8_t ntrip_client_ip[4];
    uint8_t ntrip_client_mountpoint[32];
    uint8_t radio_mode;
    uint8_t radio_protocol; // 主控协议编码
    uint8_t radio_channel;
    uint8_t poweroff_request; // 主控请求面板关机
};

static_assert(sizeof(DisplayWorkPayload) == NM_PROTOCOL_INFO3_MSG_LEN, "INFO3/SET1 layout");
static_assert(offsetof(DisplayWorkPayload, radio_change_flag) == 11
                  && offsetof(DisplayWorkPayload, ntrip_server_ip) == 12
                  && offsetof(DisplayWorkPayload, ntrip_client_ip) == 48
                  && offsetof(DisplayWorkPayload, radio_mode) == 84
                  && offsetof(DisplayWorkPayload, poweroff_request) == 87,
              "INFO3/SET1 offsets");

/** 录制：INFO4 0x0A 响应与 SET6 0x09 请求同一布局（28 字节；SET6 最少 27 字节，末字节保留）。 */
struct PANEL_PAYLOAD DisplayRecordPayload {
    uint8_t record_status;
    uint8_t record_op;
    uint8_t reserved2[2];
    float record_leftspace; // MB
    uint8_t record_name[16];
    uint8_t record_type;     // 1-XYZ 2-Rinex3.02
    uint8_t record_interval; // 0xff 不限，0x00 15 min ... 0x18 24 h
    uint8_t record_change_flag;
    uint8_t reserved27;
};

static_assert(sizeof(DisplayRecordPayload) == NM_PROTOCOL_INFO4_MSG_LEN, "INFO4/SET6 layout");
static_assert(offsetof(DisplayRecordPayload, record_leftspace) == 4
                  && offsetof(DisplayRecordPayload, record_type) == 24,
              "INFO4/SET6 offsets");

/** 定位：SET3 0x08 请求（28 字节）。 */
struct PANEL_PAYLOAD DisplayPositionPayload {
    uint8_t coordinate_status;
    uint8_t reserved1[3];
    double longitude;
    double latitude;
    double altitude;
};

static_assert(sizeof(DisplayPositionPayload) == 28 && offsetof(DisplayPositionPayload, longitude) == 4, "SET3 layout");

/** Wi-Fi 开关设定：0x0B 响应（24 字节，仅 PRODUCT_PROTOCOL_WIFI_CONTROL）。 */
struct PANEL_PAYLOAD DisplayWifiControlPayload {
    uint8_t wifi_on;
    uint8_t reserved1[2];
    uint8_t wifi_change_flag;
    uint8_t reserved4[20];
};

static_assert(sizeof(DisplayWifiControlPayload) == NM_PROTOCOL_WIFI_SET_MSG_LEN, "0x0B layout");

/** Wi-Fi 状态：SET_WIFI 0x0C 请求（24 字节）。 */
struct PANEL_PAYLOAD DisplayWifiStatusPayload {
    uint8_t wifi_status;
    uint8_t wifi_mode; // 0 AP，1 STA
    uint8_t reserved2[2];
    uint8_t wifi_ip[4];
    char wifi_ssid[16];
};

static_assert(sizeof(DisplayWifiStatusPayload) == 24 && offsetof(DisplayWifiStatusPayload, wifi_ssid) == 8,
              "0x0C layout");

/** 星图卫星数：0x0D 请求（7 字节，仅 PRODUCT_PROTOCOL_SATELLITE）。 */
struct PANEL_PAYLOAD DisplaySatellitePayload {
    uint8_t gps;
    uint8_t glonass;
    uint8_t galileo;
    uint8_t bds;
    uint8_t sbas;
    uint8_t qzss;
    uint8_t irnss;
};

static_assert(sizeof(DisplaySatellitePayload) == 7, "0x0D layout");
#endif
