#pragma once

#include <memory>
#include "DataProc/DataProc.h"

namespace page {
/** 页面独占的 DataNode；按需在此增加快照与订阅。 */
class TemplateModel {
  public:
    /** 在服务初始化后创建节点；注册失败返回 false。 */
    bool init();
    /** 在服务销毁前释放节点，可重复调用。 */
    void deinit();

  private:
    std::unique_ptr<DataNode> account_;
};
} // namespace page
