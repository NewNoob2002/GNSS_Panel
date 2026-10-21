#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "linux_display.h"
#include "lv_display.h"

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

int main(int argc, char** argv) {
    unsigned long speed, chunk = 4096;
    if ((argc != 4 && argc != 5) || !number(argv[3], UINT32_MAX, &speed)
        || (argc == 5 && !number(argv[4], 32768, &chunk)) || chunk < 4 || chunk % 2) {
        fprintf(stderr,
                "Usage: %s SPI_DEVICE GPIO1_CHIP VERIFIED_SPEED_HZ [EVEN_CHUNK_BYTES=4096]\n"
                "Requires exclusive SPI0 bus use; verify wiring, voltage and GPIO bank first.\n",
                argv[0]);
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
    static struct lv_display port;
    if (rm_init(&screen) < 0) {
        perror("RM690A0 init");
        goto done;
    }
    if (stop)
        goto done;
    lv_init();
    if (lv_display_register(&port, &screen) < 0) {
        perror("LVGL display");
        goto done;
    }
    lv_display_pattern();
    uint64_t last = milliseconds();
    while (!stop && !port.error) {
        uint64_t now = milliseconds();
        if (!now || now < last) {
            fprintf(stderr, "monotonic clock failed\n");
            port.error = EIO;
            break;
        }
        lv_tick_inc((uint32_t)(now - last));
        last = now;
        lv_timer_handler();
        struct timespec wait = {.tv_nsec = 5000000};
        nanosleep(&wait, NULL);
    }
    if (port.error) {
        errno = port.error;
        perror("display flush");
    } else {
        result = EXIT_SUCCESS;
    }
    lv_disp_remove(port.display);
done:
    if (linux_display_close(&io) < 0) {
        perror("display close");
        result = EXIT_FAILURE;
    }
    return result;
}
