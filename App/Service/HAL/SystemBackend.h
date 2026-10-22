#pragma once
#include <cstddef>
#include <cstdint>
#include "DataProc/DataProc_Def.h"
#include "DeviceObject.h"
#include "DeviceManager.h"

/** Platform implementation of system operations; exactly one per build.
 * Called by SystemService on the application thread; must be bounded and must not
 * block on I/O. The platform reports asynchronous results through SystemService (see SystemService.h).
 */
namespace system_backend {
struct Capabilities {
    bool simulated;        // Show DEMO markers; data and power actions are not real.
    bool reboot_available; // SystemDash offers Reboot.
    bool wifi_available;   // SystemDash may toggle Wi-Fi.
    bool star_map;         // StarMap page, reached from the Dialplate satellite icon.
    bool record_settings;  // RecordConfig type/interval can be changed (otherwise display only).
};
/** 电源状态按值读取；异步状态上报仍由平台主循环驱动。 */
struct PowerReport {
    bool powered_on = false, shutting_down = false, standby = false;
    DataProc::PowerCause cause = DataProc::PowerCause::User;
};
/** 设备业务和 Wi-Fi 使用同一时刻的值快照。 */
struct DeviceReport {
    DataProc::DeviceSnapshot snapshot;
    bool wifi_on = false, wifi_pending = false;
};
/** 初始化元数据查询不执行硬件检查。名称指针仅供同步读取平台静态字符串。 */
struct InitializationInfo {
    unsigned step = 0;
    const char* name = "invalid";
    i18n::TextId title = i18n::TextId::Initializing;
};
/** 初始化轮询的输入输出均由调用方拥有，不跨回调保存。 */
struct InitializationPoll {
    unsigned step = 0;
    std::uint32_t elapsed_ms = 0;
    DataProc::InitializationStatus status = DataProc::InitializationStatus::Failed;
    std::array<char, 96> detail{};
};
// ioctl 的方向、类型、编号和尺寸构成完整命令，必须精确匹配。
constexpr uint32_t kRequestPowerOn = DEVICE_OBJECT_IOCMD_DEF(::DeviceObject::DIR_NONE, 0, 'P', 1);
constexpr uint32_t kSetWifi = DEVICE_OBJECT_IOCMD_DEF(::DeviceObject::DIR_IN, sizeof(bool), 'D', 1);
constexpr uint32_t kCapabilities = DEVICE_OBJECT_IOCMD_DEF(::DeviceObject::DIR_OUT, sizeof(Capabilities), 'S', 1);
constexpr uint32_t kInitializationInfo = DEVICE_OBJECT_IOCMD_DEF(::DeviceObject::DIR_IN_OUT, sizeof(InitializationInfo), 'S', 2);
constexpr uint32_t kPollInitialization = DEVICE_OBJECT_IOCMD_DEF(::DeviceObject::DIR_IN_OUT, sizeof(InitializationPoll), 'S', 3);

/** Power 设备的公共解码层；平台只实现类型明确的操作，全部在应用线程同步执行。 */
class PowerObject : public ::DeviceObject {
public:
    /** 注册名固定为 Power，不触碰硬件。 */
    PowerObject() : ::DeviceObject("Power") {}
    /** 读取平台电源状态。 */
    virtual PowerReport power_report() = 0;
    /** Startup 完成长按后请求开机；返回即时确认结果。 */
    virtual bool request_power_on() = 0;
    /** 执行已确认的关机或重启请求。 */
    virtual bool execute_power(DataProc::PowerAction action) = 0;
private:
    /** 设备初始化只标记可用，不改变电源状态。 */
    int onInit() override { return RES_OK; }
    /** 按值输出 PowerReport，拒绝尺寸不匹配。 */
    int onRead(void*, size_t) override;
    /** 按值解码 PowerAction。 */
    int onWrite(const void*, size_t) override;
    /** 分派精确匹配的开机命令。 */
    int onIoctl(IO_Cmd_t, void*) override;
};
/** Device 设备的公共解码层；选项索引与协议码转换属于平台实现。 */
class BusinessObject : public ::DeviceObject {
public:
    /** 注册名固定为 Device。 */
    BusinessObject() : ::DeviceObject("Device") {}
    /** 返回设备快照和 Wi-Fi 状态。 */
    virtual DeviceReport device_report() = 0;
    /** 执行服务已经校验来源及范围的业务请求。 */
    virtual bool execute_device(const DataProc::DeviceRequest& request) = 0;
    /** 请求切换 Wi-Fi，异步结果由平台上报。 */
    virtual bool request_wifi(bool on) = 0;
private:
    /** 无硬件副作用的初始化。 */
    int onInit() override { return RES_OK; }
    /** 按值输出 DeviceReport。 */
    int onRead(void*, size_t) override;
    /** 按值解码业务请求。 */
    int onWrite(const void*, size_t) override;
    /** 分派 Wi-Fi 开关命令。 */
    int onIoctl(IO_Cmd_t, void*) override;
};
/** Status 设备承载状态栏、能力和初始化检查，核心不依赖 LVGL。 */
class StatusObject : public ::DeviceObject {
public:
    /** 注册名固定为 Status。 */
    StatusObject() : ::DeviceObject("Status") {}
    /** 返回当前状态栏值快照。 */
    virtual DataProc::StatusSnapshot status_snapshot() = 0;
    /** 返回产品能力。 */
    virtual Capabilities capabilities() = 0;
    /** 返回初始化步骤的静态名称。 */
    virtual const char* initialization_name(unsigned step) = 0;
    /** 返回初始化步骤的本地化标题。 */
    virtual i18n::TextId initialization_title(unsigned step) = 0;
    /** 非阻塞轮询步骤；detail 必须在容量内以 NUL 结尾。 */
    virtual DataProc::InitializationStatus poll_initialization_step(unsigned, uint32_t, char*, size_t) = 0;
private:
    /** 无硬件副作用的初始化。 */
    int onInit() override { return RES_OK; }
    /** 按值输出状态栏快照。 */
    int onRead(void*, size_t) override;
    /** 分派能力、元数据与轮询命令。 */
    int onIoctl(IO_Cmd_t, void*) override;
};
/** 每个程序提供恰好一组 Power/Device/Status，返回已初始化的静态管理器。 */
DeviceManager& devices();
/** 读取电源报告，失败时返回默认状态。 */
PowerReport read_power();
/** 读取设备报告，失败时返回默认状态。 */
DeviceReport read_device();
/** 读取状态栏报告，失败时返回默认状态。 */
DataProc::StatusSnapshot read_status();
Capabilities capabilities();
/** Startup hold completed. Return true to confirm power-on now; a platform that decides power-on itself
 * returns false and later calls SystemService::set_powered_on(true).
 */
bool request_power_on();
/** Execute a confirmed user power action; false on failure. Completion of Off is reported with
 * SystemService::enter_standby(), possibly later or never (when power is really removed).
 */
bool execute_power(DataProc::PowerAction action);
/** Wi-Fi on/off requested on SystemDash (only when wifi_available). The platform reports the applied
 * state, and whether a change is still pending, with SystemService::set_wifi().
 */
bool request_wifi(bool on);
/** Device/business data before the platform's first DeviceService::update(). */
DataProc::DeviceSnapshot initial_device();
/** Execute a validated business request (indexes already range-checked); false on failure. The platform
 * publishes data with DeviceService::update(). For asynchronous requests, set work_pending/record_pending
 * before returning true and clear it only when the host has received the request. Receipt is not proof
 * that the physical operation has finished.
 */
bool execute_device(const DataProc::DeviceRequest& request);
/** Short name of initialization step 0..kInitializationStepCount-1, for logs. */
const char* initialization_name(unsigned step);
/** Localized SystemLoading headline of the step (what is actually being initialized or checked). */
i18n::TextId initialization_title(unsigned step);
/** Advance initialization step 0..kInitializationStepCount-1 without blocking. SystemLoading polls it on the UI
 * thread about every 50 ms; elapsed_ms counts from the step's first poll (0). Return Pending while the check is
 * still running, otherwise Ok/Failed with a NUL-terminated detail. Each step's duration is the real check time;
 * the platform bounds it with its own timeout (SystemLoading also stops waiting after kStepLimitMs). A failure is
 * shown and logged but never stops startup.
 */
DataProc::InitializationStatus poll_initialization_step(unsigned step, std::uint32_t elapsed_ms, char* detail,
                                                        std::size_t size);
} // namespace system_backend
