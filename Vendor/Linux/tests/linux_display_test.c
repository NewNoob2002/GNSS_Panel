#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <time.h>
#include "linux_display.h"

static int calls, fail_at, opened[3], releases, short_write, lock_failure, interrupted;

static int step(void) {
    if (++calls == fail_at) {
        errno = EACCES;
        return -1;
    }
    return 0;
}

int __wrap_open(const char* path, int flags, ...) {
    if (step() < 0)
        return -1;
    assert(flags & O_CLOEXEC);
    int index = !strcmp(path, "spi") ? 0 : 1;
    assert((flags & O_ACCMODE) == (index ? O_RDONLY : O_RDWR));
    assert(!opened[index]);
    opened[index] = 1;
    return 100 + index;
}

int __wrap_close(int fd) {
    assert(fd >= 100 && fd <= 102 && opened[fd - 100]);
    opened[fd - 100] = 0;
    return 0;
}

int __wrap_flock(int fd, int operation) {
    assert(fd == 100 && operation == (LOCK_EX | LOCK_NB));
    if (lock_failure) {
        errno = EWOULDBLOCK;
        return -1;
    }
    return step();
}

int __wrap_ioctl(int fd, unsigned long operation, ...) {
    va_list args;
    va_start(args, operation);
    void* data = va_arg(args, void*);
    va_end(args);
    assert(fd >= 100 && fd <= 102 && opened[fd - 100]);
    if (step() < 0)
        return -1;
    switch (operation) {
        case SPI_IOC_WR_MODE32:
            assert(fd == 100 && *(uint32_t*)data == SPI_MODE_3);
            break;
        case SPI_IOC_WR_BITS_PER_WORD:
            assert(*(uint8_t*)data == 8);
            break;
        case SPI_IOC_WR_MAX_SPEED_HZ:
            assert(*(uint32_t*)data == 1000000);
            break;
        case GPIO_GET_LINEHANDLE_IOCTL: {
            struct gpiohandle_request* r = data;
            assert(fd == 101 && r->lineoffsets[0] == 22 && r->lineoffsets[1] == 20);
            assert(r->lines == 2 && r->flags == GPIOHANDLE_REQUEST_OUTPUT);
            assert(r->default_values[0] == 1 && r->default_values[1] == 1);
            r->fd = 102;
            opened[2] = 1;
            break;
        }
        case GPIOHANDLE_SET_LINE_VALUES_IOCTL:
            assert(fd == 102);
            break;
        case SPI_IOC_MESSAGE(1): {
            struct spi_ioc_transfer* t = data;
            assert(fd == 100 && t->speed_hz == 1000000 && t->bits_per_word == 8);
            if (!t->len) {
                assert(!t->tx_buf && !t->cs_change);
                ++releases;
            }
            return (int)t->len - (short_write && t->len ? 1 : 0);
        }
        default:
            assert(0);
    }
    return 0;
}

int __wrap_nanosleep(const struct timespec* requested, struct timespec* remaining) {
    assert(requested->tv_nsec >= 0 && requested->tv_nsec < 1000000000);
    if (interrupted) {
        interrupted = 0;
        *remaining = *requested;
        errno = EINTR;
        return -1;
    }
    return 0;
}

/* Buildroot ARM redirects these libc calls for 64-bit file offsets and time. */
int __wrap_open64(const char*, int, ...) __attribute__((alias("__wrap_open")));
int __wrap___ioctl_time64(int, unsigned long, ...) __attribute__((alias("__wrap_ioctl")));
int __wrap___nanosleep64(const struct timespec*, struct timespec*) __attribute__((alias("__wrap_nanosleep")));

static void closed(void) {
    assert(!opened[0] && !opened[1] && !opened[2]);
}

int main(void) {
    struct linux_display io;
    struct Rm690a0 screen;
    assert(linux_display_open(&io, &screen, "spi", "gpio", 0, 4096) < 0);
    assert(linux_display_open(&io, &screen, "spi", "gpio", 1000000, 3) < 0);
    assert(calls == 0);
    assert(linux_display_open(&io, &screen, "spi", "gpio", 1000000, 4096) == 0);
    int open_calls = calls;
    assert(!opened[1]);
    interrupted = 1;
    assert(rm_init(&screen) == 0 && !interrupted);
    short_write = 1;
    const uint8_t pixel[] = {0xf8, 0};
    assert(rm_write(&screen, 0, 0, 1, 1, pixel, 2) < 0 && errno == EIO && !screen.ready);
    short_write = 0;
    assert(linux_display_close(&io) == 0);
    closed();
    assert(linux_display_close(&io) == 0);
    for (int i = 1; i <= open_calls; ++i) {
        calls = 0;
        fail_at = i;
        assert(linux_display_open(&io, &screen, "spi", "gpio", 1000000, 4096) < 0);
        assert(errno == EACCES);
        closed();
    }
    fail_at = 0;
    lock_failure = 1;
    int before = releases;
    assert(linux_display_open(&io, &screen, "spi", "gpio", 1000000, 4096) < 0);
    assert(errno == EWOULDBLOCK && releases == before); /* Never disturb another owner. */
    closed();
    lock_failure = 0;
    assert(linux_display_open(&io, &screen, "spi", "gpio", 1000000, 4096) == 0);
    fail_at = calls + 1;
    assert(linux_display_close(&io) < 0 && errno == EACCES);
    closed();
    puts("P3 Linux IO configuration, short transfer and cleanup checks passed");
    return 0;
}
