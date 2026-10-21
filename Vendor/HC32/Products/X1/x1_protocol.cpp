// X1 面板协议表（源自 X1_Panel_dev）：0x01 信息、0x02 控制状态、0x03 录制设置、0x04 复位、0x05 关机。
// 帧校验与响应编码由 Shared/Comm/panel_protocol.cpp 完成；动作在响应被完整读出后执行。
// 0x01/0x02 与带屏产品同一布局（PanelInfoPayload / PanelStatusPayload），0x03~0x05 为单字节标志。
#include <cstring>
#include "SystemState.h"
#include "mcu_define.h"
#include "panel_protocol.h"
#include "platform_debug.h"
#include "product_config.h"

#define NM_PANNEL_INFO_ID  0x01
#define NM_PANNEL_CTRL_ID  0x02
#define NM_PANNEL_HOST_ID  0x03
#define NM_PANNEL_RST_ID   0x04
#define NM_PANNEL_POWER_ID 0x05

// X1 主控只读软件版本的前 8 字节（沿用 X1_Panel_dev）。
static constexpr size_t VERSION_FIELD_LENGTH = 8U;

static struct {
    uint8_t poweroffFlag;
    uint32_t recordRevision;
} ctrl_snapshot;

static uint8_t pending_reset;
static uint8_t pending_power_off;

static bool
validate_flag(const uint8_t* payload, uint16_t payloadLength) {
    return panel_payload<PanelFlagRequest>(payload, payloadLength).value <= 1U;
}

static void
fill_info_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info1_count++;
    const BatteryInfo_t& battery = systemInfo.power.batteryInfo;
    PanelInfoPayload info{};
    constexpr size_t hardwareVersionLength =
        sizeof(HW_VERSION) - 1U < VERSION_FIELD_LENGTH ? sizeof(HW_VERSION) - 1U : VERSION_FIELD_LENGTH;
    std::memcpy(info.hardware_version, HW_VERSION, hardwareVersionLength);
    std::memcpy(info.software_version, systemInfo.board.software_version,
                strnlen(systemInfo.board.software_version, VERSION_FIELD_LENGTH));
    info.battery_percent = battery.Percent;
    info.battery_temperature = battery.Temp;
    info.battery_voltage = battery.Voltage;
    info.status = 1;
    panel_write(msg, info);
}

static void
fill_ctrl_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info2_count++;
    PanelStatusPayload status{};
    status.reset = systemInfo.power.reset_flag;
    status.poweroff = systemInfo.power.poweroff_flag;
    status.record_status = State::Effective_Record().record_status;
    status.record_op = systemInfo.request.record_op;
    status.charging = systemInfo.power.batteryInfo.chargeStatus != notCharge;
    panel_write(msg, status);
    ctrl_snapshot.poweroffFlag = status.poweroff;
    ctrl_snapshot.recordRevision = State::Record_Revision();
}

static void
ctrl_read_complete() {
    // X1_Panel_dev 语义：主控读到关机标志即视为确认，进入 8 s 断电延时。
    if (ctrl_snapshot.poweroffFlag) {
        State::Host_ShutdownAcknowledged();
        CORE_DEBUG_PRINTF("Shutdown Sync");
    }
    // 读出期间按键又切换了录制时 revision 已变，保留操作标志，下次查询再上报。
    State::Confirm_Record(ctrl_snapshot.recordRevision, false);
}

static void
handle_host_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    State::Diagnostics().messages.Set1_count++;
    // 面板本地切换录制尚未同步给主控时，显示仍以面板请求为准（State::Effective_Record）。
    const bool recording = panel_request<PanelFlagRequest>(parse).value != 0U;
    State::Host().record.record_status = recording ? On_Off_Status_ON : On_Off_Status_OFF;
    panel_write_success_payload(msg);
}

static void
handle_reset_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_reset = panel_request<PanelFlagRequest>(parse).value;
    CORE_DEBUG_PRINTF("Reset Panel request: %u", pending_reset);
    panel_write_success_payload(msg);
}

static void
reset_read_complete() {
    State::Host_ResetRequested(pending_reset);
}

static void
handle_power_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_power_off = panel_request<PanelFlagRequest>(parse).value;
    panel_write_success_payload(msg);
}

static void
power_read_complete() {
    if (pending_power_off) {
        State::Host_ShutdownRequested();
    }
}

static const PanelPacket packetTable[] = {
    {NM_PANNEL_INFO_ID, 0, sizeof(PanelInfoPayload), NM_MSG_QUERY_RES_TYPE, fill_info_response, nullptr, nullptr},
    {NM_PANNEL_CTRL_ID, 0, sizeof(PanelStatusPayload), NM_MSG_QUERY_RES_TYPE, fill_ctrl_response, nullptr,
     ctrl_read_complete},
    {NM_PANNEL_HOST_ID, sizeof(PanelFlagRequest), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_host_request,
     validate_flag, nullptr},
    {NM_PANNEL_RST_ID, sizeof(PanelFlagRequest), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_reset_request,
     validate_flag, reset_read_complete},
    {NM_PANNEL_POWER_ID, sizeof(PanelFlagRequest), sizeof(PanelSetAck), NM_MSG_SET_RES_TYPE, handle_power_request,
     validate_flag, power_read_complete},
};

const PanelPacket*
panel_protocol_packets(size_t* count) {
    *count = sizeof(packetTable) / sizeof(packetTable[0]);
    return packetTable;
}
