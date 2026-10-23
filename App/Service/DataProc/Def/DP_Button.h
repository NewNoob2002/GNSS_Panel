#pragma once
#include "Helper/NodeHelper.h"
#include "ButtonGesture.h"

namespace dp {
/** @brief 应用线程的逻辑按键；Both 仅用于同时取消两键。 */
enum class ButtonKey { Power, Function, Both };
enum class ButtonOperation { Level, Poll, Cancel };
/** @brief 同一次采样的两键动作和去抖电平；动作沿用 ButtonGesture 的互斥语义。 */
struct ButtonState {
    ButtonGesture::Action power{}, function{};
    bool power_held = false, function_held = false;
};
/** @brief 应用线程输入命令；Cancel 可清除电平，Poll 的毫秒时间在节点内单调化。 */
struct ButtonCommand {
    ButtonOperation operation{};
    ButtonKey key = ButtonKey::Both;
    bool pressed = false, clear_levels = false;
    uint64_t time_ms = 0;
};
using Button = Writer<names::Button, ButtonState, ButtonCommand>;
}
