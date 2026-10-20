#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct online_device_t {
    bool bq40z50;
    bool mp2762;
} online_device_t;

typedef enum Charger_Status_t {
    notCharge = 0,
    normalCharge,
    fastCharge,
} Charger_Status_t;

typedef enum On_Off_Status_t {
    On_Off_Status_OFF = 0,
    On_Off_Status_ON = 1,
} On_Off_Status_t;

typedef enum RadioMode_t {
    radio_mode_tx = 0,
    radio_mode_rx = 1,
    radio_mode_bridge = 2,
} RadioMode_t;

typedef enum Protocol_index_t {
    TRIMTALK,
    TRIMMK3,
    TT450S,
    TRANSEOT,
    SOUTH,
    HUACE,
    SATEL,
    CCS,
    PROTOCOL_MAX
} Protocol_index_t;

typedef enum Channel_index_t {
    Channel1,
    Channel2,
    Channel3,
    Channel4,
    Channel5,
    Channel6,
    Channel7,
    Channel8,
    Channel9,
    ChannelMax
} Channel_index_t;

typedef enum Redcord_Type_t { Redcord_Type_XYZ, Redcord_Type_Rinex, Redcord_Type_MAX } Redcord_Type_t;

typedef enum Redcord_Interval_t {
    Redcord_Interval_infinite,
    Redcord_Interval_15min,
    Redcord_Interval_60min,
    Redcord_Interval_120min,
    Redcord_Interval_240min,
    Redcord_Interval_24hour,
    Redcord_Interval_MAX
} Redcord_Interval_t;

typedef enum WorkMode_t { rover_mode = 0, base_mode, single_mode, autobase_mode } WorkMode_t;

typedef enum PositionStatus_t {
    position_none = 0,
    position_single = 1,
    position_fix = 4,
    position_float = 5,
} PositionStatus_t;

typedef struct BatteryInfo_t {
    float Percent_f;
    float Voltage_f;
    float Temp_f;
    float fOtsTemp;
    uint16_t Actual_Percent;
    uint16_t Processed_Percent;
    uint16_t Percent;
    uint16_t Temp;
    uint16_t Voltage;
    uint8_t isOverTemp;
    uint8_t ChargerOverTempCount;
    uint8_t ChargerDisable;
    uint8_t ChargerDetect;
    uint8_t ChargerPlugCount;
    uint8_t ChargerCurrent;
    uint8_t mp2762_cfg0;
    bool sample_valid;
    uint32_t last_sample_ms;
    Charger_Status_t chargeStatus;

} BatteryInfo_t, *pBatteryInfo_t;

typedef struct Power_Monitor_t {
    BatteryInfo_t batteryInfo;
    uint32_t WatchDogLastFeedTime;
    uint32_t BatteryLastHandleTime;

    bool LinuxPowerOff;
    bool LowBatteryPowerOff;
    bool ShutdownReq;
    bool ShutdownGoing;
    bool ShutdownEnsure;
    bool ShutdownComplete;

    bool Force_ShutDown;

    uint8_t reset_flag;
    uint8_t poweroff_flag;
    uint8_t ExternalPower;
    uint8_t ExternalPowerChange;
    bool panel_power_on;
    uint32_t pannel_power_on_time;
} Power_Monitor_t, *pPower_Monitor_t;

typedef struct PositionInfo_t {
    PositionStatus_t coordinate_status; // 0- none, 1-single, 4-fixed, 5- float, 7-fixedcoordinate
    uint8_t satellite_number_used;
    uint8_t satellite_number_track;
    double coordinate_lon;
    double coordinate_lat;
    double coordinate_alt;
} PositionInfo_t;

/* 主控上报的录制状态 */
typedef struct RecordInfo_t {
    On_Off_Status_t record_status; // 0-off, 1-on
    float record_leftspace;
    uint8_t record_type;     // 1-xyz 2-Rinex3.02
    uint8_t record_interval; // 0xff-不限 0x00-15min, 0x01-60min, 0x02-120min, 0x04-240min, 0x18- 24hour
    uint8_t record_name[16];
} RecordInfo_t;

