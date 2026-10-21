#pragma once

/**
 * HAL 电源状态 -> 共享应用。Startup 完整长按经 HAL::Power_RequestOn() 确认开机，
 * HAL 负责软件复位直接开机、关机时序与断电后驻留；这里只在状态变化时幂等通知 SystemService。
 */
class PowerBridge {
  public:
    void update();

  private:
    bool powered_on_ = false;
    bool shutdown_reported_ = false;
    bool standby_reported_ = false;
};
