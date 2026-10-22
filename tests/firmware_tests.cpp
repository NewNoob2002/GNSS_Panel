#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>
#include "product_config.h"
#include "BlinkLed.h"
#include "Charger.h"
#include <HAL.h>
#include "HoldTimer.h"
#include "PowerPolicy.h"
#include "SystemState.h"
#include "board_peripherals.h"
#include "bq40z50.h"
#include "mp2762a.h"
#if !defined(POWER_TESTS_ONLY)
#include "panel_protocol.h"
#include "display_protocol.h"
#include "slave_i2c.h"
#endif

CM_I2C_TypeDef fake_i2c1, fake_i2c2;
FakeGpio fake_gpio;
func_ptr_t fake_irqs[16] = {};
uint32_t fake_ms, fake_primask, fake_slave_initializations, fake_master_resets;
int fake_io_error, fake_receive_error, fake_slave_init_error;
int fake_master_init_error;
char fake_master_failure;
std::string fake_master_trace;
stc_i2c_init_t fake_master_config;
uint8_t fake_address, fake_register;
uint16_t fake_registers[128][256] = {};
uint32_t fake_master_writes;
int fake_pins[32] = {};
uint32_t fake_system_resets;

void
HAL::HAL_Update() {}

void
App_Update() {}

#ifndef POWER_TESTS_ONLY
static std::vector<uint8_t>
frame(uint16_t id, const std::vector<uint8_t>& payload = {}, uint8_t type = NM_MSG_QUERY_TYPE) {
    std::vector<uint8_t> bytes(NM_PROTOCOL_HEADER_LEN, 0);
    bytes[0] = 0xaa;
    bytes[1] = 0x44;
    bytes[2] = 0x18;
    bytes[3] = NM_PROTOCOL_HEADER_LEN;
    bytes[4] = id & 255U;
    bytes[5] = id >> 8;
    bytes[12] = payload.size() & 255U;
    bytes[13] = payload.size() >> 8;
    bytes[17] = type;
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

static void
write_request(const std::vector<uint8_t>& bytes) {
    fake_i2c1.SR = I2C_FLAG_MATCH_ADDR0 | I2C_FLAG_START | I2C_FLAG_BUSY;
    fake_irqs[I2C_EEI_IRQN_DEF]();
    for (uint8_t byte : bytes) {
        fake_i2c1.DR = byte;
        fake_irqs[I2C_RXI_IRQN_DEF]();
    }
    fake_i2c1.SR = I2C_FLAG_STOP | I2C_FLAG_START | I2C_FLAG_BUSY;
    fake_irqs[I2C_EEI_IRQN_DEF]();
    assert(!(fake_i2c1.SR & (I2C_FLAG_STOP | I2C_FLAG_START | I2C_FLAG_BUSY)));
    for (unsigned i = 0; i < (bytes.size() + 31U) / 32U; ++i) {
        slave_i2c_update();
    }
}

static std::vector<uint8_t>
read_response(size_t count, bool stop = true) {
    std::vector<uint8_t> bytes;
    fake_i2c1.SR = I2C_FLAG_MATCH_ADDR0 | I2C_FLAG_TRA | I2C_FLAG_START | I2C_FLAG_NACKF | I2C_FLAG_STOP;
    fake_irqs[I2C_EEI_IRQN_DEF]();
    bytes.push_back(fake_i2c1.DR);
    for (size_t i = 1; i < count; ++i) {
        fake_i2c1.SR = I2C_FLAG_TX_CPLT;
        fake_irqs[I2C_TEI_IRQN_DEF]();
        bytes.push_back(fake_i2c1.DR);
    }
    if (stop) {
        // 覆盖最终 NACK+STOP 已清 TRA 的真实外设时序。
        fake_i2c1.SR = I2C_FLAG_NACKF | I2C_FLAG_STOP | I2C_FLAG_START | I2C_FLAG_BUSY;
        fake_irqs[I2C_EEI_IRQN_DEF]();
        slave_i2c_update();
    }
    return bytes;
}

static bool
crc_valid(const std::vector<uint8_t>& bytes) {
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i + 4U < bytes.size(); ++i) {
        crc = semp_crc32Table[(crc ^ bytes[i]) & 255U] ^ (crc >> 8);
    }
    crc ^= 0xffffffffU;
    const size_t n = bytes.size();
    return bytes[n - 4] == (crc & 255U) && bytes[n - 3] == ((crc >> 8) & 255U) && bytes[n - 2] == ((crc >> 16) & 255U)
           && bytes[n - 1] == ((crc >> 24) & 255U);
}

