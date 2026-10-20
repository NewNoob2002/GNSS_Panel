#pragma once

#include "mcu_define.h"

/**
 * 系统状态的写入口。所有模块经 systemInfo（const）读取；写入按域限定：
 *
 * | 域              | 写入者                         | 入口                                   |
 * |-----------------|--------------------------------|----------------------------------------|
 * | board           | HAL（I2C 扫描）                | State::Board()                         |
 * | power           | HAL 电源策略                   | State::Power()                         |
 * | host            | 面板协议（主控上报）           | State::Host()                          |
 * | request         | 界面、按键、电源策略发起的请求 | State::Request_*()，协议 Confirm_*()   |
 * | diag            | 通信层                         | State::Diagnostics()                   |
 *
 * 其它模块对电源的请求（主控确认/请求关机、复位、外部供电、强制关机）经 State::Host_* /
 * Request_ForcedShutdown()，由 HAL 电源策略处理。全部在主循环调用（I2C 中断只收发字节）。
 *
 * 面板请求与主控上报分开保存：请求待确认期间界面显示请求值（Effective_*()），主控上报仍照常
 * 更新；协议填充响应时记下 revision，主控完整读出后若请求未再变化则确认，并把请求值视为主控
 * 已采用的值写入 host，直到主控下一次上报。
 */
namespace State {

/* ---- 各域写入（只由该域的写入者调用） ---- */
BoardInfo_t& Board();
Power_Monitor_t& Power();
HostReport_t& Host();
Diagnostics_t& Diagnostics();

/** 上电初始化：全部清零。 */
void Reset();
/** 关机驻留后开机：主控已断电重启，清除主控上报与面板请求；板卡、电池与诊断计数保留。 */
void Reset_HostData();

/* ---- 显示用的有效值：请求待确认时为请求值，否则为主控上报 ---- */
WorkConfig_t Effective_Work();
RecordInfo_t Effective_Record();

/* ---- 面板请求 ---- */
void Request_Work(const WorkConfig_t& work);
/** 带屏产品录制启停与设置（type/interval 为主控编码）。 */
void Request_Record(On_Off_Status_t status, uint8_t type, uint8_t interval);
/** X1 功能键：切换录制，不改设置。 */
void Request_RecordToggle();
/** 电源策略：低电量停止录制（未在录制时不发请求）。 */
void Request_RecordStop();
void Request_Wifi(bool on);
/** Function 键长按强制关机。 */
void Request_ForcedShutdown();

/* ---- 面板协议：确认请求（revision 取自填充响应时） ---- */
uint32_t Work_Revision();
void Confirm_Work(uint32_t revision);
uint32_t Record_Revision();
/** settings：该响应带有录制设置（带屏 INFO4），确认时同时清除设置待确认。 */
void Confirm_Record(uint32_t revision, bool settings);
uint32_t Wifi_Revision();
void Confirm_Wifi(uint32_t revision);
/** 主控在上报中置位了变更标志：把刚上报的值作为待确认请求回传（保持旧协议行为）。 */
void Echo_HostWork();
void Echo_HostRecord();

/* ---- 面板协议：主控对电源的通知 ---- */
/** 主控完整读出含关机标志的状态帧：确认关机，进入断电延时。 */
void Host_ShutdownAcknowledged();
/** 主控发送关机确认（带屏 0x05）：同上；面板未在关机时视为主控发起的关机。 */
void Host_ShutdownConfirmed();
/** 主控请求关机。 */
void Host_ShutdownRequested();
void Host_ResetRequested(uint8_t reset);
void Host_ExternalPower(uint8_t external_power);

} // namespace State
