#pragma once

#include "mcu_define.h"

namespace HAL {

/**
 * 充电控制。主板有两个版本：V1.5 带 MP2762A（I2C 配置与读取状态），V1.3 没有该芯片，
 * 由 GPIO 控制充电使能/电流并检测充电器插入。版本由 I2C 扫描是否找到 MP2762A 推断
 * （systemInfo.online_device.mp2762），见 HAL::Charger_Get()。
 *
 * 只在主循环调用；电池温度保护等策略在 HAL::Charger_Protect()，与芯片无关。
 */
class Charger {
  public:
    virtual const char* name() const = 0;
    /** 上电后配置一次（在 I2C 扫描之后）。 */
    virtual void begin(BatteryInfo_t& battery) = 0;

    /** 进入等待开机（冷启动或关机驻留）。 */
    virtual void
    enter_standby() {}

    /** 开机后恢复输出。 */
    virtual void
    power_on() {}

    /** 周期刷新充电状态（chargeStatus）。 */
    virtual void update(BatteryInfo_t& battery) = 0;

    /** 允许/禁止充电；状态未变时不重复操作芯片，失败时保持原状态。 */
    void set_enabled(bool enabled, BatteryInfo_t& battery);

  protected:
    ~Charger() = default;
    /** 实际切换充电使能，成功返回 true。 */
    virtual bool apply_enabled(bool enabled, BatteryInfo_t& battery) = 0;
};

/** 当前主板的充电控制实现。 */
Charger& Charger_Get();

/** 电池温度保护：过温或电量计在线但样本无效时禁止充电，否则允许。 */
void Charger_Protect(BatteryInfo_t& battery);

/** 读取 BQ40Z50 电量计；任一寄存器读取失败或数值无效时 sample_valid = false，其余字段保持。 */
void Battery_Sample(BatteryInfo_t& battery);

} // namespace HAL
