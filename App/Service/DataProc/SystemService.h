#pragma once
#include "DataProc/DataProc.h"

/** @brief 页面迁移期的聚合状态读取接口；领域节点独占状态写入。 */
namespace SystemService {
/** @brief 返回当前 System 聚合值快照。 */
DataProc::SystemState snapshot();
/** @brief 校验应用 RAM 日期的范围及闰年。 */
bool valid_date(const DataProc::DateTime& date);
/** @brief 返回月份天数，非法月份返回零。 */
unsigned days_in_month(unsigned year, unsigned month);
}
namespace DeviceService {
/** @brief 返回当前 Device 聚合值快照。 */
DataProc::DeviceSnapshot snapshot();
}
