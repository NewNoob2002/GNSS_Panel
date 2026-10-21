#include "SystemBackend.h"
#include <cstdio>
#include <cstring>

namespace system_backend {
namespace {
/** 按值拷贝输出，避免缓冲区的对齐和严格别名假设。 */
template<class T> int output(void* buffer, size_t size, const T& value) {
    if (size != sizeof(T)) return ::DeviceObject::RES_PARAM_ERROR;
    std::memcpy(buffer, &value, sizeof(value));
    return ::DeviceObject::RES_OK;
}
/** 以编码字段重建命令，禁止读取联合体非活动成员。 */
uint32_t command(::DeviceObject::IO_Cmd_t cmd) {
    return DEVICE_OBJECT_IOCMD_DEF(cmd.ch.dir, cmd.ch.size, cmd.ch.type, cmd.ch.number);
}
/** 查找已初始化设备并发送同步命令。 */
int control(const char* name, uint32_t cmd, void* data, size_t size) {
    auto* dev = devices().getDevice(name);
    return dev ? dev->ioctl(cmd, data, size) : ::DeviceObject::RES_INIT_FAILED;
}
/** 从设备读取按值报告；失败保留默认值。 */
template<class T> T read_report(const char* name) {
    T result{};
    auto* dev = devices().getDevice(name);
    if (dev) (void)dev->read(&result, sizeof(result));
    return result;
}
}
int PowerObject::onRead(void* buffer, size_t size) { return output(buffer, size, power_report()); }
int PowerObject::onWrite(const void* buffer, size_t size) {
    if (size != sizeof(DataProc::PowerAction)) return RES_PARAM_ERROR;
    DataProc::PowerAction action;
    std::memcpy(&action, buffer, sizeof(action));
    if (action != DataProc::PowerAction::Off && action != DataProc::PowerAction::Reboot) return RES_PARAM_ERROR;
    return execute_power(action) ? RES_OK : RES_UNKNOWN;
}
int PowerObject::onIoctl(IO_Cmd_t cmd, void*) {
    if (command(cmd) != kRequestPowerOn) return RES_UNSUPPORT;
    return request_power_on() ? RES_OK : RES_UNKNOWN;
}
int BusinessObject::onRead(void* buffer, size_t size) { return output(buffer, size, device_report()); }
int BusinessObject::onWrite(const void* buffer, size_t size) {
    if (size != sizeof(DataProc::DeviceRequest)) return RES_PARAM_ERROR;
    DataProc::DeviceRequest request;
    std::memcpy(&request, buffer, sizeof(request));
    // 设备可被直接调用；范围在索引转换前再兜底一次。
    switch (request.command) {
    case DataProc::DeviceCommand::ApplyWork:
        if ((request.target != DataProc::WorkTarget::Rover && request.target != DataProc::WorkTarget::Base
             && request.target != DataProc::WorkTarget::Ntrip)
            || request.radio_protocol >= DataProc::kRadioProtocolCount
            || request.radio_channel >= DataProc::kRadioChannelCount
            || (request.target != DataProc::WorkTarget::Ntrip && request.radio_channel == 0)) return RES_PARAM_ERROR;
        break;
    case DataProc::DeviceCommand::StartRecord:
        if (request.record_type >= DataProc::kRecordTypeCount || request.record_interval >= DataProc::kRecordIntervalCount)
            return RES_PARAM_ERROR;
        break;
    case DataProc::DeviceCommand::StopRecord: break;
    default: return RES_PARAM_ERROR;
    }
    return execute_device(request) ? RES_OK : RES_UNKNOWN;
}
int BusinessObject::onIoctl(IO_Cmd_t cmd, void* data) {
    if (command(cmd) != kSetWifi) return RES_UNSUPPORT;
    bool on;
    std::memcpy(&on, data, sizeof(on));
    return request_wifi(on) ? RES_OK : RES_UNKNOWN;
}
int StatusObject::onRead(void* buffer, size_t size) { return output(buffer, size, status_snapshot()); }
int StatusObject::onIoctl(IO_Cmd_t cmd, void* data) {
    switch (command(cmd)) {
    case kCapabilities: return output(data, sizeof(Capabilities), capabilities());
    case kInitializationInfo: {
        InitializationInfo info;
        std::memcpy(&info, data, sizeof(info));
        if (info.step >= DataProc::kInitializationStepCount) return RES_PARAM_ERROR;
        info.name = initialization_name(info.step);
        info.title = initialization_title(info.step);
        return output(data, sizeof(info), info);
    }
    case kPollInitialization: {
        InitializationPoll poll;
        std::memcpy(&poll, data, sizeof(poll));
        if (poll.step >= DataProc::kInitializationStepCount) return RES_PARAM_ERROR;
        poll.detail = {};
        poll.status = poll_initialization_step(poll.step, poll.elapsed_ms, poll.detail.data(), poll.detail.size());
        poll.detail.back() = '\0';
        return output(data, sizeof(poll), poll);
    }
    default: return RES_UNSUPPORT;
    }
}
/** 按值读取 Power 设备。 */
PowerReport read_power() { return read_report<PowerReport>("Power"); }
/** 按值读取 Device 设备。 */
DeviceReport read_device() { return read_report<DeviceReport>("Device"); }
/** 按值读取 Status 设备。 */
DataProc::StatusSnapshot read_status() { return read_report<DataProc::StatusSnapshot>("Status"); }
/** 通过 Status 的精确尺寸 ioctl 获取产品能力。 */
Capabilities capabilities() {
    Capabilities value{};
    (void)control("Status", kCapabilities, &value, sizeof(value));
    return value;
}
/** 请求开机，RES_OK 表示平台已即时确认。 */
bool request_power_on() { return control("Power", kRequestPowerOn, nullptr, 0) == ::DeviceObject::RES_OK; }
/** 通过 Power 的 write 提交按值动作。 */
bool execute_power(DataProc::PowerAction action) {
    auto* dev = devices().getDevice("Power");
    return dev && dev->write(&action, sizeof(action)) == ::DeviceObject::RES_OK;
}
/** 通过 Device 的 ioctl 请求 Wi-Fi 开关。 */
bool request_wifi(bool on) { return control("Device", kSetWifi, &on, sizeof(on)) == ::DeviceObject::RES_OK; }
/** 初始设备快照也经过相同设备读接口。 */
DataProc::DeviceSnapshot initial_device() { return read_device().snapshot; }
/** 业务命令在同步回调内复制和执行。 */
bool execute_device(const DataProc::DeviceRequest& request) {
    auto* dev = devices().getDevice("Device");
    return dev && dev->write(&request, sizeof(request)) == ::DeviceObject::RES_OK;
}
/** 元数据查询不推进初始化步骤。 */
const char* initialization_name(unsigned step) {
    InitializationInfo info;
    info.step = step;
    (void)control("Status", kInitializationInfo, &info, sizeof(info));
    return info.name;
}
/** 读取步骤标题，不触发硬件探测。 */
i18n::TextId initialization_title(unsigned step) {
    InitializationInfo info;
    info.step = step;
    (void)control("Status", kInitializationInfo, &info, sizeof(info));
    return info.title;
}
/** 按值轮询并拷贝到调用方缓冲区，不保留指针。 */
DataProc::InitializationStatus poll_initialization_step(unsigned step, uint32_t elapsed, char* detail, size_t size) {
    if (!detail || !size) return DataProc::InitializationStatus::Failed;
    InitializationPoll poll;
    poll.step = step;
    poll.elapsed_ms = elapsed;
    if (control("Status", kPollInitialization, &poll, sizeof(poll)) != ::DeviceObject::RES_OK)
        return DataProc::InitializationStatus::Failed;
    std::snprintf(detail, size, "%s", poll.detail.data());
    return poll.status;
}
}
