// 带屏产品的 C++ 动态内存使用 LVGL 内存池，覆盖 Platform/Core/src/new.cpp 中的弱符号默认实现。
#include <new>
#include <stdlib.h>
#include "lvgl.h"

static void*
allocate(size_t size) {
    if (!lv_is_initialized()) {
        lv_init();
    }
    return lv_mem_alloc(size == 0U ? 1U : size);
}

void*
operator new(size_t size) {
    void* ptr = allocate(size);
    if (!ptr) {
        // 固件关闭异常，普通 new 失败必须终止，不能违反 C++ 的非空返回约定。
        abort();
    }
    return ptr;
}

void*
operator new[](size_t size) {
    return ::operator new(size);
}

void*
operator new(size_t size, const std::nothrow_t&) noexcept {
    return allocate(size);
}

void*
operator new[](size_t size, const std::nothrow_t&) noexcept {
    return allocate(size);
}

void
operator delete(void* ptr) noexcept {
    lv_mem_free(ptr);
}

void
operator delete[](void* ptr) noexcept {
    lv_mem_free(ptr);
}

void
operator delete(void* ptr, size_t) noexcept {
    lv_mem_free(ptr);
}

void
operator delete[](void* ptr, size_t) noexcept {
    lv_mem_free(ptr);
}

void
operator delete(void* ptr, const std::nothrow_t&) noexcept {
    lv_mem_free(ptr);
}

void
operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    lv_mem_free(ptr);
}
