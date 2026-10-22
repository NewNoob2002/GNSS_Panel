#pragma once

#include "UI/Page.h"
#include "TemplateModel.h"
#include "TemplateView.h"

namespace page {
/** 新页面的最小 MVP 模板；复制后在 PageList.inc 显式登记。 */
class Template final : public PageBase {
  public:
    /** 安装时设置页面动画。 */
    void onInstalled() override;
    /** 创建本页 Model 与 View，仅在应用线程调用。 */
    void onViewLoad() override;
    /** 卸载时释放 Model，清除 View 对根对象子控件的引用。 */
    void onViewUnload() override;

  private:
    TemplateModel model_;
    TemplateView view_;
};
} // namespace page
