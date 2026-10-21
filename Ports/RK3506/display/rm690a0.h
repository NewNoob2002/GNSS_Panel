#ifndef RM690A0_H
#define RM690A0_H

#include <stddef.h>
#include <stdint.h>

enum { RM_WIDTH = 126, RM_HEIGHT = 294 };

/* Synchronous callbacks return 0 or -1 with errno. Pixels are wire-order RGB565.
 * transfer holds hardware CS when keep_cs is true. A zero length releases CS.
 * The display must have exclusive use of its SPI bus for the entire session. */
struct RmIo {
    void* context;
    int (*dc)(void*, int);
    int (*reset)(void*, int);
    int (*sleep_ms)(void*, unsigned);
    int (*transfer)(void*, const uint8_t*, size_t, int keep_cs);
    size_t chunk_size;
};

struct Rm690a0 {
    struct RmIo io;
    int ready;
};

int rm_init(struct Rm690a0* screen);
int rm_write(struct Rm690a0* screen, int x, int y, int width, int height, const void* pixels, size_t bytes);
int rm_release(struct Rm690a0* screen);

#endif