static void
test_slave_transactions() {
    assert(slave_i2c_init() == LL_OK);
    const uint32_t initializations = fake_slave_initializations;
    for (unsigned i = 0; i < 4; ++i) {
        fake_ms += 2500U;
        slave_i2c_update();
    }
    assert(fake_slave_initializations == initializations);
    State::Request_Work(systemInfo.host.work);
    write_request(frame(NM_PANEL_INFO3_ID));
    assert(txBufferAvailable() == NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    assert(systemInfo.request.work_pending);
    read_response(5);
    assert(slave_i2c_diagnostics.partial_reads == 1);
    assert(txBufferAvailable() == NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    write_request(frame(NM_PANEL_SET1_ID, std::vector<uint8_t>(88, 0), NM_MSG_SET_TYPE));
    assert(slave_i2c_diagnostics.response_busy == 1);
    auto response = read_response(NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    assert(response[4] == NM_PANEL_INFO3_ID && crc_valid(response));
    assert(!systemInfo.request.work_pending);
    assert(txBufferAvailable() == 0);

    auto bad = frame(NM_PANEL_SET1_ID, std::vector<uint8_t>(88, 0), NM_MSG_SET_TYPE);
    bad.back() ^= 1;
    const auto set_count = systemInfo.diag.messages.Set1_count;
    write_request(bad);
    assert(txBufferAvailable() == 0 && systemInfo.diag.messages.Set1_count == set_count);

    write_request(frame(NM_PANEL_INFO1_ID));
    read_response(3, false);
    fake_i2c1.SR = I2C_FLAG_BUSY;
    slave_i2c_update();
    fake_ms += 2001U;
    slave_i2c_update();
    assert(slave_i2c_diagnostics.recoveries == 1);
    response = read_response(NM_PROTOCOL_INFO1_MSG_PACK_LEN);
    assert(response[4] == NM_PANEL_INFO1_ID && crc_valid(response));

    fake_i2c1.SR = I2C_FLAG_BUSY;
    fake_ms += 2001U;
    slave_i2c_update();
    assert(slave_i2c_diagnostics.recoveries == 2);
    fake_ms += 10000U;
    slave_i2c_update();
    assert(slave_i2c_diagnostics.recoveries == 2);

    auto partial = frame(NM_PANEL_INFO1_ID);
    partial.resize(8);
    write_request(partial);
    fake_ms += 251U;
    slave_i2c_update();
    assert(slave_i2c_diagnostics.parser_timeouts == 1);
    write_request(frame(NM_PANEL_INFO1_ID));
    assert(crc_valid(read_response(NM_PROTOCOL_INFO1_MSG_PACK_LEN)));

    fake_primask = 1;
    assert(slave_i2c_init() == LL_OK);
    assert(fake_primask == 1);
    fake_primask = 0;
    write_request(frame(NM_PANEL_RST_ID, {1}, NM_MSG_SET_TYPE));
    assert(State::Power().reset_flag == 0);
    read_response(4);
    assert(State::Power().reset_flag == 0);
    read_response(NM_PROTOCOL_RST_RESP_MSG_PACK_LEN);
    assert(State::Power().reset_flag == 1);
    State::Power().reset_flag = 0;

    // 关机确认方式一：主控完整读到含关机标志的 INFO2；短读不算确认。
    State::Power().poweroff_flag = 1;
    write_request(frame(NM_PANEL_INFO2_ID));
    read_response(4);
    assert(!systemInfo.power.ShutdownEnsure && State::Power().poweroff_flag == 1);
    read_response(NM_PROTOCOL_PINFO2_MSG_PACK_LEN);
    assert(systemInfo.power.ShutdownEnsure && State::Power().poweroff_flag == 0);
    State::Power().ShutdownEnsure = false;
    // 方式二：主控发送 0x05=1，ACK 完整读出后确认。
    State::Power().poweroff_flag = 1;
    write_request(frame(NM_PANEL_SHUTDOWN_CONFIRM_ID, {1}, NM_MSG_SET_TYPE));
    read_response(4);
    assert(!systemInfo.power.ShutdownEnsure);
    read_response(NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(systemInfo.power.ShutdownEnsure && State::Power().poweroff_flag == 0);
    State::Power().ShutdownEnsure = false;
    State::Power().LinuxPowerOff = false;

    write_request(frame(NM_PANEL_INFO1_ID));
    fake_slave_init_error = LL_ERR;
    fake_i2c1.SR = I2C_FLAG_BUSY;
    fake_ms += 2001U;
    slave_i2c_update();
    assert(slave_i2c_diagnostics.init_failures == 1);
    fake_slave_init_error = 0;
    fake_ms += 2001U;
    slave_i2c_update();
    assert(crc_valid(read_response(NM_PROTOCOL_INFO1_MSG_PACK_LEN)));
}

static void
test_decoder_boundary() {
    uint8_t response[256];
    auto bytes = frame(NM_PANEL_SET_WIFI_ID, std::vector<uint8_t>(24, 0), NM_MSG_SET_TYPE);
    bytes[20] = 2;
    SEMP_PARSE_STATE parse = {};
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == 0);
    bytes = frame(NM_PANEL_SET3_ID, std::vector<uint8_t>(28, 0), NM_MSG_SET_TYPE);
    const double invalid = std::numeric_limits<double>::quiet_NaN();
    memcpy(bytes.data() + 24, &invalid, sizeof(invalid));
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == 0);
    bytes = frame(NM_PANEL_INFO1_ID);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    memset(State::Board().hardware_version, 'H', sizeof(systemInfo.board.hardware_version));
    memset(State::Board().software_version, 'S', sizeof(systemInfo.board.software_version));
    assert(message_decode(&parse, response, 4) == 0);
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_INFO1_MSG_PACK_LEN);
    assert(response[20] == 'H' && response[27] == 'H' && response[28] == 'S' && response[37] == 'S');
    assert(message_decode(nullptr, response, sizeof(response)) == 0);

    State::Request_Work(systemInfo.host.work);
    bytes = frame(NM_PANEL_INFO3_ID);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) > 0);
    // 读出期间面板又改了请求：本次读出不能确认新请求。
    WorkConfig_t changed = systemInfo.request.work;
    ++changed.radio_channel;
    State::Request_Work(changed);
    message_response_complete(response, sizeof(response));
    assert(systemInfo.request.work_pending);
}

