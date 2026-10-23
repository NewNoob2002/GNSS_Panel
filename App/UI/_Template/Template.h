#pragma once

#include <memory>
#include "UI/Page.h"
#include "TemplateModel.h"
#include "TemplateView.h"

namespace page {
/** @brief 新页面复制时保持：Page 转发输入/导航，Model 持有值与 subject，View 写控件。 */
class Template final : public PageBase {
  public:
    /** 安装时设置页面动画。 */
    void onInstalled() override;
    /** 创建本页 Model 与 View，仅在应用线程调用。 */
    void onViewLoad() override;
    void onViewWillAppear() override;
    void onViewWillDisappear() override;
    /** @brief 先解绑 View，再销毁 Model；根对象仍由 PageManager 释放。 */
    void onViewUnload() override;

  private:
    static void clicked(lv_event_t* event);
    std::unique_ptr<TemplateModel> model_;
    TemplateView view_;
};
} // namespace page
