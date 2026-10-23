#pragma once

#include "UI/Common/Model.h"
#include "Def/DP_Recorder.h"

namespace page {
/** @brief 新页复制步骤：定义最小值、声明领域订阅、pull 初值、解码后按值通知、提供类型化命令。
 * Model 在 onViewLoad 创建，在 View 解绑后销毁；subject 指向 Model 自有值。
 */
class TemplateModel final : public Model<bool> {
  public:
    TemplateModel() : Model("TemplateModel") {}
    bool init();
    int onEvent(EventParam_t* event) override;
    /** @brief 命令只经对应领域 Helper 同步发送，不经 Device 聚合节点。 */
    bool toggle_recording();
};
} // namespace page
