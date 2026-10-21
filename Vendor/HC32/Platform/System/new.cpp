// 平台默认的 C++ 动态内存：使用链接脚本预留的 C 堆（_sbrk）。
// 带屏产品在 Display/Lvgl/lvgl_new.cpp 中以强符号改用 LVGL 内存池，这里全部声明为弱符号。
#include <new>
#include <stdlib.h>

#define PLATFORM_WEAK __attribute__((weak))

PLATFORM_WEAK void*
operator new(size_t size) {
    void* ptr = malloc(size == 0U ? 1U : size);
    if (!ptr) {
        // 固件关闭异常，普通 new 失败必须终止，不能违反 C++ 的非空返回约定。
        abort();
    }
    return ptr;
}

PLATFORM_WEAK void*
operator new[](size_t size) {
    return ::operator new(size);
}

PLATFORM_WEAK void*
operator new(size_t size, const std::nothrow_t&) noexcept {
    return malloc(size == 0U ? 1U : size);
}

PLATFORM_WEAK void*
operator new[](size_t size, const std::nothrow_t&) noexcept {
    return malloc(size == 0U ? 1U : size);
}

PLATFORM_WEAK void
operator delete(void* ptr) noexcept {
    free(ptr);
}

PLATFORM_WEAK void
operator delete[](void* ptr) noexcept {
    free(ptr);
}

PLATFORM_WEAK void
operator delete(void* ptr, size_t) noexcept {
    free(ptr);
}

PLATFORM_WEAK void
operator delete[](void* ptr, size_t) noexcept {
    free(ptr);
}

PLATFORM_WEAK void
operator delete(void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}

PLATFORM_WEAK void
operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    free(ptr);
}
