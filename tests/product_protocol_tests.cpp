// 产品协议表回归：同一文件分别以 X1 与 XBase 的 product_config.h 编译，验证各自报文集合与生效时机。
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "mcu_define.h"
#include "panel_protocol.h"
#include "product_config.h"

SystemInfo_t systemInfo = {};

static std::vector<uint8_t>
frame(uint16_t id, const std::vector<uint8_t>& payload = {}, uint8_t type = NM_MSG_QUERY_TYPE) {
    std::vector<uint8_t> bytes(NM_PROTOCOL_HEADER_LEN, 0);
    bytes[0] = NM_PROTOCOL_SYN_BYTE1;
    bytes[1] = NM_PROTOCOL_SYN_BYTE2;
    bytes[2] = NM_PROTOCOL_SYN_BYTE3;
    bytes[3] = NM_PROTOCOL_HEADER_LEN;
    bytes[NM_PROTOCOL_MSG_ID_INDEX_L] = id & 255U;
    bytes[NM_PROTOCOL_MSG_ID_INDEX_H] = id >> 8;
    bytes[NM_PROTOCOL_MSG_LEN_INDEX_L] = payload.size() & 255U;
    bytes[NM_PROTOCOL_MSG_LEN_INDEX_H] = payload.size() >> 8;
    bytes[NM_PROTOCOL_MSG_TYPE_INDEX] = type;
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    uint32_t crc = 0xffffffffU;
    for (uint8_t byte : bytes) {
        crc = semp_crc32Table[(crc ^ byte) & 255U] ^ (crc >> 8);
    }
    crc ^= 0xffffffffU;
    for (unsigned i = 0; i < 4; ++i) {
        bytes.push_back((crc >> (8 * i)) & 255U);
    }
    return bytes;
}

// 解码一帧并返回响应长度；response 保存编码结果。
static int
decode(std::vector<uint8_t> bytes, uint8_t* response, uint16_t size = 128) {
    SEMP_PARSE_STATE parse = {};
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    return message_decode(&parse, response, size);
}

#if PRODUCT_HAS_DISPLAY
// XBase：没有 0x0B Wi-Fi 查询与 0x0D 星图；0x0C Wi-Fi 状态照常同步。
static void
test_xbase_packets() {
    uint8_t response[128];
    assert(decode(frame(0x0B), response) == 0);
    assert(decode(frame(0x0D, std::vector<uint8_t>(7, 1), NM_MSG_SET_TYPE), response) == 0);
    std::vector<uint8_t> wifi(24, 0);
    wifi[0] = 1;
    memcpy(&wifi[8], "base-ap", 7);
    assert(decode(frame(0x0C, wifi, NM_MSG_SET_TYPE), response) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(systemInfo.wifiInfo.wifi_status == On_Off_Status_ON
           && strcmp(systemInfo.wifiInfo.wifi_ssid, "base-ap") == 0);
    assert(decode(frame(0x07), response) > 0);
}
#else
// X1：0x02 读出关机标志后才确认关机；0x04 复位在 ACK 完整读出后生效；0x03 同步录制状态。
static void
test_x1_packets() {
    uint8_t response[128];
    const int info = decode(frame(0x01), response);
    assert(info == NM_PROTOCOL_HEADER_LEN + 36 + NM_PROTOCOL_CRC_LEN);
    assert(memcmp(&response[NM_PROTOCOL_HEADER_LEN], HW_VERSION, sizeof(HW_VERSION) - 1U) == 0);
    assert(response[NM_PROTOCOL_HEADER_LEN + 26] == 1);

    // 不属于 X1 的显示面板报文不响应。
    assert(decode(frame(0x07), response) == 0);
    assert(decode(frame(0x06, std::vector<uint8_t>(88, 0), NM_MSG_SET_TYPE), response) == 0);

    systemInfo.powerMonitor.poweroff_flag = 1;
    systemInfo.recordInfo.record_op = 1;
    const int ctrl = decode(frame(0x02), response);
    assert(ctrl == NM_PROTOCOL_HEADER_LEN + 8 + NM_PROTOCOL_CRC_LEN);
    assert(response[NM_PROTOCOL_HEADER_LEN + 1] == 1 && response[NM_PROTOCOL_HEADER_LEN + 3] == 1);
    assert(!systemInfo.powerMonitor.ShutdownEnsure);
    message_response_complete(response, static_cast<uint16_t>(ctrl));
    assert(systemInfo.powerMonitor.ShutdownEnsure && systemInfo.powerMonitor.poweroff_flag == 0);
    assert(systemInfo.recordInfo.record_op == 0);

    // 面板按键切换录制后、主控读走前，主控的 0x03 不覆盖面板状态。
    systemInfo.recordInfo.record_status = On_Off_Status_ON;
    systemInfo.recordInfo.record_op = 1;
    assert(decode(frame(0x03, {0, 0, 0, 0, 0, 0, 0, 0}, NM_MSG_SET_TYPE), response) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(systemInfo.recordInfo.record_status == On_Off_Status_ON);
    systemInfo.recordInfo.record_op = 0;
    assert(decode(frame(0x03, {0, 0, 0, 0, 0, 0, 0, 0}, NM_MSG_SET_TYPE), response) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(systemInfo.recordInfo.record_status == On_Off_Status_OFF);
    assert(decode(frame(0x03, {2}, NM_MSG_SET_TYPE), response) == 0);

    const int reset = decode(frame(0x04, {1}, NM_MSG_SET_TYPE), response);
    assert(reset == NM_PROTOCOL_SET_MSG_PACK_LEN && response[NM_PROTOCOL_HEADER_LEN] == 1);
    assert(systemInfo.powerMonitor.reset_flag == 0);
    message_response_complete(response, static_cast<uint16_t>(reset));
    assert(systemInfo.powerMonitor.reset_flag == 1);

    const int power = decode(frame(0x05, {1}, NM_MSG_SET_TYPE), response);
    assert(power == NM_PROTOCOL_SET_MSG_PACK_LEN && !systemInfo.powerMonitor.LinuxPowerOff);
    message_response_complete(response, static_cast<uint16_t>(power));
    assert(systemInfo.powerMonitor.LinuxPowerOff);
}
#endif

int
main() {
#if PRODUCT_HAS_DISPLAY
    test_xbase_packets();
#else
    test_x1_packets();
#endif
    printf("%s protocol regressions passed\n", PRODUCT_NAME);
}
