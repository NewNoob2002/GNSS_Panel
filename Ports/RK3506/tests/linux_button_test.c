// Linux evdev button mapping test
#include "../platform/linux_button.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

static void check_event(int write_fd, int read_fd, unsigned type, unsigned code, int value, int expected) {
    struct input_event event = {.type = type, .code = code, .value = value};
    assert(write(write_fd, &event, sizeof(event)) == (ssize_t)sizeof(event));
    assert(linux_button_read(read_fd) == expected);
}

int main(void) {
    int pipe_fd[2];
    assert(pipe(pipe_fd) == 0);
    assert(fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK) == 0);
    assert(linux_button_read(pipe_fd[0]) == LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEUP, 1, LINUX_POWER_PRESS);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEUP, 0, LINUX_POWER_RELEASE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEDOWN, 1, LINUX_FUNCTION_PRESS);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEDOWN, 0, LINUX_FUNCTION_RELEASE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEUP, 2, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_VOLUMEDOWN, 2, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_MENU, 1, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_MENU, 0, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_ESC, 1, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_ENTER, 1, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_KEY, KEY_A, 1, LINUX_BUTTON_NONE);
    check_event(pipe_fd[1], pipe_fd[0], EV_SYN, 0, 0, LINUX_BUTTON_NONE);

    // One drain sees a whole click with its kernel stamps; EV_SYN and unmapped keys are skipped.
    const struct input_event burst[] = {
        {.time = {.tv_sec = 12, .tv_usec = 345000}, .type = EV_KEY, .code = KEY_VOLUMEUP, .value = 1},
        {.type = EV_SYN},
        {.type = EV_KEY, .code = KEY_MENU, .value = 1},
        {.time = {.tv_sec = 12, .tv_usec = 405999}, .type = EV_KEY, .code = KEY_VOLUMEUP, .value = 0},
        {.type = EV_SYN},
    };
    assert(write(pipe_fd[1], burst, sizeof(burst)) == (ssize_t)sizeof(burst));
    unsigned long long stamp = 0;
    assert(linux_button_read_timed(pipe_fd[0], &stamp) == LINUX_POWER_PRESS && stamp == 12345);
    assert(linux_button_read_timed(pipe_fd[0], &stamp) == LINUX_POWER_RELEASE && stamp == 12405);
    stamp = 7;
    assert(linux_button_read_timed(pipe_fd[0], &stamp) == LINUX_BUTTON_NONE && stamp == 7);
    errno = 0;
    assert(linux_button_read_keys(pipe_fd[0], KEY_VOLUMEUP, KEY_VOLUMEUP) == -1 && errno == EINVAL);
    errno = 0;
    assert(linux_button_read_keys(pipe_fd[0], KEY_MAX + 1, KEY_VOLUMEDOWN) == -1 && errno == EINVAL);
    assert(close(pipe_fd[1]) == 0);
    errno = 0;
    assert(linux_button_read(pipe_fd[0]) == -1 && errno == EIO);
    assert(close(pipe_fd[0]) == 0);
}
