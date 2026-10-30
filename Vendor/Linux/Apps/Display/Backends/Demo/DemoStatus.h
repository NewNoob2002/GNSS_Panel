#pragma once
#include <cstdint>
#include "DemoModel.h"
#include "UI/StatusBar/StatusBar.h"

/** @brief Demo 单一模型的确定性/随机采样配置；不发布应用快照。 */
class DemoStatus {
  public:
    using Clock = uint32_t (*)();
    /** @brief 启用自动采样；以当前 tick 为零点，同种子与时间可复现。 */
    static void configure(Clock clock, bool random = false, uint32_t seed = 0);
    /** @brief 测试/离线预览冻结在指定采样时间，返回同一模型的显示值。 */
    static page::StatusBarState sample(uint64_t elapsed_ms);
    /** @brief 每 3 秒生成随机状态，保持原 --random-status 语义。 */
    static page::StatusBarState sample_random(uint64_t elapsed_ms, uint32_t seed);
    /** @brief 当前模型运行时长，单位 ms。 */
    static uint64_t uptime_ms();
    /** @brief 设备同步读取前按自动时钟更新模型。 */
    static void refresh();
};
