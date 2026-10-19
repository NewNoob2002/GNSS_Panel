#pragma once

// 面板与 Linux 主控之间的共用帧格式：20 字节帧头 + 载荷 + CRC32（小端）。
// 帧校验、响应编码和“完整读出后生效”的生命周期由本模块统一处理，
// 各产品只提供自己的报文表（消息 ID、长度与处理函数）。

#include <stddef.h>
#include <stdint.h>
#include "SparkFun_Extensible_Message_Parser.h"

#define NM_PROTOCOL_SYN_BYTE1          0xaa
#define NM_PROTOCOL_SYN_BYTE2          0x44
#define NM_PROTOCOL_SYN_BYTE3          0x18
#define NM_PROTOCOL_HEADER_LEN         20
#define NM_PROTOCOL_CRC_LEN            4
#define NM_PROTOCOL_MSG_ID_INDEX_L     4
#define NM_PROTOCOL_MSG_ID_INDEX_H     5
#define NM_PROTOCOL_MSG_LEN_INDEX_L    12
#define NM_PROTOCOL_MSG_LEN_INDEX_H    13
#define NM_PROTOCOL_MSG_SENDER_INDEX   16
#define NM_PROTOCOL_MSG_TYPE_INDEX     17
#define NM_PROTOCOL_MSG_INTERVAL_INDEX 19

#define NM_PROTOCOL_MSG_SENDER_DEV     0x01
#define NM_PROTOCOL_MSG_SENDER_APP     0x00
#define NM_PROTOCOL_MSG_SENDER_PANEL   0x03

#define NM_MSG_QUERY_TYPE              0x00
#define NM_MSG_QUERY_RES_TYPE          0x01
#define NM_MSG_SET_TYPE                0x02
#define NM_MSG_SET_RES_TYPE            0x03
#define NM_MSG_ERR_REP_TYPE            0x05

#define NM_MSG_CRC_ERR_REP_ID          0x01
#define NM_MSG_MSG_ID_ERR_REP_ID       0x02
#define NM_MSG_MODULE_ID_ERR_REP_ID    0x03

/* 设置类请求的通用 ACK：载荷 2 字节 */
#define NM_PROTOCOL_SET_MSG_LEN        2
#define NM_PROTOCOL_SET_MSG_PACK_LEN   26

#ifdef __cplusplus
extern "C" {
#endif

/* 填写响应载荷；msg 指向整帧，帧头和 CRC 由协议核心写入 */
typedef void (*PanelPacketFill)(const SEMP_PARSE_STATE* parse, uint8_t* msg);
/* 检查请求载荷取值；为空表示只检查长度 */
typedef bool (*PanelPacketValidate)(const uint8_t* payload, uint16_t payloadLength);
/* 主控完整读出该响应后执行；用于确认通知已送达后再清除标志或执行复位等动作 */
typedef void (*PanelPacketReadComplete)(void);

typedef struct {
    uint16_t messageId;
    uint16_t minRequestPayloadLength;
    uint16_t responsePayloadLength;
    uint8_t responseType; /* 请求类型 + 1 */
    PanelPacketFill fill;
    PanelPacketValidate validate;
    PanelPacketReadComplete readComplete;
} PanelPacket;

/* 由产品实现：返回本产品支持的报文表 */
const PanelPacket* panel_protocol_packets(size_t* count);

void panel_write_success_payload(uint8_t* msg);

int message_decode(SEMP_PARSE_STATE* parse, uint8_t* txBuffer, uint16_t txBufferSize);
void message_response_complete(const uint8_t* packet, uint16_t packetSize);

#ifdef __cplusplus
}
#endif