/* 工作模式与电台/网络基站配置：主控上报的当前值，也是面板请求修改的内容 */
typedef struct WorkConfig_t {
    WorkMode_t work_mode;
    On_Off_Status_t radio_status;
    RadioMode_t radio_mode;
    uint8_t radio_protocol; // 主控协议编码
    uint8_t radio_channel;
    On_Off_Status_t gprs_status;
    On_Off_Status_t NtripServer_status;
} WorkConfig_t;

/* 主控上报的网络基站/客户端链路信息（面板不修改） */
typedef struct NtripInfo_t {
    On_Off_Status_t NtripClient_status;
    uint8_t NtripServer_IP[4];
    uint8_t NtripClient_IP[4];
    uint8_t NtripServer_Mountpoint[32];
    uint8_t NtripClient_Mountpoint[32];
} NtripInfo_t;

/* 主控上报的 Wi-Fi 状态 */
typedef struct WifiInfo_t {
    uint8_t wifi_mode;
    On_Off_Status_t wifi_status;
    char wifi_ssid[16];
    uint8_t wifi_ip[4];
} WifiInfo_t;

typedef struct StarMapInfo_t {
    uint32_t sequence; // 每收到一帧星图数据加 1；0 表示从未收到
    uint8_t numberGPS;
    uint8_t numberGLONASS;
    uint8_t numberGALILEO;
    uint8_t numberBDS;
    uint8_t numberSBAS;
    uint8_t numberQZSS;
    uint8_t numberIRNSS;
} StarMapInfo_t;

typedef struct MessageDecode_Count_t {
    uint32_t Info1_count;
    uint32_t Info2_count;
    uint32_t Info3_count;
    uint32_t Info4_count;
    uint32_t InfoWifi_count;

    uint32_t Set1_count;
    uint32_t Set3_count;
    uint32_t Set6_count;
    uint32_t SetWifi_count;
    uint32_t SetSatellite_count;

    uint32_t error_count;
} MessageDecode_Count_t;

/* ---- 系统状态：按写入者分域（见 Platform/State/SystemState.h） ---- */

/* 板卡与器件：HAL（I2C 扫描）写入 */
typedef struct BoardInfo_t {
    char software_version[16];
    char hardware_version[16];
    online_device_t online_device;
} BoardInfo_t;

/* 主控上报：面板协议写入。不含面板尚未被主控确认的请求，界面显示用 State::Effective_*() */
typedef struct HostReport_t {
    bool online; // 收到过主控的有效帧
    PositionInfo_t position;
    StarMapInfo_t starMap;
    WifiInfo_t wifi;
    RecordInfo_t record;
    WorkConfig_t work;
    NtripInfo_t ntrip;
} HostReport_t;

/* 面板请求：经 State::Request_*() 写入，面板协议上报，主控完整读出后经 State::Confirm_*() 确认 */
typedef struct PanelRequest_t {
    WorkConfig_t work;
    bool work_pending;
    uint32_t work_revision;

    On_Off_Status_t record_status;
    uint8_t record_type;
    uint8_t record_interval;
    bool record_op;               // 录制启停待主控确认
    bool record_settings_pending; // 带屏产品：录制设置（含启停）待主控确认
    uint32_t record_revision;

    On_Off_Status_t wifi_on; // 面板设定值，确认后保留
    bool wifi_pending;
    uint32_t wifi_revision;
} PanelRequest_t;

/* 通信诊断计数：通信层写入 */
typedef struct Diagnostics_t {
    MessageDecode_Count_t messages;
    uint16_t i2c_errors;
} Diagnostics_t;

typedef struct SystemInfo_t {
    BoardInfo_t board;
    Power_Monitor_t power; /* HAL 电源策略写入；其它模块经 HAL::Power_* / State::Host_* 请求 */
    HostReport_t host;
    PanelRequest_t request;
    Diagnostics_t diag;
} SystemInfo_t;

#ifdef __cplusplus
/* 全部模块只读；写入经 State:: 入口（Platform/State/SystemState.h） */
extern const SystemInfo_t& systemInfo;
#endif