// 线上字节偏移固定（独立于载荷结构体，按主控协议文档的偏移直接读写）。
static void
test_wire_offsets() {
    State::Reset();
    uint8_t response[256];
    SEMP_PARSE_STATE parse = {};
    std::vector<uint8_t> set1(88, 0);
    set1[0] = base_mode;
    set1[1] = 21;   // 使用卫星
    set1[2] = 4;    // 固定解
    set1[6] = 1;    // 电台开
    set1[7] = 30;   // 跟踪卫星
    set1[8] = 1;    // 外部供电
    set1[12] = 10;  // NTRIP 服务器 IP
    set1[16] = 'M'; // 挂载点
    set1[48] = 172; // NTRIP 客户端 IP
    set1[84] = radio_mode_tx;
    set1[85] = 13; // 协议编码
    set1[86] = 7;  // 信道
    auto bytes = frame(NM_PANEL_SET1_ID, set1, NM_MSG_SET_TYPE);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(response[20] == 1 && response[21] == 0);
    const HostReport_t& host = systemInfo.host;
    assert(host.work.work_mode == base_mode && host.position.satellite_number_used == 21);
    assert(host.position.coordinate_status == position_fix && host.work.radio_status == On_Off_Status_ON);
    assert(host.position.satellite_number_track == 30 && systemInfo.power.ExternalPower == 1);
    assert(host.ntrip.NtripServer_IP[0] == 10 && host.ntrip.NtripServer_Mountpoint[0] == 'M');
    assert(host.ntrip.NtripClient_IP[0] == 172);
    assert(host.work.radio_mode == radio_mode_tx && host.work.radio_protocol == 13 && host.work.radio_channel == 7);
    assert(!systemInfo.request.work_pending && !systemInfo.power.LinuxPowerOff);

    // INFO3：面板请求待确认时上报请求值与变更标志。
    WorkConfig_t work = host.work;
    work.radio_channel = 3;
    State::Request_Work(work);
    bytes = frame(NM_PANEL_INFO3_ID);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    const uint8_t* info3 = response + NM_PROTOCOL_HEADER_LEN;
    assert(info3[0] == base_mode && info3[1] == 21 && info3[2] == 4 && info3[6] == 1 && info3[7] == 0);
    assert(info3[11] == 1 && info3[12] == 10 && info3[16] == 'M' && info3[48] == 172);
    assert(info3[84] == radio_mode_tx && info3[85] == 13 && info3[86] == 3 && info3[87] == 0);

    // INFO4 / SET6：录制空间（float，偏移 4）、文件名（偏移 8）、类型/间隔（24/25）。
    std::vector<uint8_t> set6(27, 0);
    set6[0] = 1;
    const float space = 512.5f;
    memcpy(&set6[4], &space, sizeof(space));
    set6[8] = 'R';
    set6[24] = 2;
    set6[25] = 0x18;
    bytes = frame(NM_PANEL_SET6_ID, set6, NM_MSG_SET_TYPE);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(host.record.record_status == On_Off_Status_ON && host.record.record_leftspace == space);
    assert(host.record.record_name[0] == 'R' && host.record.record_type == 2 && host.record.record_interval == 0x18);
    bytes = frame(NM_PANEL_INFO4_ID);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) > 0);
    const uint8_t* info4 = response + NM_PROTOCOL_HEADER_LEN;
    float reported_space;
    memcpy(&reported_space, info4 + 4, sizeof(reported_space));
    assert(info4[0] == 1 && reported_space == space && info4[8] == 'R' && info4[24] == 2 && info4[25] == 0x18);

    // SET_WIFI：IP 偏移 4，SSID 偏移 8。
    std::vector<uint8_t> wifi(24, 0);
    wifi[0] = 1;
    wifi[1] = 1;
    wifi[4] = 192;
    memcpy(&wifi[8], "station", 7);
    bytes = frame(NM_PANEL_SET_WIFI_ID, wifi, NM_MSG_SET_TYPE);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(host.wifi.wifi_status == On_Off_Status_ON && host.wifi.wifi_mode == 1 && host.wifi.wifi_ip[0] == 192);
    assert(memcmp(host.wifi.wifi_ssid, "station", 7) == 0);
    State::Reset();
}

static int received_frames;

static void
received(SEMP_PARSE_STATE*, uint16_t) {
    ++received_frames;
}

