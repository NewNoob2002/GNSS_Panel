#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "platform/linux_button.h"

static const char* action_name(int action) {
    switch (action) {
        case LINUX_POWER_PRESS:
            return "V+ Power press";
        case LINUX_POWER_RELEASE:
            return "V+ Power release";
        case LINUX_FUNCTION_PRESS:
            return "V- Function press";
        case LINUX_FUNCTION_RELEASE:
            return "V- Function release";
        default:
            return "unknown";
    }
}

/* 单独测 evdev 按键，避免屏幕同步 SPI flush 阻塞采样。由外部 timeout 限时。 */
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s EVDEV_DEVICE\n", argv[0]);
        return 2;
    }
    int fd = linux_button_open(argv[1]);
    if (fd < 0) {
        perror("evdev open");
        return 1;
    }
    for (;;) {
        int action = linux_button_read(fd);
        if (action < 0) {
            perror("evdev read");
            close(fd);
            return 1;
        }
        if (action > 0) {
            puts(action_name(action));
            fflush(stdout);
        }
        const struct timespec interval = {.tv_sec = 0, .tv_nsec = 5000000};
        if (nanosleep(&interval, NULL) < 0 && errno != EINTR) {
            perror("nanosleep");
            close(fd);
            return 1;
        }
    }
}
