#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "benchmark/lv_demo_benchmark.h"
#include "demo_perf.h"
#include "linux_display.h"
#include "lv_display.h"
#include "lv_tick_posix.h"

static volatile sig_atomic_t stop;
static void on_signal(int signal_number) {
    (void)signal_number;
    stop = 1;
}

static int number(const char* text, unsigned long max, unsigned long* value) {
    char* end;
    errno = 0;
    *value = strtoul(text, &end, 10);
    return !errno && text[0] >= '0' && text[0] <= '9' && !*end && *value && *value <= max;
}

static uint64_t milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
        return 0;
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}

static int benchmark_summary(void) {
    lv_obj_t* root = lv_screen_active();
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        lv_obj_t* child = lv_obj_get_child(root, i);
        if (!lv_obj_check_type(child, &lv_label_class))
            continue;
        const char* text = lv_label_get_text(child);
        if (strncmp(text, "Weighted FPS:", 13) == 0) {
            fprintf(stderr, "benchmark %s\n", text);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    unsigned long speed, chunk = 4096;
    if ((argc != 4 && argc != 5) || !number(argv[3], UINT32_MAX, &speed)
        || (argc == 5 && !number(argv[4], 32768, &chunk)) || chunk < 4 || chunk % 2) {
        fprintf(stderr, "Usage: %s SPI_DEVICE GPIO1_CHIP VERIFIED_SPEED_HZ [EVEN_CHUNK_BYTES=4096]\n", argv[0]);
        return EXIT_FAILURE;
    }
    struct sigaction action = {.sa_handler = on_signal};
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGTERM, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    struct linux_display io;
    struct Rm690a0 screen;
    if (linux_display_open(&io, &screen, argv[1], argv[2], (uint32_t)speed, chunk) < 0) {
        perror("display open");
        return EXIT_FAILURE;
    }
    int result = EXIT_FAILURE;
    static struct panel_display port;
    if (rm_init(&screen) < 0) {
        perror("RM690A0 init");
        goto done;
    }
    if (stop)
        goto done;
    lv_init();
    lv_tick_set_cb(lv_tick_posix_ms);
    if (lv_display_register(&port, &screen) < 0) {
        perror("LVGL display");
        goto done;
    }
    lv_demo_benchmark();
    uint64_t last = milliseconds();
    struct demo_perf perf;
    demo_perf_start(&perf, port.display, last);
    uint64_t last_report = last;
    uint64_t handler_total_ms = 0, handler_max_ms = 0;
    unsigned handler_calls = 0;
    int finished = 0;
    while (!stop && !port.error && !finished) {
        uint64_t now = milliseconds();
        if (!now || now < last) {
            fprintf(stderr, "monotonic clock failed\n");
            port.error = EIO;
            break;
        }
        last = now;
        uint64_t handler_start = milliseconds();
        lv_timer_handler();
        now = milliseconds();
        if (benchmark_summary()) {
            lv_refr_now(port.display);
            demo_perf_finish(&perf, milliseconds());
            finished = 1;
            break;
        }
        uint64_t handler_ms = now - handler_start;
        handler_total_ms += handler_ms;
        if (handler_ms > handler_max_ms)
            handler_max_ms = handler_ms;
        ++handler_calls;
        demo_perf_update(&perf, now);
        if (now - last_report >= 5000) {
            fprintf(stderr, "handler: %u calls, %llu ms total, %llu ms max\n", handler_calls,
                    (unsigned long long)handler_total_ms, (unsigned long long)handler_max_ms);
            handler_calls = 0;
            handler_total_ms = handler_max_ms = 0;
            last_report = now;
        }
        struct timespec wait = {.tv_nsec = 5000000};
        if (nanosleep(&wait, NULL) < 0 && errno != EINTR) {
            perror("nanosleep");
            break;
        }
    }
    if (port.error) {
        errno = port.error;
        perror("display flush");
    } else if (stop || finished) {
        result = EXIT_SUCCESS;
    }
    lv_display_delete(port.display);
done:
    if (linux_display_close(&io) < 0) {
        perror("display close");
        result = EXIT_FAILURE;
    }
    return result;
}