static bool
reject_crc(SEMP_PARSE_STATE*) {
    return true;
}

static std::string printed;

static void
print_capture(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printed += buffer;
}

static void
test_parser_boundaries() {
    const SEMP_PARSE_ROUTINE routines[] = {sempCustomPreamble};
    const char* names[] = {"CUSTOM"};
    auto* p = sempBeginParser(routines, 1, names, 1, 0, 512, received, "test", nullptr, nullptr, reject_crc);
    assert(p);
    auto query = frame(NM_PANEL_INFO1_ID);
    for (auto byte : query) {
        sempParseNextByte(p, byte);
    }
    assert(received_frames == 1);
    auto maximum = frame(1, std::vector<uint8_t>(488, 0));
    for (auto byte : maximum) {
        sempParseNextByte(p, byte);
    }
    assert(received_frames == 2);
    auto invalid_header = frame(1);
    invalid_header[3] = 21;
    for (auto byte : invalid_header) {
        sempParseNextByte(p, byte);
    }
    auto oversized = frame(1, std::vector<uint8_t>(489, 0));
    for (auto byte : oversized) {
        sempParseNextByte(p, byte);
    }
    for (auto byte : query) {
        sempParseNextByte(p, byte);
    }
    assert(received_frames == 3);
    query.back() ^= 1;
    for (auto byte : query) {
        sempParseNextByte(p, byte);
    }
    assert(received_frames == 3);
    sempPrintf(print_capture, "%s", "100% complete");
    assert(printed == "100% complete\r\n");
    sempStopParser(&p);
    assert(p == nullptr);
}

static int scan_callbacks, scan_online;

static void
scan_capture(const uint8_t* address) {
    ++scan_callbacks;
    if (address) {
        assert(*address == scan_callbacks);
        ++scan_online;
    }
}

/** 核对类接口与原事务轨迹，包括频率边界、NACK、超时恢复和双 STOP。 */
static void
test_i2c_master() {
    auto* bus = &board_i2c;
    uint8_t bytes[2] = {0x08, 0x34};
    bus->end();
    fake_master_trace.clear();
    assert(!bus->isReady());
    assert(!bus->probe(0x0b));
    assert(!bus->begin(0) && !bus->begin(400001U));
    assert(fake_master_trace.empty());
    fake_master_init_error = LL_ERR;
    assert(!bus->begin(100000U));
    assert(!bus->isReady() && !bus->probe(0x0b) && fake_master_trace.empty());
    fake_master_init_error = LL_OK;
    for (uint32_t hz : {1U, 100000U, 100001U, 400000U}) {
        assert(bus->begin(hz) && bus->isReady());
        assert(fake_master_config.u32Baudrate == hz);
        assert(fake_master_config.u32ClockDiv == (hz <= 100000U ? I2C_CLK_DIV8 : I2C_CLK_DIV2));
        assert(fake_master_config.u32SclTime == (hz <= 100000U ? 3U : 5U));
    }
    assert(fake_master_trace == "BBBB");
    fake_master_trace.clear();
    assert(!bus->write(0x80, bytes, 2));
    assert(!bus->write(0x0b, nullptr, 2));
    assert(!bus->write(0x0b, bytes, 0));
    assert(!bus->writeRead(0x80, 0x08, bytes, 2));
    assert(!bus->writeRead(0x0b, 0x08, nullptr, 2));
    assert(!bus->writeRead(0x0b, 0x08, bytes, 0));
    if (SIZE_MAX > UINT32_MAX) {
        assert(!bus->write(0x0b, bytes, static_cast<size_t>(UINT32_MAX) + 1U));
        assert(!bus->writeRead(0x0b, 0x08, bytes, static_cast<size_t>(UINT32_MAX) + 1U));
    }
    assert(fake_master_trace.empty());
    assert(bus->write(0x5c, bytes, 2));
    assert(fake_registers[0x5c][0x08] == 0x34 && fake_master_trace == "ERrASWDPAe");
    fake_registers[0x0b][0x08] = 0x1234;
    fake_master_trace.clear();
    assert(bus->writeRead(0x0b, 0x08, bytes, 1));
    assert(bytes[0] == 0x34 && fake_master_trace == "ERrASWDTNVGAe");
    fake_master_trace.clear();
    assert(bus->writeRead(0x0b, 0x08, bytes, 2));
    assert(bytes[0] == 0x34 && bytes[1] == 0x12 && fake_master_trace == "ERrASWDTVGAe");
    for (char step : std::string("SWDTVG")) {
        fake_master_failure = step;
        fake_master_trace.clear();
        assert(!bus->writeRead(0x0b, 0x08, bytes, 1));
        assert(fake_master_trace.size() >= 5 && fake_master_trace.substr(fake_master_trace.size() - 5) == "PARre");
        fake_master_failure = 0;
        assert(bus->probe(0x0b));
    }
    fake_master_failure = 'D';
    fake_master_trace.clear();
    assert(!bus->write(0x5c, bytes, 2));
    assert(fake_master_trace == "ERrASWDPARrePARre");
    fake_master_failure = 'P';
    fake_master_trace.clear();
    assert(!bus->probe(0x0b));
    assert(fake_master_trace == "ERrASWPARre");
    fake_master_failure = 0;
    assert(bus->probe(0) && bus->probe(0x7f));
    assert(bus->scan(scan_capture) == 126);
    assert(scan_callbacks == 126 && scan_online == 126);
    scan_callbacks = scan_online = 0;
    fake_master_failure = 'W';
    assert(bus->scan(scan_capture) == 0);
    assert(scan_callbacks == 126 && scan_online == 0);
    fake_master_failure = 0;
    assert(bus->scan(nullptr) == 126);
    bus->end();
    fake_master_trace.clear();
    assert(!bus->isReady());
    assert(!bus->writeRead(0x0b, 0x08, bytes, 2));
    assert(!bus->probe(0x0b) && fake_master_trace.empty());
    bus->end();

    // 配置其他外设与引脚，核对构造无硬件动作、实例就绪状态相互独立。
    CM_I2C_TypeDef otherRegs;
    fake_fcg1_clock = 0;
    fake_mux_pin = -1;
    I2cMaster other({&otherRegs, 0x1234, PA10, PA11, 60, 61});
    assert(fake_fcg1_clock == 0 && fake_mux_pin == -1 && !other.isReady());
    assert(other.begin(400000U));
    assert(fake_fcg1_clock == 0x1234 && fake_mux_pin == PA11 && fake_mux_function == 61);
    assert(fake_master_regs == &otherRegs && other.isReady() && !bus->isReady());
    assert(other.probe(0x42) && fake_master_regs == &otherRegs);
    other.end();
    assert(!other.isReady() && !bus->isReady());
}

