#pragma once
#include <cstdint>
#include "DeviceObject.h"

namespace hal {
/** @brief 电源动作，与旧 Power write 的枚举布局一致。 */
enum class PowerCommand { Off, Reboot };
/** @brief 关机原因。 */
enum class PowerCause { User, Host, LowBattery, Forced };
/** @brief 平台电源状态；应用线程同步读取，状态与动作分别传递。 */
struct PowerState {
    bool powered_on = false, shutting_down = false, standby = false;
    PowerCause cause = PowerCause::User;
    bool operator==(const PowerState& v) const {
        return powered_on == v.powered_on && shutting_down == v.shutting_down && standby == v.standby && cause == v.cause;
    }
    bool operator!=(const PowerState& v) const { return !(*this == v); }
};
/** @brief 无载荷开机请求；RES_OK 表示即时确认，编码兼容旧 Power。 */
inline constexpr uint32_t kRequestPowerOn = DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_NONE, 0, 'P', 1);
} // namespace hal
