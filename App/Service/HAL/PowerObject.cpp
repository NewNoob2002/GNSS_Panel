#include "PowerObject.h"

namespace hal {
int PowerObject::onRead(void* buffer, size_t size) { return output(buffer, size, power_report()); }
int PowerObject::onWrite(const void* buffer, size_t size) {
    if (size != sizeof(PowerCommand)) return RES_PARAM_ERROR;
    PowerCommand action;
    std::memcpy(&action, buffer, sizeof(action));
    if (action != PowerCommand::Off && action != PowerCommand::Reboot) return RES_PARAM_ERROR;
    return execute_power(action) ? RES_OK : RES_UNKNOWN;
}
int PowerObject::onIoctl(IO_Cmd_t cmd, void*) {
    if (command(cmd) != kRequestPowerOn) return RES_UNSUPPORT;
    return request_power_on() ? RES_OK : RES_UNKNOWN;
}
}
