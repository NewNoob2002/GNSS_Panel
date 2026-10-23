#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Clock.h"
namespace DataProc {
/** @brief 应用 RAM 日期；不修改平台时钟。 */
struct DateTime {
    unsigned year = 2026, month = 9, day = 30, hour = 12, minute = 30;
    bool operator==(const DateTime& v) const {
        return year == v.year && month == v.month && day == v.day && hour == v.hour && minute == v.minute;
    }
};
}
namespace dp {
/** @brief 平台时钟与应用 RAM 日期互相独立；SetDate 不修改 HAL。 */
struct ClockState {
    hal::ClockState clock{};
    DataProc::DateTime date{};
    bool operator==(const ClockState& v) const {
        return clock == v.clock && date.year == v.date.year && date.month == v.date.month && date.day == v.date.day
            && date.hour == v.date.hour && date.minute == v.date.minute;
    }
};
struct ClockCommand { DataProc::DateTime date{}; };
using Clock = Writer<names::Clock, ClockState, ClockCommand>;
/** @brief 公历月份天数；非法月份返回 0。 */
unsigned days_in_month(unsigned year, unsigned month);
/** @brief 仅允许 2000..2099 年的有效日期，时/分范围 0..23/0..59。 */
bool valid_date(const DataProc::DateTime& date);
}
