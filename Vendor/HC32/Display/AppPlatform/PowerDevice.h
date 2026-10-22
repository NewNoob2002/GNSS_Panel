#pragma once
#include "SystemBackend.h"

/**
 * HAL 电源状态 -> 共享应用。Startup 完整长按经 HAL::Power_RequestOn() 确认开机，
 * HAL 负责软件复位直接开机、关机时序与断电后驻留；这里只在状态变化时幂等通知 SystemService。
 */
class PowerDevice : public system_backend::PowerObject {
  public:
    void update();
    /** 当前 HAL 电源报告，不改变电源策略。 */
    system_backend::PowerReport power_report() override;
    /** Startup 长按完成后转交原 HAL 请求。 */
    bool request_power_on() override;
    /** 关机仍由原 HAL 时序执行。 */
    bool execute_power(DataProc::PowerAction action) override;

  private:
    bool powered_on_ = false;
    bool shutdown_reported_ = false;
    bool standby_reported_ = false;
};
