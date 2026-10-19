#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
unsigned char test_heap[64];
__asm__(".globl __HeapBase\n.set __HeapBase, test_heap\n.globl __HeapLimit\n.set __HeapLimit, test_heap+64\n");
caddr_t _sbrk(int increment);

int
main(void) {
    assert(_sbrk(0) == (caddr_t)test_heap);
    assert(_sbrk(32) == (caddr_t)test_heap);
    assert(_sbrk(32) == (caddr_t)(test_heap + 32));
    assert(_sbrk(1) == (caddr_t)-1 && errno == ENOMEM);
    assert(_sbrk(-64) == (caddr_t)(test_heap + 64));
    assert(_sbrk(-1) == (caddr_t)-1);
    assert(_sbrk(INT_MIN) == (caddr_t)-1);
    assert(_sbrk(INT_MAX) == (caddr_t)-1);
    assert(_sbrk(0) == (caddr_t)test_heap);
    puts("C heap boundary regression passed");
}
