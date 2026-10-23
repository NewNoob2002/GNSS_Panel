#pragma once
#include "Helper/NodeHelper.h"

namespace dp {
/** @brief Configure 在 UI 组装时设置连续 page ID 上界；Consume 仅供 UI 定时器确认队首。 */
enum class PageNaviOperation { Push, Pop, Replace, ResetRoot, Preload, Configure, Consume };
/** @brief 页面 ID 由 UI 定义，DataProc 只校验配置的数值范围；Pop/Consume 的 page 必须为零。 */
struct PageNaviCommand {
    PageNaviOperation operation{};
    uint16_t page = 0;
};
/** @brief 有界 FIFO 的队首值快照；没有请求时 pending 为零。 */
struct PageNaviState {
    PageNaviCommand request{};
    unsigned pending = 0;
    bool operator==(const PageNaviState& v) const {
        return pending == v.pending && request.operation == v.request.operation && request.page == v.request.page;
    }
};
using PageNavi = Writer<names::PageNavi, PageNaviState, PageNaviCommand>;
}
