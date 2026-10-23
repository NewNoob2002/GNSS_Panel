#pragma once
#include <cstdint>
#include "SystemBackend.h"

/** @brief 主控协议与领域设备间的采样和命令映射；本对象不注册为设备、不发布数据。 */
class BusinessDevice {
  public:
    /** @brief 返回主控与面板有效请求的按值快照，不消费协议确认。 */
    DataProc::DeviceSnapshot snapshot(std::uint32_t now);
    /** @brief 检查命令范围后经 State::Request_* 提交。 */
    bool execute_device(const DataProc::DeviceRequest& request);
    /** @brief 提交 Wi-Fi 开关并开始平台内部超时计时。 */
    bool request_wifi(bool on);
    /** @brief 维护 Wi-Fi 回报与超时；不向 App 推送状态。 */
    void update(std::uint32_t now);

    /** @brief 当前 Wi-Fi 请求是否等待主控回报。 */
    bool
    wifi_pending() const {
        return wifi_requested_;
    }

  private:
    bool execute(const DataProc::DeviceRequest& request);
    static constexpr std::uint32_t kWifiTimeoutMs = 15000;
    bool wifi_requested_ = false;
    std::uint32_t wifi_request_ms_ = 0;
};
