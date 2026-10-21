#ifndef RK3506_DEMO_PERF_H
#define RK3506_DEMO_PERF_H

#include <stdint.h>
#include "lvgl.h"

struct demo_perf {
    lv_obj_t* label;
    void (*benchmark_monitor)(lv_disp_drv_t*, uint32_t, uint32_t);
    uint64_t updates;
    uint64_t pixels;
    uint64_t total_updates;
    uint64_t start_wall_ms;
    uint64_t last_wall_ms;
    uint64_t last_cpu_ns;
};

/* Attach after lv_demo_benchmark() so the demo's own monitor still runs. */
void demo_perf_start(struct demo_perf* perf, lv_disp_t* display, uint64_t wall_ms);
void demo_perf_update(struct demo_perf* perf, uint64_t wall_ms);
void demo_perf_finish(const struct demo_perf* perf, uint64_t wall_ms);

#endif
