#pragma once
#include <cstddef>
#include <cstdint>
#include "DataProc/DataProc_Def.h"
#include "DeviceObject.h"
#include "DeviceManager.h"
#include "HAL_Def.h"
#include "Def/HAL_Power.h"
#include "Def/HAL_Board.h"

/** @brief 迁移期平台入口；所有状态由领域节点经 DeviceManager 拉取。 */
namespace system_backend {
using Capabilities = hal::Capabilities;
using PowerReport = hal::PowerState;
/** @brief Power 精确尺寸解码层；实现只采样或执行 HAL 电源策略。 */
class PowerObject : public DeviceObject {
public:
    PowerObject() : DeviceObject(hal::names::Power) {}
    virtual PowerReport power_report() = 0;
    virtual bool request_power_on() = 0;
    virtual bool execute_power(DataProc::PowerAction action) = 0;
private:
    int onInit() override { return RES_OK; }
    int onRead(void*, size_t) override;
    int onWrite(const void*, size_t) override;
    int onIoctl(IO_Cmd_t, void*) override;
};
/** @brief 返回当前平台的九个细设备，初始化不改变硬件状态。 */
DeviceManager& devices();
/** @brief 页面迁移期保留的初始化便捷接口；均通过 HAL Init 精确尺寸 ioctl。 */
const char* initialization_name(unsigned step);
i18n::TextId initialization_title(unsigned step);
DataProc::InitializationStatus poll_initialization_step(unsigned step, uint32_t elapsed_ms, char* detail, size_t size);
}
