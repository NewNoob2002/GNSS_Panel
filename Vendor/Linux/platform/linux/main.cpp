#define _POSIX_C_SOURCE 200809L

#include "App.h"
#include "DemoStatus.h"
extern "C" {
#include "lv_display.h"
#include "lv_tick_posix.h"
#include "linux_button.h"
#include "linux_display.h"
}

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

static volatile sig_atomic_t stop;
static void on_signal(int) {
    stop = 1;
}

static bool number(const char* text, unsigned long max, unsigned long& value) {
    char* end;
    errno = 0;
    value = std::strtoul(text, &end, 10);
    return !errno && text[0] >= '0' && text[0] <= '9' && !*end && value && value <= max;
}

static std::uint64_t milliseconds() {
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return 0;
    return std::uint64_t(now.tv_sec) * 1000 + std::uint64_t(now.tv_nsec) / 1000000;
}

int main(int argc, char** argv) {
    const bool display_only = argc > 1 && std::strcmp(argv[1], "--display-only") == 0;
    const char* page = display_only && argc > 3 && std::strcmp(argv[2], "--page") == 0 ? argv[3] : nullptr;
    const int first = page ? 4 : display_only ? 2 : 1;
    const char* event_device = "/dev/input/event0";
    unsigned long speed, chunk = 4096;
    if (display_only) {
        if ((argc != first + 3 && argc != first + 4) || !number(argv[first + 2], UINT32_MAX, speed)
            || (argc == first + 4 && !number(argv[first + 3], 32768, chunk))) {
            std::fprintf(stderr,
                         "Usage: %s [--display-only [--page Pages/NAME]] SPI_DEVICE GPIO1_CHIP VERIFIED_SPEED_HZ "
                         "[EVEN_CHUNK_BYTES]\n",
                         argv[0]);
            return EXIT_FAILURE;
        }
    } else {
        if (argc < first + 3 || argc > first + 5 || !number(argv[first + 2], UINT32_MAX, speed)
            || (argc == first + 5 && !number(argv[first + 4], 32768, chunk))) {
            std::fprintf(stderr,
                         "Usage: %s SPI_DEVICE GPIO1_CHIP VERIFIED_SPEED_HZ [EVDEV_DEVICE [EVEN_CHUNK_BYTES]]\n",
                         argv[0]);
            return EXIT_FAILURE;
        }
        if (argc >= first + 4)
            event_device = argv[first + 3];
    }
    if (chunk < 4 || chunk % 2) {
        std::fprintf(stderr, "EVEN_CHUNK_BYTES must be an even value from 4 to 32768\n");
        return EXIT_FAILURE;
    }
    struct sigaction action{};
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, nullptr) < 0 || sigaction(SIGTERM, &action, nullptr) < 0) {
        std::perror("sigaction");
        return EXIT_FAILURE;
    }
    // 先打开 evdev，避免屏初始化后才发现输入设备不可用。
    int button_fd = display_only ? -1 : linux_button_open(event_device);
    if (!display_only && button_fd < 0) {
        std::perror(event_device);
        return EXIT_FAILURE;
    }
    linux_display io;
    Rm690a0 screen;
    if (linux_display_open(&io, &screen, argv[first], argv[first + 1], speed, chunk) < 0) {
        std::perror("display open");
        if (button_fd >= 0)
            close(button_fd);
        return EXIT_FAILURE;
    }
    int result = EXIT_FAILURE;
    if (rm_init(&screen) < 0) {
        std::perror("RM690A0 init");
    } else if (!stop) {
        lv_init();
        lv_tick_set_cb(lv_tick_posix_ms);
        panel_display port{};
        if (lv_display_register(&port, &screen) < 0) {
            std::perror("LVGL display");
        } else {
            {
                App app;
                if (!app.init()) {
                    std::fprintf(stderr, "App init failed\n");
                } else {
                    if (page) {
                        lv_delay_ms(50);
                        lv_timer_handler();
                    }
                    if (page && !app.show_page(page)) {
                        std::fprintf(stderr, "Unknown or unavailable page: %s\n", page);
                    } else {
                        std::uint64_t last = milliseconds();
                        const std::uint64_t started = last; // DemoStatus takes elapsed time, as in simulator.
                        std::uint64_t next_status_update = last;
                        while (!stop && !port.error) {
                            std::uint64_t now = milliseconds();
                            if (!now || now < last) {
                                std::fprintf(stderr, "monotonic clock failed\n");
                                break;
                            }
                            last = now;
                            if (now >= next_status_update) {
                                app.update_status(DemoStatus::sample(now - started));
                                next_status_update = now + 200;
                            }
                            if (button_fd >= 0) {
                                // Drain every queued event: one key transition is several evdev records, and a
                                // slow frame must not delay or merge clicks. Each transition keeps its own stamp.
                                bool read_failed = false;
                                for (;;) {
                                    unsigned long long event_ms = 0;
                                    const int event_action = linux_button_read_timed(button_fd, &event_ms);
                                    if (event_action < 0) {
                                        read_failed = true;
                                        break;
                                    }
                                    if (event_action == LINUX_BUTTON_NONE)
                                        break; // Queue empty; the reader already skipped EV_SYN and other keys.
                                    const bool pressed =
                                        event_action == LINUX_POWER_PRESS || event_action == LINUX_FUNCTION_PRESS;
                                    const auto key =
                                        event_action == LINUX_POWER_PRESS || event_action == LINUX_POWER_RELEASE
                                            ? App::Key::Power
                                            : App::Key::Function;
                                    app.on_key_at(key, pressed, event_ms);
                                }
                                if (read_failed) {
                                    app.cancel_input();
                                    std::perror("evdev read");
                                    break;
                                }
                                app.poll_keys(now);
                            }
                            app.update();
                            lv_timer_handler();
                            timespec wait{0, 5000000};
                            if (nanosleep(&wait, nullptr) < 0 && errno != EINTR) {
                                std::perror("nanosleep");
                                break;
                            }
                        }
                        if (stop && !port.error)
                            result = EXIT_SUCCESS;
                    }
                }
            }
            if (port.error) {
                errno = port.error;
                std::perror("display flush");
            }
            lv_display_delete(port.display);
        }
    }
    if (linux_display_close(&io) < 0) {
        std::perror("display close");
        result = EXIT_FAILURE;
    }
    if (button_fd >= 0 && close(button_fd) < 0) {
        std::perror("button close");
        result = EXIT_FAILURE;
    }
    return result;
}