static void
test_device_failures() {
    assert(board_i2c.begin(100000U));
    assert(!bq40z50Begin(nullptr) && !mp2762aBegin(nullptr));
    assert(bq40z50Begin(&board_i2c));
    assert(mp2762aBegin(&board_i2c));
    fake_registers[0x0b][BQ40Z50_RELATIVE_STATE_OF_CHARGE] = 50;
    fake_registers[0x0b][BQ40Z50_REMAINING_CAPACITY] = 1000;
    fake_registers[0x0b][BQ40Z50_FULL_CHARGE_CAPACITY] = 2000;
    fake_registers[0x0b][BQ40Z50_VOLTAGE] = 7800;
    fake_registers[0x0b][BQ40Z50_TEMPERATURE] = 2982;
    fake_ms = 20000;
    BatteryInfo_t battery = {};
    HAL::Battery_Sample(battery);
    assert(battery.sample_valid && battery.Percent == 50 && battery.Percent_f == 50.0f);
    const float voltage = battery.Voltage_f;
    fake_receive_error = LL_ERR;
    uint16_t value = 0x1234;
    assert(!bq40z50ReadRegister16Checked(BQ40Z50_VOLTAGE, &value) && value == 0x1234);
    HAL::Battery_Sample(battery);
    assert(!battery.sample_valid && battery.Voltage_f == voltage);
    uint8_t config = 0;
    const uint32_t writes = fake_master_writes;
    assert(!mp2762setChargerEnabled(true, &config));
    assert(fake_master_writes == writes);
    fake_receive_error = 0;
    fake_registers[0x0b][BQ40Z50_FULL_CHARGE_CAPACITY] = 0;
    HAL::Battery_Sample(battery);
    assert(!battery.sample_valid && bq40z50getRelativeStateOfCharge_float() == -1.0f);
    fake_io_error = LL_ERR;
    assert(!board_i2c.probe(0x0b));
    fake_io_error = 0;
    assert(board_i2c.probe(0x0b));

    State::Board().online_device.mp2762 = false;
    battery.sample_valid = true;
    battery.Temp_f = 50.0f;
    for (int i = 0; i < 20; ++i) {
        HAL::Charger_Protect(battery);
    }
    assert(battery.ChargerDisable && gpio_read(CHARGER_ENABLE_PIN) == GPIO_LOW);
    battery.Temp_f = 25.0f;
    battery.ChargerPlugCount = 3;
    battery.ChargerDetect = 1;
    HAL::Charger_Protect(battery);
    assert(!battery.ChargerDisable && gpio_read(CHARGER_ENABLE_PIN) == GPIO_HIGH);
    // 电量计在线但样本失效：禁止充电。
    State::Board().online_device.bq40z50 = true;
    battery.sample_valid = false;
    HAL::Charger_Protect(battery);
    assert(battery.ChargerDisable && gpio_read(CHARGER_ENABLE_PIN) == GPIO_LOW);
    // 电量计从未识别：交由充电芯片保护，恢复充电。
    State::Board().online_device.bq40z50 = false;
    HAL::Charger_Protect(battery);
    assert(!battery.ChargerDisable && gpio_read(CHARGER_ENABLE_PIN) == GPIO_HIGH);
    State::Board().online_device.mp2762 = true;
    assert(mp2762setChargerEnabled(false, &config));
    assert(mp2762setChargerEnabled(true, &config) && (config & 16));
}

