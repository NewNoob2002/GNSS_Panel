#pragma once
#include <cstdint>
#include "Def/HAL_Battery.h"
#include "Def/HAL_Board.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Work.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"

/** @brief 主控协议与细设备间的采样和命令映射；不注册设备、不发布数据。 */
class BusinessDevice {
  public:
    /** @brief 返回各领域的有效值，不消费协议确认。 */
    hal::BatteryState battery();
    hal::GNSSState gnss();
    hal::WorkState work();
    hal::RecorderState recorder();
    hal::WifiState wifi();
    hal::BoardState board(std::uint32_t now);
    /** @brief 检查命令范围后经 State::Request_* 提交；协议码只在此转换。 */
    bool execute(const hal::WorkCommand& command);
    bool execute(const hal::RecorderCommand& command);
    /** @brief 提交 Wi-Fi 开关并开始平台内部超时计时。 */
    bool request_wifi(bool on);
    /** @brief 维护 Wi-Fi 回报与超时；不向 App 推送状态。 */
    void update(std::uint32_t now);

  private:
    static constexpr std::uint32_t kWifiTimeoutMs = 15000;
    bool wifi_requested_ = false;
    std::uint32_t wifi_request_ms_ = 0;
};
