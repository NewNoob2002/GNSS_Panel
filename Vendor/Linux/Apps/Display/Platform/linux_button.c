#include "linux_button.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

int linux_button_open(const char* event_device) {
    const int fd = open(event_device, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        return -1;
    // Event stamps default to CLOCK_REALTIME; gesture timing compares them with the monotonic UI clock.
    int clock_id = CLOCK_MONOTONIC;
    if (ioctl(fd, EVIOCSCLOCKID, &clock_id) < 0) {
        const int error = errno;
        close(fd);
        errno = error;
        return -1;
    }
    return fd;
}
int linux_button_read_keys(int event_fd, unsigned power_code, unsigned function_code) {
    return linux_button_read_keys_timed(event_fd, power_code, function_code, NULL);
}
int linux_button_read_keys_timed(int event_fd, unsigned power_code, unsigned function_code,
                                 unsigned long long* time_ms) {
    if (power_code > KEY_MAX || function_code > KEY_MAX || power_code == function_code) {
        errno = EINVAL;
        return -1;
    }
    // Skip records that are not a mapped press/release (EV_SYN, repeats, other keys), so NONE means the
    // non-blocking queue is empty and callers can drain it in one loop.
    for (;;) {
        struct input_event event;
        const ssize_t size = read(event_fd, &event, sizeof(event));
        if (size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                return LINUX_BUTTON_NONE;
            return -1;
        }
        if (size != (ssize_t)sizeof(event)) {
            errno = EIO;
            return -1;
        }
        if (event.type != EV_KEY || (event.value != 0 && event.value != 1)
            || (event.code != power_code && event.code != function_code))
            continue;
        if (time_ms)
            *time_ms = (unsigned long long)event.input_event_sec * 1000ULL
                       + (unsigned long long)event.input_event_usec / 1000ULL;
        if (event.code == power_code)
            return event.value ? LINUX_POWER_PRESS : LINUX_POWER_RELEASE;
        return event.value ? LINUX_FUNCTION_PRESS : LINUX_FUNCTION_RELEASE;
    }
}
int linux_button_read(int event_fd) {
    return linux_button_read_keys(event_fd, KEY_VOLUMEUP, KEY_VOLUMEDOWN);
}
int linux_button_read_timed(int event_fd, unsigned long long* time_ms) {
    return linux_button_read_keys_timed(event_fd, KEY_VOLUMEUP, KEY_VOLUMEDOWN, time_ms);
}
