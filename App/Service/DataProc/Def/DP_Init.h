#pragma once
#include "Helper/NodeHelper.h"
#include "HAL/Def/HAL_Init.h"
namespace dp {
using InitializationResults = std::array<DataProc::InitializationResult, DataProc::kInitializationStepCount>;
/** @brief 最近一次初始化结果，重新 Begin 替换整次记录；不保存历史。 */
struct InitState {
    bool started = false;
    InitializationResults results{};
    bool operator==(const InitState& v) const {
        if (started != v.started) return false;
        for (size_t i = 0; i < results.size(); ++i)
            if (results[i].status != v.results[i].status || results[i].detail != v.results[i].detail) return false;
        return true;
    }
};
enum class InitOperation { Begin, Record };
struct InitCommand { InitOperation operation{}; unsigned step = 0; DataProc::InitializationResult result{}; };
using Init = Writer<names::Init, InitState, InitCommand>;
/** @brief 按需读取 HAL 初始化步骤元数据，不推进步骤。 */
int initialization_info(hal::InitInfo& value);
/** @brief 按需执行一次 HAL 有界轮询，返回值归调用方所有。 */
int initialization_poll(hal::InitPoll& value);
}
