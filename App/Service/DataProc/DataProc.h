#pragma once

#include <cstring>
#include <type_traits>
#include "DataProc_Def.h"
#include "DataBroker.h"

/** @brief 页面 Model 创建前初始化领域节点，注入独立于 LVGL/OS 的毫秒时钟。
 * 同步读取 HAL 值，不发命令；任一节点或定时器创建失败时逆序回滚。
 */
bool DataProc_Init(DataTimerManager::TickFunction_t tick_source);
/** Execute service timers on the application thread; returns time until the next deadline. */
uint32_t DataProc_RunLoop();
/** Call after all page/component DataNodes have been destroyed. */
bool DataProc_Deinit();

namespace DataProc {
/** Shared center accessor only; routing is implemented by Frameworks/DataBroker. */
DataBroker* Broker();

/** Validate native payload size and copy, avoiding alignment/aliasing assumptions. */
template <class T> int ReadPayload(const DataNode::EventParam_t* event, T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "DataNode payload must be trivially copyable");
    if (!event || !event->data_p)
        return DataNode::RES_PARAM_ERROR;
    if (event->size != sizeof(T))
        return DataNode::RES_SIZE_MISMATCH;
    std::memcpy(&value, event->data_p, sizeof(T));
    return DataNode::RES_OK;
}
} // namespace DataProc