// 面板请求与主控上报分开保存：请求待确认时显示请求值，确认按 revision 判定。
static void
test_state_requests() {
    State::Reset();
    HostReport_t& host = State::Host();
    const PanelRequest_t& request = systemInfo.request;

    // 工作模式：主控上报不改写待确认请求；确认后请求值写入主控上报。
    host.work.work_mode = rover_mode;
    WorkConfig_t work = host.work;
    work.work_mode = autobase_mode;
    State::Request_Work(work);
    host.work.radio_channel = 7; // 主控在确认前的上报照常更新
    assert(State::Effective_Work().work_mode == autobase_mode && State::Effective_Work().radio_channel == 0);
    const uint32_t sent = State::Work_Revision();
    State::Confirm_Work(sent - 1U); // 旧响应不能确认
    assert(request.work_pending);
    State::Confirm_Work(sent);
    assert(!request.work_pending && host.work.work_mode == autobase_mode);
    State::Confirm_Work(sent); // 重复确认无副作用
    assert(!request.work_pending);
    State::Echo_HostWork(); // 主控置位变更标志：把上报值作为请求回传
    assert(request.work_pending && request.work.work_mode == autobase_mode);
    State::Confirm_Work(State::Work_Revision());

    // 录制：低电停止只在录制中发请求；X1 只确认启停，带屏确认同时清除设置待确认。
    State::Request_RecordStop();
    assert(!request.record_op);
    host.record.record_status = On_Off_Status_ON;
    State::Request_RecordStop();
    assert(request.record_op && State::Effective_Record().record_status == On_Off_Status_OFF);
    host.record.record_status = On_Off_Status_ON; // 主控尚未读走：显示仍为停止
    assert(State::Effective_Record().record_status == On_Off_Status_OFF);
    State::Confirm_Record(State::Record_Revision(), false);
    assert(!request.record_op && host.record.record_status == On_Off_Status_OFF);
    State::Request_Record(On_Off_Status_ON, 2, 0x04);
    State::Confirm_Record(State::Record_Revision(), false); // 不带设置的确认不清除设置待确认
    assert(!request.record_op && request.record_settings_pending);
    assert(State::Effective_Record().record_interval == 0x04);
    State::Confirm_Record(State::Record_Revision(), true);
    assert(!request.record_settings_pending && host.record.record_type == 2 && host.record.record_interval == 0x04);

    // Wi-Fi：设定值在确认后保留。
    State::Request_Wifi(true);
    State::Confirm_Wifi(State::Wifi_Revision());
    assert(!request.wifi_pending && request.wifi_on == On_Off_Status_ON);

    // 驻留后开机清除主控数据与请求；复位前的 revision 不能确认复位后的请求。
    const uint32_t before = State::Work_Revision();
    State::Request_Work(work);
    State::Reset_HostData();
    assert(!request.work_pending && !host.online && request.wifi_on == On_Off_Status_OFF);
    State::Request_Work(work);
    State::Confirm_Work(before + 1U);
    assert(request.work_pending);
    State::Reset();
}

// 两个主板版本：V1.3 无 MP2762A（GPIO 控制充电与 USB 通路），V1.5 经 MP2762A。
static void
test_charger_variants() {
    State::Reset();
    BatteryInfo_t battery = {};
    HAL::Charger& gpio = HAL::Charger_Get();
    assert(std::string(gpio.name()) == "GPIO");
    gpio.begin(battery);
    gpio.enter_standby();
    assert(fake_pins[USB_SWITCH_PIN] == HIGH);
    fake_pins[CHARGER_ADC_DETECT_PIN] = HIGH; // 接有 Type-C 供电：保持
    gpio.power_on();
    assert(fake_pins[USB_SWITCH_PIN] == HIGH);
    fake_pins[CHARGER_ADC_DETECT_PIN] = LOW;
    gpio.power_on();
    assert(fake_pins[USB_SWITCH_PIN] == LOW);

    State::Board().online_device.mp2762 = true;
    HAL::Charger& mp2762 = HAL::Charger_Get();
    assert(std::string(mp2762.name()) == "MP2762A");
    battery.chargeStatus = normalCharge;
    mp2762.set_enabled(false, battery);
    assert(battery.ChargerDisable && battery.chargeStatus == notCharge);
    const uint32_t writes = fake_master_writes;
    mp2762.set_enabled(false, battery); // 状态未变不访问芯片
    assert(fake_master_writes == writes);
    // 写入失败时保持禁止状态，下次再试。
    fake_io_error = LL_ERR;
    mp2762.set_enabled(true, battery);
    assert(battery.ChargerDisable);
    fake_io_error = 0;
    mp2762.set_enabled(true, battery);
    assert(!battery.ChargerDisable);
    State::Reset();
}

#endif // POWER_TESTS_ONLY

