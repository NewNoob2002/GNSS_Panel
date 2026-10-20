#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>
#include "BlinkLed.h"
#include "Charger.h"
#include "HoldTimer.h"
#include "HAL.h"
#include "PowerPolicy.h"
#include "Wire.h"
#include "bq40z50.h"
#include "display_protocol.h"
#include "mp2762a.h"
#include "slave_i2c.h"

CM_I2C_TypeDef fake_i2c1, fake_i2c2;
FakeGpio fake_gpio;
func_ptr_t fake_irqs[16] = {};
uint32_t fake_ms, fake_primask, fake_slave_initializations, fake_master_resets;
int fake_io_error, fake_receive_error, fake_slave_init_error;
uint8_t fake_address, fake_register;
uint16_t fake_registers[128][256] = {};
uint32_t fake_master_writes;
int fake_pins[32] = {};
uint32_t fake_system_resets;
SystemInfo_t systemInfo = {};

void
HAL::HAL_Update() {}

void
App_Update() {}

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
    systemInfo.radioInfo.radio_change_flag = 1;
    write_request(frame(NM_PANEL_INFO3_ID));
    assert(txBufferAvailable() == NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    assert(systemInfo.radioInfo.radio_change_flag == 1);
    read_response(5);
    assert(slave_i2c_diagnostics.partial_reads == 1);
    assert(txBufferAvailable() == NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    write_request(frame(NM_PANEL_SET1_ID, std::vector<uint8_t>(88, 0), NM_MSG_SET_TYPE));
    assert(slave_i2c_diagnostics.response_busy == 1);
    auto response = read_response(NM_PROTOCOL_PINFO3_MSG_PACK_LEN);
    assert(response[4] == NM_PANEL_INFO3_ID && crc_valid(response));
    assert(systemInfo.radioInfo.radio_change_flag == 0);
    assert(txBufferAvailable() == 0);

    auto bad = frame(NM_PANEL_SET1_ID, std::vector<uint8_t>(88, 0), NM_MSG_SET_TYPE);
    bad.back() ^= 1;
    const auto set_count = systemInfo.messageDecode.Set1_count;
    write_request(bad);
    assert(txBufferAvailable() == 0 && systemInfo.messageDecode.Set1_count == set_count);

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
    assert(systemInfo.powerMonitor.reset_flag == 0);
    read_response(4);
    assert(systemInfo.powerMonitor.reset_flag == 0);
    read_response(NM_PROTOCOL_RST_RESP_MSG_PACK_LEN);
    assert(systemInfo.powerMonitor.reset_flag == 1);
    systemInfo.powerMonitor.reset_flag = 0;

    // 关机确认方式一：主控完整读到含关机标志的 INFO2；短读不算确认。
    systemInfo.powerMonitor.poweroff_flag = 1;
    write_request(frame(NM_PANEL_INFO2_ID));
    read_response(4);
    assert(!systemInfo.powerMonitor.ShutdownEnsure && systemInfo.powerMonitor.poweroff_flag == 1);
    read_response(NM_PROTOCOL_PINFO2_MSG_PACK_LEN);
    assert(systemInfo.powerMonitor.ShutdownEnsure && systemInfo.powerMonitor.poweroff_flag == 0);
    systemInfo.powerMonitor.ShutdownEnsure = false;
    // 方式二：主控发送 0x05=1，ACK 完整读出后确认。
    systemInfo.powerMonitor.poweroff_flag = 1;
    write_request(frame(NM_PANEL_SHUTDOWN_CONFIRM_ID, {1}, NM_MSG_SET_TYPE));
    read_response(4);
    assert(!systemInfo.powerMonitor.ShutdownEnsure);
    read_response(NM_PROTOCOL_SET_MSG_PACK_LEN);
    assert(systemInfo.powerMonitor.ShutdownEnsure && systemInfo.powerMonitor.poweroff_flag == 0);
    systemInfo.powerMonitor.ShutdownEnsure = false;
    systemInfo.powerMonitor.LinuxPowerOff = false;

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
    memset(systemInfo.hardware_version, 'H', sizeof(systemInfo.hardware_version));
    memset(systemInfo.software_version, 'S', sizeof(systemInfo.software_version));
    assert(message_decode(&parse, response, 4) == 0);
    assert(message_decode(&parse, response, sizeof(response)) == NM_PROTOCOL_INFO1_MSG_PACK_LEN);
    assert(response[20] == 'H' && response[27] == 'H' && response[28] == 'S' && response[37] == 'S');
    assert(message_decode(nullptr, response, sizeof(response)) == 0);

    systemInfo.radioInfo.radio_change_flag = 1;
    bytes = frame(NM_PANEL_INFO3_ID);
    parse.buffer = bytes.data();
    parse.length = bytes.size();
    assert(message_decode(&parse, response, sizeof(response)) > 0);
    ++systemInfo.radioInfo.radio_channel;
    message_response_complete(response, sizeof(response));
    assert(systemInfo.radioInfo.radio_change_flag == 1);
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

static void
test_device_failures() {
    assert(Wire.begin());
    assert(bq40z50Begin(&Wire));
    assert(mp2762aBegin(&Wire));
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
    assert(!Wire.beginTransmission(0x0b));
    assert(Wire.endTransmission() != 0);
    fake_io_error = 0;
    assert(Wire.beginTransmission(0x0b) && Wire.endTransmission() == 0);

    systemInfo.online_device.mp2762 = false;
    battery.sample_valid = true;
    battery.Temp_f = 50.0f;
    for (int i = 0; i < 20; ++i) {
        HAL::Charger_Protect(battery);
    }
    assert(battery.ChargerDisable && digitalRead(CHARGER_ENABLE_PIN) == LOW);
    battery.Temp_f = 25.0f;
    battery.ChargerPlugCount = 3;
    battery.ChargerDetect = 1;
    HAL::Charger_Protect(battery);
    assert(!battery.ChargerDisable && digitalRead(CHARGER_ENABLE_PIN) == HIGH);
    // 电量计在线但样本失效：禁止充电。
    systemInfo.online_device.bq40z50 = true;
    battery.sample_valid = false;
    HAL::Charger_Protect(battery);
    assert(battery.ChargerDisable && digitalRead(CHARGER_ENABLE_PIN) == LOW);
    // 电量计从未识别：交由充电芯片保护，恢复充电。
    systemInfo.online_device.bq40z50 = false;
    HAL::Charger_Protect(battery);
    assert(!battery.ChargerDisable && digitalRead(CHARGER_ENABLE_PIN) == HIGH);
    systemInfo.online_device.mp2762 = true;
    assert(mp2762setChargerEnabled(false, &config));
    assert(mp2762setChargerEnabled(true, &config) && (config & 16));
}

// 两个主板版本：V1.3 无 MP2762A（GPIO 控制充电与 USB 通路），V1.5 经 MP2762A。
static void
test_charger_variants() {
    systemInfo = {};
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

    systemInfo.online_device.mp2762 = true;
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
    systemInfo = {};
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

// 外部供电关机后驻留：不复位、不自动开机；长按电源键 2 s 原地开机，状态与冷启动一致。
static void
test_resident_off() {
    systemInfo = {};
    systemInfo.powerMonitor.panel_power_on = true;
    systemInfo.powerMonitor.Force_ShutDown = true;
    systemInfo.online_device.eg25_board = true;
    systemInfo.recordInfo.record_status = On_Off_Status_ON;
    fake_pins[POWER_KEY_PIN] = HIGH; // 关机时按键仍按住
    fake_ms = 1000;
    const uint32_t resets = fake_system_resets;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.powerMonitor.panel_power_on && fake_pins[POWER_CONTROL_PIN] == LOW);
    assert(systemInfo.powerMonitor.ShutdownComplete && !systemInfo.powerMonitor.Force_ShutDown);
    assert(fake_pins[POWER_LED_PIN] == LOW && fake_pins[FUNCTION_LED_PIN] == LOW);
    fake_ms += 1000;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.powerMonitor.panel_power_on);

    // 驻留期间的强制关机请求不能带到下次开机。
    systemInfo.powerMonitor.Force_ShutDown = true;
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms += 5000;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.powerMonitor.panel_power_on);
    fake_pins[POWER_KEY_PIN] = HIGH;
    HAL::Power_PowerOffMonitor();
    fake_ms += 1999;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.powerMonitor.panel_power_on);
    fake_ms += 1;
    HAL::Power_PowerOffMonitor();
    assert(systemInfo.powerMonitor.panel_power_on && !systemInfo.powerMonitor.ShutdownComplete);
    assert(fake_pins[POWER_CONTROL_PIN] == HIGH && fake_pins[POWER_LED_PIN] == HIGH);
    assert(!systemInfo.powerMonitor.Force_ShutDown && !systemInfo.online_device.eg25_board);
    assert(systemInfo.recordInfo.record_status == On_Off_Status_OFF);
    assert(fake_system_resets == resets);

    // 开机后电源策略重新可用：再次强制关机仍进入驻留。
    fake_pins[POWER_KEY_PIN] = LOW;
    fake_ms += 100;
    HAL::Power_PowerOffMonitor();
    assert(systemInfo.powerMonitor.panel_power_on);
    systemInfo.powerMonitor.Force_ShutDown = true;
    HAL::Power_PowerOffMonitor();
    assert(!systemInfo.powerMonitor.panel_power_on && systemInfo.powerMonitor.ShutdownComplete);
    assert(fake_system_resets == resets);
}

int
main() {
    test_parser_boundaries();
    test_slave_transactions();
    test_decoder_boundary();
    test_device_failures();
    test_charger_variants();
    test_hold_and_blink();
    test_power_policy();
    test_resident_off();
    puts("protocol, transport, devices and power regressions passed");
}
