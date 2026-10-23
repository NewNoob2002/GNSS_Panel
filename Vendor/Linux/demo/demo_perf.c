#define _POSIX_C_SOURCE 200809L
#include "demo_perf.h"
#include <stdio.h>
#include <time.h>

static uint64_t process_nanoseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now) != 0)
        return 0;
    return (uint64_t)now.tv_sec * 1000000000u + (uint64_t)now.tv_nsec;
}

/** @brief 按完整渲染帧统计 FPS，并累计实际提交到端口的像素数。 */
static void
monitor(lv_event_t* event) {
    struct demo_perf* perf = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_RENDER_READY) {
        ++perf->updates;
        ++perf->total_updates;
    } else if (lv_event_get_code(event) == LV_EVENT_FLUSH_START) {
        const lv_area_t* area = lv_event_get_param(event);
        perf->pixels += (uint64_t)lv_area_get_width(area) * (uint64_t)lv_area_get_height(area);
    }
}

void
demo_perf_start(struct demo_perf* perf, lv_display_t* display, uint64_t wall_ms) {
    *perf = (struct demo_perf){0};
    lv_display_add_event_cb(display, monitor, LV_EVENT_RENDER_READY, perf);
    lv_display_add_event_cb(display, monitor, LV_EVENT_FLUSH_START, perf);
    perf->start_wall_ms = perf->last_wall_ms = wall_ms;
    perf->last_cpu_ns = process_nanoseconds();
    perf->label = lv_label_create(lv_layer_sys());
    lv_obj_set_style_bg_opa(perf->label, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(perf->label, lv_color_black(), 0);
    lv_obj_set_style_text_color(perf->label, lv_color_white(), 0);
    lv_label_set_text(perf->label, "0.0 FPS\n0% CPU");
    lv_obj_align(perf->label, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
}

void demo_perf_update(struct demo_perf* perf, uint64_t wall_ms) {
    uint64_t elapsed_ms = wall_ms - perf->last_wall_ms;
    if (elapsed_ms < 5000)
        return;
    uint64_t cpu_ns = process_nanoseconds();
    uint64_t used_cpu_ns = cpu_ns >= perf->last_cpu_ns ? cpu_ns - perf->last_cpu_ns : 0;
    uint32_t fps_x10 = (uint32_t)(perf->updates * 10000u / elapsed_ms);
    uint32_t cpu_x10 = (uint32_t)(used_cpu_ns / (elapsed_ms * 1000u));
    fprintf(stderr, "display: %u.%u FPS, %u.%u%% process CPU, %llu px/s\n", fps_x10 / 10, fps_x10 % 10, cpu_x10 / 10,
            cpu_x10 % 10, (unsigned long long)(perf->pixels * 1000u / elapsed_ms));
    lv_label_set_text_fmt(perf->label, "%u.%u FPS\n%u.%u%% CPU", fps_x10 / 10, fps_x10 % 10, cpu_x10 / 10,
                          cpu_x10 % 10);
    perf->updates = perf->pixels = 0;
    perf->last_wall_ms = wall_ms;
    perf->last_cpu_ns = cpu_ns;
}

void demo_perf_finish(const struct demo_perf* perf, uint64_t wall_ms) {
    uint64_t elapsed_ms = wall_ms - perf->start_wall_ms;
    if (!elapsed_ms)
        return;
    uint32_t fps_x10 = (uint32_t)(perf->total_updates * 10000u / elapsed_ms);
    fprintf(stderr, "completed: %llu updates in %llu ms = %u.%u actual FPS\n", (unsigned long long)perf->total_updates,
            (unsigned long long)elapsed_ms, fps_x10 / 10, fps_x10 % 10);
}
