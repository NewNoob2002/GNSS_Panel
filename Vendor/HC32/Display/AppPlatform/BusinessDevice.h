#pragma once
#include <cstdint>
#include "SystemBackend.h"

#include "DataProc/DataProc_Def.h"

/**
 * systemInfo（主控上报与面板请求）<-> 共享应用的 Device DataNode 与业务命令。
 * 页面只使用选项索引；与主控协议编码的映射在这里完成。新增业务数据时扩展
 * snapshot()（读）与 execute()（写），并在 display_protocol.cpp 中收发对应字段。
 */
class BusinessDevice : public system_backend::BusinessObject {
  public:
    /** 设备 read 报告；协议码转换仍由本对象完成。 */
    system_backend::DeviceReport device_report() override;
    /** 设备 write 命令在当前 tick 执行。 */
    bool execute_device(const DataProc::DeviceRequest& request) override;
    /** ioctl Wi-Fi 请求沿用原超时与状态确认。 */
    bool request_wifi(bool on) override;
    /** 当前设备快照（Device 提供者首次发布前也用它）。不消费协议确认状态。 */
    DataProc::DeviceSnapshot snapshot(std::uint32_t now);
    /** 每轮主循环：同步 Wi-Fi 状态；周期检查快照，内容变化才发布。 */
    void update(std::uint32_t now);
    /** 执行已校验的业务请求：经 State::Request_*() 记录面板请求，由面板协议发给主控。 */
    bool execute(const DataProc::DeviceRequest& request, std::uint32_t now);
    /** 发出 Wi-Fi 开关设定；主控回报一致或超时前视为切换中。 */
    bool request_wifi(bool on, std::uint32_t now);

  private:
    void publish(std::uint32_t now);
    void forward_wifi(std::uint32_t now);

    static constexpr std::uint32_t kPeriodMs = 500;
    // 旧界面切换 Wi-Fi 后无限等待主控回报；此处超时后不再显示 "..."（设定值仍保留，主控稍后仍可应用）。
    static constexpr std::uint32_t kWifiTimeoutMs = 15000;

    std::uint32_t last_poll_ms_ = 0;
    bool polled_ = false;
    bool sent_ = false; // last_ 有效
    DataProc::DeviceSnapshot last_;
    bool wifi_requested_ = false;
    std::uint32_t wifi_request_ms_ = 0;
};
