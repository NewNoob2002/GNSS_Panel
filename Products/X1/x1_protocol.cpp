// X1 面板协议表（源自 X1_Panel_dev）：0x01 信息、0x02 控制状态、0x03 录制设置、0x04 复位、0x05 关机。
// 帧校验与响应编码由 Shared/Comm/panel_protocol.cpp 完成；动作在响应被完整读出后执行。
#include <cstring>
#include "SystemState.h"
#include "core_debug.h"
#include "mcu_define.h"
#include "panel_protocol.h"
#include "product_config.h"

#define NM_PANNEL_INFO_ID         0x01
#define NM_PROTOCOL_PINFO_MSG_LEN 36
#define NM_PROTOCOL_PHV_OFFSET    (NM_PROTOCOL_HEADER_LEN + 0)
#define NM_PROTOCOL_PFV_OFFSET    (NM_PROTOCOL_HEADER_LEN + 8)
#define NM_PROTOCOL_PBL_OFFSET    (NM_PROTOCOL_HEADER_LEN + 18)
#define NM_PROTOCOL_PBT_OFFSET    (NM_PROTOCOL_HEADER_LEN + 20)
#define NM_PROTOCOL_PBV_OFFSET    (NM_PROTOCOL_HEADER_LEN + 22)
#define NM_PROTOCOL_PSS_OFFSET    (NM_PROTOCOL_HEADER_LEN + 26)

#define NM_PANNEL_CTRL_ID         0x02
#define NM_PROTOCOL_PCTRL_MSG_LEN 8
#define NM_PROTOCOL_PRE_OFFSET    (NM_PROTOCOL_HEADER_LEN + 0)
#define NM_PROTOCOL_PPC_OFFSET    (NM_PROTOCOL_HEADER_LEN + 1)
#define NM_PROTOCOL_PRC_OFFSET    (NM_PROTOCOL_HEADER_LEN + 2)
#define NM_PROTOCOL_PRO_OFFSET    (NM_PROTOCOL_HEADER_LEN + 3)
#define NM_PROTOCOL_PEP_OFFSET    (NM_PROTOCOL_HEADER_LEN + 4)

#define NM_PANNEL_HOST_ID         0x03
#define NM_PANNEL_RST_ID          0x04
#define NM_PANNEL_POWER_ID        0x05

static constexpr size_t VERSION_FIELD_LENGTH = 8U;

static struct {
    uint8_t poweroffFlag;
    uint32_t recordRevision;
} ctrl_snapshot;

static uint8_t pending_reset;
static uint8_t pending_power_off;

static bool
validate_flag(const uint8_t* payload, uint16_t payloadLength) {
    (void)payloadLength;
    return payload[0] <= 1U;
}

static void
fill_info_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info1_count++;
    constexpr size_t hardwareVersionLength =
        sizeof(HW_VERSION) - 1U < VERSION_FIELD_LENGTH ? sizeof(HW_VERSION) - 1U : VERSION_FIELD_LENGTH;
    memcpy(&msg[NM_PROTOCOL_PHV_OFFSET], HW_VERSION, hardwareVersionLength);
    memcpy(&msg[NM_PROTOCOL_PFV_OFFSET], systemInfo.board.software_version,
           strnlen(systemInfo.board.software_version, VERSION_FIELD_LENGTH));
    memcpy(&msg[NM_PROTOCOL_PBL_OFFSET], &systemInfo.power.batteryInfo.Percent, 2);
    memcpy(&msg[NM_PROTOCOL_PBT_OFFSET], &systemInfo.power.batteryInfo.Temp, 2);
    memcpy(&msg[NM_PROTOCOL_PBV_OFFSET], &systemInfo.power.batteryInfo.Voltage, 2);
    msg[NM_PROTOCOL_PSS_OFFSET] = 1;
}

static void
fill_ctrl_response(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    (void)parse;
    State::Diagnostics().messages.Info2_count++;
    msg[NM_PROTOCOL_PRE_OFFSET] = systemInfo.power.reset_flag;
    msg[NM_PROTOCOL_PPC_OFFSET] = systemInfo.power.poweroff_flag;
    msg[NM_PROTOCOL_PRC_OFFSET] = State::Effective_Record().record_status;
    msg[NM_PROTOCOL_PRO_OFFSET] = systemInfo.request.record_op;
    msg[NM_PROTOCOL_PEP_OFFSET] = systemInfo.power.batteryInfo.chargeStatus != notCharge;
    ctrl_snapshot.poweroffFlag = systemInfo.power.poweroff_flag;
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
    State::Host().record.record_status = parse->buffer[NM_PROTOCOL_HEADER_LEN] ? On_Off_Status_ON : On_Off_Status_OFF;
    panel_write_success_payload(msg);
}

static void
handle_reset_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_reset = parse->buffer[NM_PROTOCOL_HEADER_LEN];
    CORE_DEBUG_PRINTF("Reset Panel request: %u", pending_reset);
    panel_write_success_payload(msg);
}

static void
reset_read_complete() {
    State::Host_ResetRequested(pending_reset);
}

static void
handle_power_request(const SEMP_PARSE_STATE* parse, uint8_t* msg) {
    pending_power_off = parse->buffer[NM_PROTOCOL_HEADER_LEN];
    panel_write_success_payload(msg);
}

static void
power_read_complete() {
    if (pending_power_off) {
        State::Host_ShutdownRequested();
    }
}

static const PanelPacket packetTable[] = {
    {NM_PANNEL_INFO_ID, 0, NM_PROTOCOL_PINFO_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_info_response, nullptr, nullptr},
    {NM_PANNEL_CTRL_ID, 0, NM_PROTOCOL_PCTRL_MSG_LEN, NM_MSG_QUERY_RES_TYPE, fill_ctrl_response, nullptr,
     ctrl_read_complete},
    {NM_PANNEL_HOST_ID, 1, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_host_request, validate_flag, nullptr},
    {NM_PANNEL_RST_ID, 1, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_reset_request, validate_flag,
     reset_read_complete},
    {NM_PANNEL_POWER_ID, 1, NM_PROTOCOL_SET_MSG_LEN, NM_MSG_SET_RES_TYPE, handle_power_request, validate_flag,
     power_read_complete},
};

const PanelPacket*
panel_protocol_packets(size_t* count) {
    *count = sizeof(packetTable) / sizeof(packetTable[0]);
    return packetTable;
}
