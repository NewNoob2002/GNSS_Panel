#pragma once
#include <cstdint>
#include "UI/Common/Model.h"
#include "Def/DP_Version.h"
#include "Def/DP_Init.h"

namespace page {
/** @brief SystemLoading 只需要一个字段：平台是否为模拟环境。 */
struct SystemLoadingState {
    bool simulated = false;
};

/** @brief 命令节点：订阅 Version 取初值，通过 Init 节点发送 Begin/Record 命令。
 *  不需要 PUBLISH 接收——初始化步骤结果由 Page 轮询，不走发布路径。
 */
class SystemLoadingModel final : public Model<SystemLoadingState> {
  public:
    SystemLoadingModel() : Model("SystemLoadingModel") {}
    bool init();

    bool set_status_bar(bool visible, DataProc::StatusBarStyle style = {});

    /** UI 线程：开始新一轮初始化（发送 Begin 命令）。 */
    bool begin_initialization();

    /** UI 线程：轮询初始化步骤 0..4；返回 Pending/Ok/Failed。 */
    DataProc::InitializationStatus poll_step(unsigned step, uint32_t elapsed_ms);

    /** @brief UI 线程：经 Init 节点读取静态步骤元数据。 */
    dp::InitStepInfo step_info(unsigned step) {
        dp::InitStepInfo value;
        (void)dp::Init::info(*this, step, value);
        return value;
    }

    static constexpr uint32_t kStepLimitMs = 35000;

    int onEvent(EventParam_t* event) override;

  private:
    template <class Domain>
    bool load() {
        typename Domain::State value;
        if (!Domain::subscribe(*this) || Domain::pull(*this, value) != RES_OK)
            return false;
        apply(value);
        return true;
    }
    void apply(const dp::Version::State& value);
};
} // namespace page