/** Verify cold-boot rail gating, repeated requests, and software-reset recovery. */
static void
test_power_on() {
    State::Reset();
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms = 100;
    HAL::Power_Init();
    assert(fake_pins[POWER_CONTROL_PIN] == LOW && fake_software_reset == RESET);
    HAL::Power_PowerOffMonitor();
    fake_pins[POWER_KEY_PIN] = HIGH;
    HAL::Power_PowerOffMonitor();
    fake_ms += 1500;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    fake_pins[POWER_KEY_PIN] = LOW;
    HAL::Power_PowerOffMonitor();
    fake_ms += 1000;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    fake_pins[POWER_KEY_PIN] = HIGH;
    HAL::Power_PowerOffMonitor();
    fake_ms += 1999;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    fake_ms += 501;
    HAL::Power_PowerOffMonitor();
#if PRODUCT_HAS_DISPLAY
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    const bool accepted = HAL::Power_RequestOn();
    assert(accepted);
#endif
    assert(systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    assert(systemInfo.power.pannel_power_on_time == fake_ms);
    const uint32_t accepted_at = systemInfo.power.pannel_power_on_time;
    State::Host().online = true;
    fake_ms += 100;
    const bool repeated = HAL::Power_RequestOn();
    assert(repeated && systemInfo.power.pannel_power_on_time == accepted_at && systemInfo.host.online);
    assert(fake_pins[POWER_CONTROL_PIN] == LOW);
    HAL::Power_OnCheck();
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH && fake_pins[POWER_LED_PIN] == HIGH);
    HAL::Power_Shutdown(false);
    const bool during_request = HAL::Power_RequestOn();
    assert(!during_request && systemInfo.power.ShutdownReq && systemInfo.power.panel_power_on);
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH && systemInfo.power.pannel_power_on_time == accepted_at);
    State::Power().ShutdownReq = false;

    State::Reset();
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_software_reset = SET;
    HAL::Power_Init();
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH && fake_software_reset == RESET);
    HAL::Power_PowerOffMonitor();
    assert(systemInfo.power.panel_power_on && systemInfo.power.pannel_power_on_time == fake_ms);
    HAL::Power_OnCheck();
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH);
    State::Reset();
    HAL::Power_Init(); // 恢复普通上电状态，后续测试不沿用软件复位原因。
    assert(fake_pins[POWER_CONTROL_PIN] == LOW);
}

// 强制关机计时与指示灯闪烁：时间差需支持 tick 回绕。
static void
test_hold_and_blink() {
    const uint32_t start = UINT32_MAX - 1000U;
    HoldTimer hold(2500U);
    assert(!hold.update(true, start));
    assert(!hold.update(true, start + 2499U));
    assert(hold.update(true, start + 2500U));
    assert(!hold.update(true, start + 5000U)); // 每次按住只触发一次
    assert(!hold.update(false, start + 5001U));
    assert(!hold.update(true, start + 6000U));
    assert(!hold.update(false, start + 7000U)); // 中途松开重新计时
    assert(!hold.update(true, start + 7001U));
    assert(hold.update(true, start + 9501U));

    BlinkLed led(POWER_LED_PIN);
    led.write(true);
    led.update(start); // 周期 0：不翻转
    assert(fake_pins[POWER_LED_PIN] == HIGH);
    led.set_period(300);
    led.update(start); // 上次翻转时间为 0，回绕后差值已超过周期
    assert(fake_pins[POWER_LED_PIN] == LOW);
    led.update(start + 299U);
    assert(fake_pins[POWER_LED_PIN] == LOW);
    led.update(start + 300U);
    assert(fake_pins[POWER_LED_PIN] == HIGH);
}

static void
test_power_policy() {
    PowerPolicy policy;
    assert(!policy.LowBatteryExpired(0, true));
    assert(!policy.LowBatteryExpired(179999, true));
    assert(!policy.LowBatteryExpired(180000, false));
    assert(!policy.LowBatteryExpired(200000, true));
    assert(policy.LowBatteryExpired(380000, true));
    PowerPolicy wrap;
    const uint32_t start = UINT32_MAX - 100U;
    assert(!wrap.LowBatteryExpired(start, true));
    assert(wrap.LowBatteryExpired(start + 180000U, true));
    assert(!policy.UpdateShutdown(0, true, false, false));
    assert(!policy.UpdateShutdown(20, false, true, false));
    assert(!policy.UpdateShutdown(30, true, true, false));
    assert(policy.UpdateShutdown(8020, false, true, false));
    assert(!policy.UpdateShutdown(8030, true, true, true));
    PowerPolicy timeout;
    assert(!timeout.UpdateShutdown(start, true, false, false));
    assert(!timeout.UpdateShutdown(start + 19999U, false, false, false));
    assert(timeout.UpdateShutdown(start + 20000U, false, false, false));
    PowerPolicy forced;
    assert(forced.UpdateShutdown(1, true, false, true));
}

