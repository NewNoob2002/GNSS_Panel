#pragma once

/**
 * HAL 电源状态 -> 共享应用。HAL 是电源状态的唯一判定者：开机（Power 键原始电平 2 s）、
 * 关机请求与断电后的待机驻留；这里只在状态变化时通知 SystemService。
 */
class PowerBridge {
  public:
    void update();

  private:
    bool powered_on_ = false;
    bool shutdown_reported_ = false;
    bool standby_reported_ = false;
};
