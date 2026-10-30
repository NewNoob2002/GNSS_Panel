#pragma once

/** Normalized Power/Function evdev transitions for the test board. */
enum linux_button_event {
    LINUX_BUTTON_NONE,
    LINUX_POWER_PRESS,
    LINUX_POWER_RELEASE,
    LINUX_FUNCTION_PRESS,
    LINUX_FUNCTION_RELEASE
};
int linux_button_open(const char* event_device);
/** Test-board mapping: V+ is Power, V- is Function; MENU/ESC and repeats are ignored.
 * Returns a linux_button_event, or -1 with errno on I/O errors.
 * Emits press/release transitions; the application interprets holds and single/double clicks.
 */
int linux_button_read(int event_fd);
/** Configurable evdev adapter with explicitly supplied distinct key codes.
 * Returns the next mapped press/release, skipping other records; LINUX_BUTTON_NONE means the non-blocking
 * queue is empty (or the read was interrupted). Returns -1 with errno on I/O errors.
 * Does not read GPIO HIGH/LOW levels or configure product pins/PMIC.
 */
int linux_button_read_keys(int event_fd, unsigned power_code, unsigned function_code);
/** As linux_button_read_keys(), also returning the event timestamp in milliseconds when time_ms is not NULL.
 * linux_button_open() selects CLOCK_MONOTONIC, so the stamp matches clock_gettime(CLOCK_MONOTONIC).
 */
int linux_button_read_keys_timed(int event_fd, unsigned power_code, unsigned function_code,
                                 unsigned long long* time_ms);
/** Test-board V+/V- mapping of linux_button_read_keys_timed(). */
int linux_button_read_timed(int event_fd, unsigned long long* time_ms);
