#define _POSIX_C_SOURCE 200809L
#include "linux_display.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static int transfer(void* context, const uint8_t* data, size_t size, int keep) {
    struct linux_display* io = context;
    struct spi_ioc_transfer t = {.tx_buf = (uintptr_t)data,
                                 .len = (uint32_t)size,
                                 .speed_hz = io->speed,
                                 .bits_per_word = 8,
                                 .cs_change = !!keep};
    int n = ioctl(io->spi_fd, SPI_IOC_MESSAGE(1), &t);
    if (n < 0)
        return -1; /* Do not retry ambiguous/partially completed writes. */
    if ((size_t)n != size) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static int gpio_set(struct linux_display* io) {
    struct gpiohandle_data values = {.values = {io->dc, io->reset}};
    return ioctl(io->gpio_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &values);
}

static int dc(void* context, int value) {
    struct linux_display* io = context;
    io->dc = !!value;
    return gpio_set(io);
}

static int reset(void* context, int value) {
    struct linux_display* io = context;
    io->reset = !!value;
    return gpio_set(io);
}

static int sleep_ms(void* context, unsigned ms) {
    (void)context;
    struct timespec delay = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000};
    while (nanosleep(&delay, &delay) < 0)
        if (errno != EINTR)
            return -1;
    return 0;
}

int linux_display_close(struct linux_display* io) {
    int error = 0;
    if (io->spi_fd >= 0) {
        if (transfer(io, NULL, 0, 0) < 0)
            error = errno;
        if (close(io->spi_fd) < 0 && !error)
            error = errno;
        io->spi_fd = -1;
    }
    if (io->gpio_fd >= 0) {
        if (close(io->gpio_fd) < 0 && !error)
            error = errno;
        io->gpio_fd = -1;
    }
    if (error) {
        errno = error;
        return -1;
    }
    return 0;
}

int linux_display_open(struct linux_display* io, struct Rm690a0* screen, const char* spi, const char* gpiochip,
                       uint32_t speed, size_t chunk) {
    *io = (struct linux_display){.spi_fd = -1, .gpio_fd = -1, .speed = speed, .dc = 1, .reset = 1};
    *screen = (struct Rm690a0){0};
    if (!speed || chunk < 4 || chunk > 32768 || chunk % 2) {
        errno = EINVAL;
        return -1;
    }
    io->spi_fd = open(spi, O_RDWR | O_CLOEXEC);
    if (io->spi_fd < 0)
        return -1;
    /* Advisory lock prevents another instance; it cannot reserve other bus devices. */
    if (flock(io->spi_fd, LOCK_EX | LOCK_NB) < 0) {
        int error = errno;
        close(io->spi_fd);
        io->spi_fd = -1;
        errno = error;
        return -1;
    }
    uint32_t mode = SPI_MODE_3;
    uint8_t bits = 8;
    if (ioctl(io->spi_fd, SPI_IOC_WR_MODE32, &mode) < 0 || ioctl(io->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0
        || ioctl(io->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
        goto fail;
    int chip = open(gpiochip, O_RDONLY | O_CLOEXEC);
    if (chip < 0)
        goto fail;
    struct gpiohandle_request request = {.lineoffsets = {22, 20},
                                         .flags = GPIOHANDLE_REQUEST_OUTPUT,
                                         .default_values = {1, 1},
                                         .consumer_label = "rm690a0",
                                         .lines = 2};
    int rc = ioctl(chip, GPIO_GET_LINEHANDLE_IOCTL, &request);
    int error = errno;
    close(chip);
    errno = error;
    if (rc < 0)
        goto fail;
    io->gpio_fd = request.fd;
    screen->io = (struct RmIo){io, dc, reset, sleep_ms, transfer, chunk};
    return 0;
fail:
    error = errno;
    linux_display_close(io);
    errno = error;
    return -1;
}
