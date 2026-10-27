#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(HC32_LOG_OUTPUT_RTT)
#include "SEGGER_RTT.h"
#elif defined(HC32_LOG_OUTPUT_UART)
#include "usart.h"
#endif

// pi-lens-ignore: identifier
extern char __HeapBase;
extern char __HeapLimit;

void
DDL_AssertHandler(const char* file, int line) {
    (void)file;
    (void)line;
    while (1) {}
}

int
_close(int file) {
    (void)file;
    return -1;
}

int
_fstat(int file, struct stat* st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int
_isatty(int file) {
    (void)file;
    return 1;
}

int
_getpid(void) {
    return 1;
}

int
_kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int
_lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int
_read(int file, char* ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

caddr_t
_sbrk(int incr) {
    static uintptr_t heap_end;
    // pi-lens-ignore: identifier
    const uintptr_t base = (uintptr_t)&__HeapBase;
    // pi-lens-ignore: identifier
    const uintptr_t limit = (uintptr_t)&__HeapLimit;
    if (heap_end == 0U) {
        heap_end = base;
    }
    const uintptr_t previous = heap_end;
    if ((incr >= 0 && (uintptr_t)incr > limit - heap_end)
        || (incr < 0 && (uintptr_t)(-(int64_t)incr) > heap_end - base)) {
        errno = ENOMEM;
        return (caddr_t)-1;
    }
    if (incr >= 0) {
        heap_end += (uintptr_t)incr;
    } else {
        heap_end -= (uintptr_t)(-(int64_t)incr);
    }
    return (caddr_t)previous;
}

int
_write(int file, char* ptr, int len) {
    (void)file;
#if defined(HC32_LOG_OUTPUT_RTT)
    SEGGER_RTT_Write(0, ptr, len);
#elif defined(HC32_LOG_OUTPUT_UART)
    if (len > 0) {
        return (int)usart_write_buffer((const uint8_t*)ptr, (size_t)len);
    }
#else
    ((void)ptr);
    ((void)len);
#endif
    return len;
}
