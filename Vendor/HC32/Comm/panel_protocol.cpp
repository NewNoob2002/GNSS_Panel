#include "panel_protocol.h"
#include <cstring>
#include "SystemState.h"
#include "core_debug.h"
#include "mcu_define.h"

static uint32_t
calculate_crc(const char* msg, unsigned int len) {
    if (msg == nullptr || len == 0) {
        return 0;
    }
    unsigned long crc = 0xFFFFFFFF;
    for (unsigned int n = 0; n < len; n++) {
        crc = semp_crc32Table[(crc ^ msg[n]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

static uint16_t
read_u16_le(const uint8_t* data) {
    return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

static uint16_t
request_message_id(const SEMP_PARSE_STATE* parse) {
    return read_u16_le(&parse->buffer[NM_PROTOCOL_MSG_ID_INDEX_L]);
}

static uint16_t
request_payload_length(const SEMP_PARSE_STATE* parse) {
    return read_u16_le(&parse->buffer[NM_PROTOCOL_MSG_LEN_INDEX_L]);
}

static void
write_u16_le(uint8_t* data, const uint16_t value) {
    data[0] = static_cast<uint8_t>(value & 0x00FF);
    data[1] = static_cast<uint8_t>((value >> 8) & 0x00FF);
}

static void
write_crc(uint8_t* msg, const uint16_t crcIndex) {
    const uint32_t crc = calculate_crc(reinterpret_cast<const char*>(msg), crcIndex);
    msg[crcIndex + 0] = static_cast<uint8_t>(crc & 0x000000FF);
    msg[crcIndex + 1] = static_cast<uint8_t>((crc >> 8) & 0x000000FF);
    msg[crcIndex + 2] = static_cast<uint8_t>((crc >> 16) & 0x000000FF);
    msg[crcIndex + 3] = static_cast<uint8_t>((crc >> 24) & 0x000000FF);
}

void
panel_write_success_payload(uint8_t* msg) {
    panel_write(msg, PanelSetAck{1, 0});
}

uint16_t
panel_request_payload_length(const SEMP_PARSE_STATE* parse) {
    return request_payload_length(parse);
}

static const PanelPacket*
find_packet(const uint16_t messageId) {
    size_t count = 0;
    const PanelPacket* packets = panel_protocol_packets(&count);
    for (size_t i = 0; i < count; i++) {
        if (packets[i].messageId == messageId) {
            return &packets[i];
        }
    }
    return nullptr;
}

static bool
validate_request(const SEMP_PARSE_STATE* parse, const PanelPacket* packet) {
    if (parse->length < (NM_PROTOCOL_HEADER_LEN + NM_PROTOCOL_CRC_LEN)) {
        return false;
    }

    const uint16_t payloadLength = request_payload_length(parse);
    if (payloadLength < packet->minRequestPayloadLength) {
        return false;
    }

    if (parse->buffer[3] != NM_PROTOCOL_HEADER_LEN
        || parse->buffer[NM_PROTOCOL_MSG_TYPE_INDEX] + 1U != packet->responseType) {
        return false;
    }
    const uint32_t expectedLength = static_cast<uint32_t>(NM_PROTOCOL_HEADER_LEN) + payloadLength + NM_PROTOCOL_CRC_LEN;
    if (parse->length != expectedLength) {
        return false;
    }
    return !packet->validate || packet->validate(parse->buffer + NM_PROTOCOL_HEADER_LEN, payloadLength);
}

static int
encode_response(const SEMP_PARSE_STATE* parse, const PanelPacket* packet, uint8_t* txBuffer,
                const uint16_t txBufferSize) {
    const uint16_t packetLength =
        static_cast<uint16_t>(NM_PROTOCOL_HEADER_LEN + packet->responsePayloadLength + NM_PROTOCOL_CRC_LEN);
    if (packetLength > txBufferSize) {
        CORE_DEBUG_PRINTF("response too long: id=0x%04X, len=%u, buffer=%u", packet->messageId, packetLength,
                          txBufferSize);
        return 0;
    }

    memset(txBuffer, 0, packetLength);
    txBuffer[0] = NM_PROTOCOL_SYN_BYTE1;
    txBuffer[1] = NM_PROTOCOL_SYN_BYTE2;
    txBuffer[2] = NM_PROTOCOL_SYN_BYTE3;
    txBuffer[3] = NM_PROTOCOL_HEADER_LEN;
    txBuffer[NM_PROTOCOL_MSG_ID_INDEX_L] = parse->buffer[NM_PROTOCOL_MSG_ID_INDEX_L];
    txBuffer[NM_PROTOCOL_MSG_ID_INDEX_H] = parse->buffer[NM_PROTOCOL_MSG_ID_INDEX_H];
    write_u16_le(&txBuffer[NM_PROTOCOL_MSG_LEN_INDEX_L], packet->responsePayloadLength);
    txBuffer[NM_PROTOCOL_MSG_SENDER_INDEX] = NM_PROTOCOL_MSG_SENDER_PANEL;
    txBuffer[NM_PROTOCOL_MSG_TYPE_INDEX] = packet->responseType;

    packet->fill(parse, txBuffer);
    write_crc(txBuffer, NM_PROTOCOL_HEADER_LEN + packet->responsePayloadLength);
    return packetLength;
}

int
message_decode(SEMP_PARSE_STATE* parse, uint8_t* txBuffer, const uint16_t txBufferSize) {
    if (!parse || !parse->buffer || !txBuffer) {
        return 0;
    }

    if (parse->length < NM_PROTOCOL_HEADER_LEN) {
        State::Diagnostics().messages.error_count++;
        CORE_DEBUG_PRINTF("short message len: %d", parse->length);
        return 0;
    }

    const uint16_t messageId = request_message_id(parse);
    const PanelPacket* packet = find_packet(messageId);
    if (packet == nullptr) {
        State::Diagnostics().messages.error_count++;
        CORE_DEBUG_PRINTF("unknown message id: 0x%04X, len: %d", messageId, parse->length);
        return 0;
    }

    if (!validate_request(parse, packet)) {
        State::Diagnostics().messages.error_count++;
        CORE_DEBUG_PRINTF("invalid message: id=0x%04X, payload=%u, len=%d", messageId, request_payload_length(parse),
                          parse->length);
        return 0;
    }

    State::Host().online = true;
    return encode_response(parse, packet, txBuffer, txBufferSize);
}

void
message_response_complete(const uint8_t* packet, uint16_t packetSize) {
    if (!packet || packetSize < NM_PROTOCOL_HEADER_LEN + NM_PROTOCOL_CRC_LEN) {
        return;
    }
    // 完整读出仅表示主控收到通知；设置的实际生效状态仍由后续 SET 帧更新。
    const PanelPacket* descriptor = find_packet(read_u16_le(packet + NM_PROTOCOL_MSG_ID_INDEX_L));
    if (descriptor && descriptor->readComplete) {
        descriptor->readComplete();
    }
}
