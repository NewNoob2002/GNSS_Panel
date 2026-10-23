#pragma once
#include <cstdint>

namespace page {
/** @brief 显式清单生成的页面 ID；Invalid 不对应任何已安装页面。 */
enum class PageId : uint16_t {
#define PAGE_DEF(Name, InstallPredicate) Name,
#include "PageList.inc"
#undef PAGE_DEF
    Count,
    Invalid = UINT16_MAX
};
/** @brief 返回 ID 对应的静态路由名，非法 ID 返回 nullptr。 */
const char* name(PageId id);
/** @brief 将平台预览接口的路由名转换为页面 ID，未知名字返回 Invalid。 */
PageId id(const char* name);
}
