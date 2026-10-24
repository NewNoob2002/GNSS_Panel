#include "lv_port.h"
#include <delay.h>

#if defined(__CORE_DEBUG) && (defined(__arm__) || defined(__thumb__))
#include <cstdint>

extern "C" {
extern uint32_t __StackLimit;
extern uint32_t __StackTop;
}

namespace {
constexpr uint32_t kStackPaint = 0xa55aa55aU;
uintptr_t painted_end;

// 在 main 前涂空闲 MSP 栈，保留当前帧下方 128 字节，避免覆盖初始化调用链。
__attribute__((constructor(101))) void
paint_debug_stack() {
    uintptr_t sp;
    uint32_t primask;
    asm volatile("mrs %0, msp" : "=r"(sp));
    asm volatile("mrs %0, primask\ncpsid i" : "=r"(primask)::"memory");
    const uintptr_t base = reinterpret_cast<uintptr_t>(&__StackLimit);
    const uintptr_t top = reinterpret_cast<uintptr_t>(&__StackTop);
    if (sp >= base + 128 && sp <= top) {
        painted_end = (sp - 128) & ~uintptr_t{3};
        for (uintptr_t p = base; p < painted_end; p += sizeof(uint32_t)) {
            *reinterpret_cast<volatile uint32_t*>(p) = kStackPaint;
        }
    }
    asm volatile("msr primask, %0" ::"r"(primask) : "memory");
    (void)lv_port_debug_stack_used(); // 保留调试查询入口，避免 --gc-sections 丢弃未被主循环调用的函数。
}
} // namespace

// 发布构建不产生代码、变量或构造器。
__attribute__((noinline)) uint32_t
lv_port_debug_stack_used() {
    if (!painted_end) {
        return 0;
    }
    uintptr_t p = reinterpret_cast<uintptr_t>(&__StackLimit);
    while (p < painted_end && *reinterpret_cast<volatile const uint32_t*>(p) == kStackPaint) {
        p += sizeof(uint32_t);
    }
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&__StackTop) - p);
}
#endif

void
lv_port_init() {
    // 输入不再经 LVGL indev：两键电平由 AppPlatform 交给共享应用（P4App）处理焦点与操作。
    lv_tick_set_cb(millis);
    lv_port_disp_init();
#if defined(__CORE_DEBUG)
    /** @brief 仅保留 GDB 查询入口，不在启动路径发额外亮度命令。 */
    asm volatile("" : : "r"(lv_port_debug_brightness));
#endif
}