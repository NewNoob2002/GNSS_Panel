#pragma once
#include "HAL_Def.h"
#include "Def/HAL_Power.h"

namespace hal {
/** @brief 电源设备的精确尺寸解码；实现只采样或执行平台电源策略。 */
class PowerObject : public DeviceObject {
public:
    PowerObject() : DeviceObject(names::Power) {}
    virtual PowerState power_report() = 0;
    virtual bool request_power_on() = 0;
    virtual bool execute_power(PowerCommand action) = 0;
private:
    int onInit() override { return RES_OK; }
    int onRead(void*, size_t) override;
    int onWrite(const void*, size_t) override;
    int onIoctl(IO_Cmd_t, void*) override;
};
}
