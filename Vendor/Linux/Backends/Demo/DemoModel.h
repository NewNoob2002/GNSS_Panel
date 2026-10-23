#pragma once
#include "Def/HAL_Battery.h"
#include "Def/HAL_Board.h"
#include "Def/HAL_Clock.h"
#include "Def/HAL_GNSS.h"
#include "Def/HAL_Power.h"
#include "Def/HAL_Recorder.h"
#include "Def/HAL_WiFi.h"
#include "Def/HAL_Work.h"

namespace demo {
/** @brief Demo 唯一模拟模型；所有细设备读取这里，命令也仅修改这里。 */
struct Model {
    hal::PowerState power{};
    hal::BatteryState battery{};
    hal::GNSSState gnss{};
    hal::ClockState clock{};
    hal::WorkState work{};
    hal::RecorderState recorder{};
    hal::WifiState wifi{};
    hal::BoardState board{};
};

/** @brief 应用线程拥有的唯一模型，测试可直接修改设备状态再推进节点定时器。 */
Model& model();
/** @brief 按配置时钟推进模型，所有设备在同一 tick 读取一致值。 */
void refresh();
} // namespace demo
