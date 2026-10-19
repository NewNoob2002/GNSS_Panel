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