// 低电量（≤9%、未充电、无外部供电）：立即停止录制，持续 3 min 后关机。
static void
test_low_battery() {
    State::Reset();
    Power_Monitor_t& power = State::Power();
    power.panel_power_on = true;
    power.batteryInfo.sample_valid = true;
    power.batteryInfo.Percent = 9;
    State::Host().record.record_status = On_Off_Status_ON;
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms = 50000;
    HAL::Power_PowerOffMonitor();
    assert(State::Effective_Record().record_status == On_Off_Status_OFF && systemInfo.request.record_op);
    assert(!power.LowBatteryPowerOff && !power.ShutdownGoing);
    fake_ms += 179999U;
    HAL::Power_PowerOffMonitor();
    assert(!power.LowBatteryPowerOff);
    // 外部供电中断计时：恢复低电后重新计 3 min。
    power.ExternalPower = 1;
    HAL::Power_PowerOffMonitor();
    power.ExternalPower = 0;
    fake_ms += 1U;
    HAL::Power_PowerOffMonitor();
    assert(!power.LowBatteryPowerOff);
    fake_ms += 180000U;
    HAL::Power_PowerOffMonitor();
    assert(power.LowBatteryPowerOff);
    HAL::Power_PowerOffMonitor();
    assert(power.ShutdownGoing && power.poweroff_flag == 1);
    const uint32_t powered_at = power.pannel_power_on_time;
    const bool accepted = HAL::Power_RequestOn();
    assert(!accepted && power.ShutdownGoing && power.pannel_power_on_time == powered_at);
    // 关机时序结束（主控未确认，20 s 兜底）进入驻留，为后续测试恢复初始状态。
    fake_ms += 20000U;
    HAL::Power_PowerOffMonitor();
    assert(power.ShutdownComplete && !power.panel_power_on);
    fake_pins[POWER_KEY_PIN] = HIGH;
    HAL::Power_PowerOffMonitor();
    fake_ms += 2000U;
    HAL::Power_PowerOffMonitor();
#if PRODUCT_HAS_DISPLAY
    assert(!power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    const bool resumed = HAL::Power_RequestOn();
    assert(resumed);
#endif
    assert(power.panel_power_on);
    fake_pins[POWER_KEY_PIN] = LOW;
    State::Reset();
}

// 外部供电关机后驻留：不复位；带屏仅接受 Startup 请求，X1 保留原始电源键 2 s 开机。
static void
test_resident_off() {
    State::Reset();
    State::Power().panel_power_on = true;
    State::Power().Force_ShutDown = true;
    State::Power().batteryInfo.sample_valid = true;
    State::Power().batteryInfo.Percent = 70;
    State::Board().online_device.bq40z50 = true;
    std::snprintf(State::Board().hardware_version, sizeof(systemInfo.board.hardware_version), "TEST");
    State::Host().online = true;
    State::Host().record.record_status = On_Off_Status_ON;
    fake_pins[POWER_KEY_PIN] = HIGH; // 关机时按键仍按住
    fake_ms = 1000;
    const uint32_t resets = fake_system_resets;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    assert(systemInfo.power.ShutdownComplete && !systemInfo.power.Force_ShutDown);
    assert(fake_pins[POWER_LED_PIN] == LOW && fake_pins[FUNCTION_LED_PIN] == LOW);
    fake_ms += 1000;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on);

    // 驻留期间的强制关机请求不能带到下次开机。
    State::Power().Force_ShutDown = true;
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms += 5000;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on);
    fake_pins[POWER_KEY_PIN] = HIGH;
    HAL::Power_PowerOffMonitor();
    fake_ms += 1999;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on);
    fake_ms += 1;
    HAL::Power_PowerOffMonitor();
#if PRODUCT_HAS_DISPLAY
    assert(!systemInfo.power.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    assert(systemInfo.host.online && systemInfo.power.ShutdownComplete);
    const bool resumed = HAL::Power_RequestOn();
    assert(resumed);
#endif
    assert(systemInfo.power.panel_power_on && !systemInfo.power.ShutdownComplete);
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH && fake_pins[POWER_LED_PIN] == HIGH);
    assert(!systemInfo.power.Force_ShutDown && !systemInfo.host.online);
    assert(systemInfo.power.batteryInfo.sample_valid && systemInfo.power.batteryInfo.Percent == 70);
    assert(systemInfo.board.online_device.bq40z50 && std::string(systemInfo.board.hardware_version) == "TEST");
    assert(State::Effective_Record().record_status == On_Off_Status_OFF);
    assert(fake_system_resets == resets);
    const uint32_t resumed_at = systemInfo.power.pannel_power_on_time;
    State::Host().online = true;
    ++fake_ms;
    const bool repeated = HAL::Power_RequestOn();
    assert(repeated && systemInfo.host.online && systemInfo.power.pannel_power_on_time == resumed_at);

    // 开机后电源策略重新可用：再次强制关机仍进入驻留。
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms += 100;
    HAL::Power_PowerOffMonitor();
    assert(systemInfo.power.panel_power_on);
    State::Power().Force_ShutDown = true;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.power.panel_power_on && systemInfo.power.ShutdownComplete);
    assert(fake_system_resets == resets);
}

int
main() {
#ifndef POWER_TESTS_ONLY
    test_parser_boundaries();
    test_slave_transactions();
    test_decoder_boundary();
    test_wire_offsets();
    test_i2c_master();
    test_device_failures();
    test_state_requests();
    test_charger_variants();
#endif
    test_power_on();
    test_hold_and_blink();
    test_power_policy();
    test_low_battery();
    test_resident_off();
    puts("protocol, transport, devices and power regressions passed");
}
