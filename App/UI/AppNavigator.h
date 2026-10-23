#pragma once
#include "InputRouter.h"
#include "PageId.h"
#include "Def/DP_PageNavi.h"
#include "Def/DP_Power.h"
#include "Def/DP_Settings.h"

namespace page {
/** @brief 组装后的 UI 导航与语言订阅者；所有节点导航只在 20 ms 定时器执行。 */
class AppNavigator final : public DataNode {
  public:
    AppNavigator(PageManager& manager, lv_group_t* group, BatteryWarning& warning);
    ~AppNavigator() override;
    bool init();
    void stop();
    int onEvent(EventParam_t* event) override;
    InputRouter& input() { return input_; }
    const InputRouter& input() const { return input_; }
    /** @brief 平台预览接口保持同步语义，不得在 DataNode 回调链中调用。 */
    bool show_page(PageId id);

  private:
    PageManager& manager_;
    BatteryWarning& warning_;
    InputRouter input_;
    i18n::Language language_{};
    lv_timer_t* timer_ = nullptr;
    unsigned preload_index_ = 0;
    static void on_navigation(lv_timer_t* timer);
    void navigate();
    void preload_next();
    bool execute(const dp::PageNaviCommand& command);
    void acknowledge(dp::PowerNavigation consumed);
};
}
