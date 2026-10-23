#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "DeviceObject.h"

/** @brief 应用线程同步 HAL 契约；数据归调用方所有，指针不跨同步回调保存。 */
namespace hal {
/** @brief 设备注册名，领域节点仅依赖这些集中定义的契约 ID。 */
namespace names {
inline constexpr char Power[] = "Power", Battery[] = "Battery", GNSS[] = "GNSS", Clock[] = "Clock";
inline constexpr char Work[] = "Work", Recorder[] = "Recorder", WiFi[] = "WiFi", Board[] = "Board", Init[] = "Init";
}

/** @brief 精确尺寸同步读取；空设备返回 RES_INIT_FAILED，输出仅在成功时更新。 */
template<class T> int read(DeviceObject* device, T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "HAL payload must be a value type");
    if (!device) return DeviceObject::RES_INIT_FAILED;
    T result{};
    const int status = device->read(&result, sizeof(result));
    if (status == DeviceObject::RES_OK) value = result;
    return status;
}
/** @brief 精确尺寸同步写入；调用方保留命令所有权，返回设备错误码。 */
template<class T> int write(DeviceObject* device, const T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "HAL payload must be a value type");
    return device ? device->write(&value, sizeof(value)) : DeviceObject::RES_INIT_FAILED;
}
/** @brief 同步 ioctl；完整编码由设备核对，尺寸不符返回 RES_PARAM_ERROR。 */
template<class T> int ioctl(DeviceObject* device, uint32_t cmd, T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "HAL payload must be a value type");
    if (!device) return DeviceObject::RES_INIT_FAILED;
    T result = value;
    const int status = device->ioctl(cmd, &result, sizeof(result));
    if (status == DeviceObject::RES_OK) value = result;
    return status;
}
/** @brief 无载荷同步 ioctl；非零编码尺寸由 DeviceObject 拒绝。 */
inline int ioctl(DeviceObject* device, uint32_t cmd) {
    return device ? device->ioctl(cmd) : DeviceObject::RES_INIT_FAILED;
}
/** @brief 重建完整 ioctl 编码，避免读取联合体非活动成员。 */
inline uint32_t command(DeviceObject::IO_Cmd_t cmd) {
    return DEVICE_OBJECT_IOCMD_DEF(cmd.ch.dir, cmd.ch.size, cmd.ch.type, cmd.ch.number);
}
/** @brief 按值输出；不假定外部缓冲区对齐，尺寸必须精确匹配。 */
template<class T> int output(void* buffer, size_t size, const T& value) {
    if (size != sizeof(T)) return DeviceObject::RES_PARAM_ERROR;
    std::memcpy(buffer, &value, sizeof(value));
    return DeviceObject::RES_OK;
}
/** @brief 只读设备的公共解码层；平台采样在应用线程同步完成，无初始化副作用。 */
template<class State> class ReadObject : public DeviceObject {
public:
    explicit ReadObject(const char* name) : DeviceObject(name) {}
    virtual State state() = 0;
private:
    int onInit() override { return RES_OK; }
    int onRead(void* buffer, size_t size) override {
        if (size != sizeof(State)) return RES_PARAM_ERROR;
        return output(buffer, size, state());
    }
};
/** @brief 可写设备的公共解码层；平台必须在任何副作用前校验命令范围。 */
template<class State, class Command> class WriteObject : public ReadObject<State> {
public:
    explicit WriteObject(const char* name) : ReadObject<State>(name) {}
    virtual int execute(const Command&) = 0;
private:
    int onWrite(const void* buffer, size_t size) override {
        if (size != sizeof(Command)) return DeviceObject::RES_PARAM_ERROR;
        Command value;
        std::memcpy(&value, buffer, sizeof(value));
        return execute(value);
    }
};
} // namespace hal
