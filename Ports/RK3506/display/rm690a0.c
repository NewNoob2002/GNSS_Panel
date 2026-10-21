#include "rm690a0.h"
#include <errno.h>

static int failed(struct Rm690a0* s) {
    int error = errno;
    s->ready = 0;
    s->io.transfer(s->io.context, NULL, 0, 0);
    errno = error;
    return -1;
}

int rm_release(struct Rm690a0* s) {
    s->ready = 0;
    return s->io.transfer(s->io.context, NULL, 0, 0);
}

static int send(struct Rm690a0* s, uint8_t command, const uint8_t* data, size_t n, int keep) {
    struct RmIo* io = &s->io;
    if (io->dc(io->context, 0) < 0 || io->transfer(io->context, &command, 1, n || keep) < 0)
        return -1;
    if (n && (io->dc(io->context, 1) < 0 || io->transfer(io->context, data, n, keep) < 0))
        return -1;
    return 0;
}

int rm_write(struct Rm690a0* s, int x, int y, int w, int h, const void* pixels, size_t bytes) {
    if (!s->ready) {
        errno = EIO;
        return -1;
    }
    if (!pixels || x < 0 || y < 0 || w <= 0 || h <= 0 || x >= RM_WIDTH || y >= RM_HEIGHT || w > RM_WIDTH - x
        || h > RM_HEIGHT - y || bytes != (size_t)w * (size_t)h * 2) {
        errno = EINVAL;
        return -1;
    }
    uint8_t column[] = {0, (uint8_t)x, 0, (uint8_t)(x + w - 1)};
    uint8_t row[] = {(uint8_t)(y >> 8), (uint8_t)y, (uint8_t)((y + h - 1) >> 8), (uint8_t)(y + h - 1)};
    if (send(s, 0x2a, column, sizeof(column), 1) < 0 || send(s, 0x2b, row, sizeof(row), 1) < 0
        || send(s, 0x2c, NULL, 0, 1) < 0 || s->io.dc(s->io.context, 1) < 0)
        return failed(s);
    const uint8_t* p = pixels;
    while (bytes) {
        size_t n = bytes < s->io.chunk_size ? bytes : s->io.chunk_size;
        if (s->io.transfer(s->io.context, p, n, bytes > n) < 0)
            return failed(s);
        bytes -= n;
        p += n;
    }
    return 0;
}

int rm_init(struct Rm690a0* s) {
    struct RmIo* io = &s->io;
    s->ready = 0;
    if (!io->dc || !io->reset || !io->sleep_ms || !io->transfer || io->chunk_size < 4 || io->chunk_size > 32768
        || io->chunk_size % 2) {
        errno = EINVAL;
        return -1;
    }
    /* HC32 generic_RM690A0, unchanged values; encoded 255 delay means 500 ms. */
    static const struct {
        uint8_t cmd, count, value;
        unsigned delay;
    } init[] = {{0x01, 0, 0, 150},  {0xfe, 1, 0x01, 0}, {0x6a, 1, 0x21, 0}, {0xfe, 1, 0x00, 0},
                {0xc4, 1, 0x80, 0}, {0x35, 1, 0x00, 0}, {0x51, 1, 0xff, 0}, {0x3a, 1, 0x05, 0},
                {0x20, 0, 0, 0},    {0x11, 0, 0, 500},  {0x29, 0, 0, 500},  {0x36, 1, 0x00, 0}};
    static const unsigned delays[] = {100, 100, 200, 50, 50, 150};
    if (rm_release(s) < 0)
        return failed(s);
    for (size_t i = 0; i < sizeof(delays) / sizeof(delays[0]); ++i) {
        if (io->reset(io->context, i % 3 != 1) < 0 || io->sleep_ms(io->context, delays[i]) < 0)
            return failed(s);
    }
    for (size_t i = 0; i < sizeof(init) / sizeof(init[0]); ++i) {
        if (send(s, init[i].cmd, &init[i].value, init[i].count, 0) < 0
            || (init[i].delay && io->sleep_ms(io->context, init[i].delay) < 0))
            return failed(s);
    }
    s->ready = 1;
    /* Clear in two-row windows, no full-frame shadow buffer. */
    const uint8_t black[RM_WIDTH * 2 * 2] = {0};
    for (int y = 0; y < RM_HEIGHT; y += 2)
        if (rm_write(s, 0, y, RM_WIDTH, 2, black, sizeof(black)) < 0)
            return -1;
    return 0;
}
