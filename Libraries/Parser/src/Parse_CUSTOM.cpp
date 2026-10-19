#include "SparkFun_Extensible_Message_Parser.h"

#if defined(PARSER_CUSTOM_CONFIG_ON)

uint32_t
sempBluetoothComputeCrc(SEMP_PARSE_STATE* parse, const uint8_t data) {
    uint32_t crc = parse->crc;
    crc = semp_crc32Table[(crc ^ data) & 0xff] ^ (crc >> 8);
    return crc;
}

//    |<------------- 20 bytes ------------------> |<----- MsgData ----->|<- 4
//    bytes ->| |                                            | |      |
//    +----------+--------+----------------+---------+----------+-------------+
//    | Synchron   |  HeaderLen  | Message ID&Type | Message     |   CRC-32Q   |
//    |  24 bits   |   8 bits    |    128 bits     |  n-bits     |   32 bits   |
//    | 0xAA 44 18 |   0x14      |   (in 16bytes)  |             |             |
//    +----------+--------+----------------+---------+----------+-------------+
//    |                                                         |
//    |<------------------------ CRC ------------------------->|

// Read the CRC
bool
sempCustomReadCrc(SEMP_PARSE_STATE* parse, uint8_t data) {
    (void)data;
    auto* scratchPad = static_cast<SEMP_SCRATCH_PAD*>(parse->scratchPad);

    // Determine if the entire message was read
    if (--scratchPad->custom.bytesRemaining) {
        // Need more bytes
        return true;
    }

    // uint32_t crcRead = parse->crc;

    // uint32_t crcComputed = scratchPad->bluetooth.crc;
    // Call the end-of-message routine with this message
    if ((!parse->crc) || (parse->badCrc && (!parse->badCrc(parse)))) {
        parse->eomCallback(parse, parse->type); // Pass parser array index
    } else {
        sempPrintf(parse->printDebug,
                   "SEMP: %s Unicore, bad CRC, "
                   "received %02x %02x %02x %02x, computed: %02x %02x %02x %02x",
                   parse->parserName, parse->buffer[parse->length - 4], parse->buffer[parse->length - 3],
                   parse->buffer[parse->length - 2], parse->buffer[parse->length - 1], scratchPad->custom.crc & 0xff,
                   (scratchPad->custom.crc >> 8) & 0xff, (scratchPad->custom.crc >> 16) & 0xff,
                   (scratchPad->custom.crc >> 24) & 0xff);
    }
    parse->state = sempFirstByte;
    return false;
}

bool
sempCustomReadData(SEMP_PARSE_STATE* parse, uint8_t data) {
    (void)data;
    auto* scratchPad = static_cast<SEMP_SCRATCH_PAD*>(parse->scratchPad);

    // Determine if the entire message was read
    if (!--scratchPad->custom.bytesRemaining) {
        // The message data is complete, read the CRC
        scratchPad->custom.bytesRemaining = 4;
        parse->crc ^= 0xFFFFFFFF;
        scratchPad->custom.crc = parse->crc;
        parse->state = sempCustomReadCrc;
    }
    return true;
}

//  Header
//
//    Bytes     Field Description
//    -----     ----------------------------------------
//      1       Header Length
//      2       Message ID
//      2       Reserved
//      4       ReservedTime
//      2       Message Length
//      2       Reserved
//      1       Sender
//      1       Message Type
//      1       protocol version
//      1       Reserved

bool
sempCustomReadHeader(SEMP_PARSE_STATE* parse, uint8_t data) {
    (void)data;
    auto* scratchPad = static_cast<SEMP_SCRATCH_PAD*>(parse->scratchPad);

    if (parse->length >= sizeof(SEMP_CUSTOM_HEADER)) {
        const uint16_t payload_length =
            static_cast<uint16_t>(parse->buffer[12]) | (static_cast<uint16_t>(parse->buffer[13]) << 8);
        if (parse->buffer[3] != sizeof(SEMP_CUSTOM_HEADER)
            || static_cast<uint32_t>(sizeof(SEMP_CUSTOM_HEADER)) + payload_length + 4U > parse->bufferLength) {
            parse->state = sempFirstByte;
            return false;
        }
        scratchPad->custom.bytesRemaining = payload_length;
        if (payload_length == 0U) {
            scratchPad->custom.bytesRemaining = 4U;
            parse->crc ^= 0xFFFFFFFFU;
            scratchPad->custom.crc = parse->crc;
            parse->state = sempCustomReadCrc;
        } else {
            parse->state = sempCustomReadData;
        }
    }
    return true;
}

bool
sempCustomSync3(SEMP_PARSE_STATE* parse, const uint8_t data) {
    // Verify sync byte 2
    if (data != 0x18) {
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Look for the last sync byte
    parse->state = sempCustomReadHeader;
    return true;
}

bool
sempCustomSync2(SEMP_PARSE_STATE* parse, const uint8_t data) {
    // Verify sync byte 2
    if (data != 0x44) {
        // Invalid sync byte, start searching for a preamble byte
        return sempFirstByte(parse, data);
    }

    // Look for the last sync byte
    parse->state = sempCustomSync3;
    return true;
}

bool
sempCustomPreamble(SEMP_PARSE_STATE* parse, const uint8_t data) {
    // Determine if this is the beginning of a Unicore message
    if (data != 0xAA) {
        return false;
    }

    // Look for the second sync byte
    parse->crc = 0xFFFFFFFF;
    parse->computeCrc = sempBluetoothComputeCrc;
    parse->crc = parse->computeCrc(parse, data);
    parse->state = sempCustomSync2;
    return true;
}

#endif