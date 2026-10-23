#pragma once
#include <array>
#include <cstdint>
#include "HAL_Def.h"

namespace hal {
inline constexpr unsigned kInitializationStepCount = 5;
/** @brief 平台步骤语义，应用自行映射标题；不承载 i18n ID。 */
enum class InitStage { Configuration, BatteryGauge, Charger, HostPower, HostLink, Services, GNSS, Network, Finalization };
/** @brief 轮询结果；Pending 表示稍后再次轮询，不阻塞应用线程。 */
enum class InitStatus { NotRun, Ok, Failed, Pending };
/** @brief 步骤元数据查询；step 为输入 0..4，其余为输出，名称为平台静态只读字符串。 */
struct InitInfo {
    unsigned step = 0;
    const char* name = "invalid";
    InitStage stage{};
};
/** @brief 非阻塞轮询；输入 step 0..4、elapsed_ms（ms），其余输出归调用方所有。 */
struct InitPoll {
    unsigned step = 0;
    std::uint32_t elapsed_ms = 0;
    InitStatus status = InitStatus::Failed;
    std::array<char, 96> detail{}; ///< NUL 结尾，最多 95 字节。
};
/** @brief Init 仅支持这两个精确编码 ioctl，不提供 read/write。 */
inline constexpr uint32_t kInitializationInfo = DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN_OUT, sizeof(InitInfo), 'I', 1);
inline constexpr uint32_t kPollInitialization = DEVICE_OBJECT_IOCMD_DEF(DeviceObject::DIR_IN_OUT, sizeof(InitPoll), 'I', 2);
/** @brief Init 的同步解码层；元数据查询不推进步骤，轮询不保留调用方指针。 */
class InitObject : public DeviceObject {
public:
    InitObject() : DeviceObject(names::Init) {}
    virtual void info(InitInfo&) = 0;
    virtual void poll(InitPoll&) = 0;
private:
    int onInit() override { return RES_OK; }
    int onIoctl(IO_Cmd_t cmd, void* data) override {
        switch (command(cmd)) {
        case kInitializationInfo: {
            InitInfo value;
            std::memcpy(&value, data, sizeof(value));
            if (value.step >= kInitializationStepCount) return RES_PARAM_ERROR;
            info(value);
            return output(data, sizeof(value), value);
        }
        case kPollInitialization: {
            InitPoll value;
            std::memcpy(&value, data, sizeof(value));
            if (value.step >= kInitializationStepCount) return RES_PARAM_ERROR;
            value.detail = {};
            poll(value);
            value.detail.back() = '\0';
            return output(data, sizeof(value), value);
        }
        default: return RES_UNSUPPORT;
        }
    }
};
} // namespace hal
