#pragma once

#include "I18n.h"

/** @brief 应用线程同步值载荷；参数不跨回调保存，不作为线协议或存储格式。 */
namespace DataProc {
enum class StatusBarStyle { Default = 0, Transparent };
struct StatusBarPresentation {
    bool visible = true;
    StatusBarStyle style{};
};
}
