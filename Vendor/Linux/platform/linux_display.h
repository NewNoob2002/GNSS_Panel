#ifndef LINUX_DISPLAY_H
#define LINUX_DISPLAY_H

#include "rm690a0.h"

struct linux_display {
    int spi_fd, gpio_fd;
    uint32_t speed;
    uint8_t dc, reset;
};

/* GPIO chip must be the verified bank GPIO1; offsets are DC=22, RESET=20. */
int linux_display_open(struct linux_display* io, struct Rm690a0* screen, const char* spi, const char* gpiochip,
                       uint32_t speed, size_t chunk);
int linux_display_close(struct linux_display* io);

#endif
