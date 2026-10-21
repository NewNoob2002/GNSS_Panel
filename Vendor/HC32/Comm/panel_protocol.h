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

#define NM_PROTOCOL_MSG_SENDER_PANEL   0x03

#define NM_MSG_QUERY_TYPE              0x00
#define NM_MSG_QUERY_RES_TYPE          0x01
#define NM_MSG_SET_TYPE                0x02
#define NM_MSG_SET_RES_TYPE            0x03


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
/* 请求帧头中声明的载荷长度（已由协议核心校验与帧长一致） */
uint16_t panel_request_payload_length(const SEMP_PARSE_STATE* parse);

int message_decode(SEMP_PARSE_STATE* parse, uint8_t* txBuffer, uint16_t txBufferSize);
void message_response_complete(const uint8_t* packet, uint16_t packetSize);

#ifdef __cplusplus
}

#include <cstring>
#include <type_traits>

/*
 * 载荷结构体：每种报文的载荷用 packed 结构体描述线上布局（字段顺序即字节偏移，多字节字段为
 * 小端，MCU 与主控均为小端）。结构体只按值拷贝进出缓冲区，不把缓冲区指针转换成结构体指针，
 * 避免非对齐访问。各结构体用 static_assert 固定长度与关键偏移。
 */
#define PANEL_PAYLOAD __attribute__((packed))

/** 从请求载荷读出 T；载荷短于 T（如可选尾部字段）时其余字段为 0。 */
template <typename T>
T
panel_payload(const uint8_t* payload, uint16_t length) {
    static_assert(std::is_trivially_copyable<T>::value, "payload must be trivially copyable");
    T value{};
    std::memcpy(&value, payload, length < sizeof(T) ? length : sizeof(T));
    return value;
}

template <typename T>
T
panel_request(const SEMP_PARSE_STATE* parse) {
    return panel_payload<T>(parse->buffer + NM_PROTOCOL_HEADER_LEN, panel_request_payload_length(parse));
}

/** 把 T 写入响应载荷（msg 指向整帧；协议核心已清零并负责帧头与 CRC）。 */
template <typename T>
void
panel_write(uint8_t* msg, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "payload must be trivially copyable");
    std::memcpy(msg + NM_PROTOCOL_HEADER_LEN, &value, sizeof(T));
}

/** 单字节标志请求（复位 0x04、关机 0x05、X1 录制 0x03）。 */
struct PANEL_PAYLOAD PanelFlagRequest {
    uint8_t value;
};

static_assert(sizeof(PanelFlagRequest) == 1, "flag request layout");

/** 设置类请求的通用 ACK（NM_PROTOCOL_SET_MSG_LEN）：result = 1 表示成功。 */
struct PANEL_PAYLOAD PanelSetAck {
    uint8_t result;
    uint8_t reserved;
};

static_assert(sizeof(PanelSetAck) == NM_PROTOCOL_SET_MSG_LEN, "set ack layout");

/** 设备信息（全部产品 0x01，36 字节）。 */
struct PANEL_PAYLOAD PanelInfoPayload {
    char hardware_version[8];
    char software_version[10];
    uint16_t battery_percent;
    uint16_t battery_temperature; // 0.1 K（BQ40Z50 原始值）
    uint16_t battery_voltage;     // V（整数）
    uint8_t reserved24[2];
    uint8_t status; // X1 固定为 1（沿用 X1_Panel_dev）；带屏产品为 0
    uint8_t reserved27[9];
};

static_assert(sizeof(PanelInfoPayload) == 36, "info layout");
static_assert(offsetof(PanelInfoPayload, battery_percent) == 18 && offsetof(PanelInfoPayload, status) == 26,
              "info offsets");

/** 面板状态（带屏 INFO2 / X1 CTRL，均为 0x02，8 字节）。 */
struct PANEL_PAYLOAD PanelStatusPayload {
    uint8_t reset;         // 主控请求的复位已生效
    uint8_t poweroff;      // 面板请求关机
    uint8_t record_status; // 当前（含待确认请求）录制状态
    uint8_t record_op;     // 面板切换了录制，待主控确认
    uint8_t charging;      // 正在充电（X1 旧名 PEP）
    uint8_t reserved[3];
};

static_assert(sizeof(PanelStatusPayload) == 8, "status layout");
#endif
