#pragma once
#include "Helper/NodeHelper.h"
#include <array>

namespace DataProc {
constexpr std::size_t kInitializationStepCount = 5;
/** @brief Pending 仅为轮询中间态，不存入已完成结果。 */
enum class InitializationStatus { NotRun, Ok, Failed, Pending };
/** @brief 最近一次检查结果；detail 为最多 95 字节的 NUL 结尾诊断。 */
struct InitializationResult {
    InitializationStatus status{};
    std::array<char, 96> detail{};
    bool operator==(const InitializationResult& v) const { return status == v.status && detail == v.detail; }
};
}
namespace dp {
using InitializationResults = std::array<DataProc::InitializationResult, DataProc::kInitializationStepCount>;
/** @brief 最近一次初始化结果，重新 Begin 替换整次记录；不保存历史。 */
struct InitState {
    bool started = false;
    InitializationResults results{};
    bool operator==(const InitState& v) const { return started == v.started && results == v.results; }
};
enum class InitOperation { Begin, Record };
struct InitCommand { InitOperation operation{}; unsigned step = 0; DataProc::InitializationResult result{}; };
/** @brief 步骤名引用平台静态只读字符串，标题由领域层映射。 */
struct InitStepInfo {
    const char* name = "invalid";
    i18n::TextId title{i18n::TextId::Initializing};
};
enum class InitQueryOperation { Info, Poll };
/** @brief 同步查询载荷；输入 operation/step/elapsed_ms，输出归调用方所有，不发布中间态。 */
struct InitQuery {
    InitQueryOperation operation{};
    unsigned step = 0;
    uint32_t elapsed_ms = 0;
    InitStepInfo info{};
    DataProc::InitializationResult result{};
};
struct Init : Writer<names::Init, InitState, InitCommand> {
    /** @brief 经 Init 节点读取元数据，不推进检查。 */
    static int info(DataNode& caller, unsigned step, InitStepInfo& value);
    /** @brief 经 Init 节点执行一次有界轮询，elapsed_ms 为步骤开始后的毫秒数。 */
    static int poll(DataNode& caller, unsigned step, uint32_t elapsed_ms, DataProc::InitializationResult& value);
};
}
