#pragma once
#include "DataProc.h"

/** @brief 集中定义所有服务节点 ID；UI 旧接口在 3.7e 迁移。 */
namespace dp::names {
inline constexpr char Power[] = "Power", Battery[] = "Battery", GNSS[] = "GNSS", Clock[] = "Clock";
inline constexpr char Work[] = "Work", Recorder[] = "Recorder", WiFi[] = "WiFi", Version[] = "Version";
inline constexpr char Init[] = "Init", Settings[] = "Settings";
inline constexpr char Status[] = "Status", System[] = "System", Device[] = "Device";
}

namespace dp {
/** @brief 只读节点的类型化同步接口；订阅不自动发布，首次值通过 pull 获取。 */
template<const char* Name, class Value> struct Reader {
    using State = Value;
    static bool subscribe(DataNode& caller) { return caller.subscribe(Name) != nullptr; }
    static int pull(DataNode& caller, State& value) { return caller.pull(Name, &value, sizeof(value)); }
    static int receive(const DataNode::EventParam_t* event, State& value) {
        if (!event || !event->tran || event->event != DataNode::EVENT_PUBLISH
            || std::strcmp(event->tran->getID(), Name) != 0) return DataNode::RES_UNSUPPORTED_REQUEST;
        return DataProc::ReadPayload(event, value);
    }
};
/** @brief 命令与状态分别传输；所有载荷按精确尺寸同步解码。 */
template<const char* Name, class Value, class Request> struct Writer : Reader<Name, Value> {
    using Command = Request;
    static int notify(DataNode& caller, const Command& command) { return caller.notify(Name, &command, sizeof(command)); }
};
}
